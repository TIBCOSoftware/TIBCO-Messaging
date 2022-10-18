/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

package security

import (
	"bufio"
	"encoding/base64"
	"encoding/json"
	"io"
	"log"
	"net/http"
	"os"
	"regexp"
	"strings"
	"time"
)

var validUser = regexp.MustCompile(`^(?P<username>.+):\s*(?P<password>\S.*?)(?:,\s+(?P<roles>[\w-]+(?:,[\w-]+)*)\s*)?$`)
var ignoreLine = regexp.MustCompile(`^\s*(#.*)?$`)

type userRec struct {
	password string
	roles    []string
}

type authTable struct {
	cache     map[string]userRec
	auth      map[string]string
	authGroup string
	file      string
	modTime   time.Time
}

func (t *authTable) readLine(lineNumber int, line string) {
	if !ignoreLine.MatchString(line) {
		match := validUser.FindStringSubmatch(line)
		if len(match) == 4 {
			match = match[1:]
			result := make(map[string]string)
			for i, name := range validUser.SubexpNames()[1:] {
				result[name] = match[i]
			}

			username := result["username"]
			password := result["password"]
			if _, ok := t.cache[username]; ok {
				log.Printf("Line %d: User `%s` was found multiple times, overwriting.\n",
					lineNumber, username)
			}

			t.cache[username] = userRec{
				password: base64.StdEncoding.EncodeToString([]byte(password)),
				roles:    strings.Split(result["roles"], ",")}

			for _, role := range t.cache[username].roles {
				if role == t.authGroup {
					t.auth[username] = password
				}
			}
		} else {
			log.Printf("Line %d: Was not able to parse `%s`. "+
				"Commented lines start with `#`. "+
				"The separator between username and password is ':' (colon followed by optional whitespace). "+
				"Password and optional roles are separated by ', ' (comma followed by at least one whitespace). "+
				"The list of roles must not contain spaces.", lineNumber, line)
		}
	}
}

func (t *authTable) readAll(r io.Reader) {
	scanner := bufio.NewScanner(r)
	for lineNumber := 1; scanner.Scan(); lineNumber++ {
		line := scanner.Text()
		t.readLine(lineNumber, line)
	}
	if err := scanner.Err(); err != nil {
		log.Println("Reading standard input:", err)
	} else if _, exists := t.auth[""]; t.authGroup == "" && !exists {
		t.auth[""] = ""
	} else if len(t.auth) == 0 {
		log.Printf("No user with role '%s' for authentication against this service found\n",
			t.authGroup)
	}
}

func (t *authTable) reRead() {
	info, err := os.Stat(t.file)
	if err != nil {
		log.Println("Obtaining file stat did not work", err)
	} else if t.modTime != info.ModTime() {
		if f, err := os.OpenFile(t.file, os.O_RDONLY, 0); err != nil {
			log.Println("Opening file did not work", err)
		} else {
			defer f.Close()
			t.cache = make(map[string]userRec)
			t.auth = make(map[string]string)
			t.readAll(bufio.NewReader(f))
			t.modTime = info.ModTime()
		}
	}
}

func SetupAuthService(file string, refresh int, auth string) (error, http.Handler, chan struct{}) {
	if f, err := os.OpenFile(file, os.O_RDONLY, 0); err != nil {
		return err, nil, nil
	} else {
		f.Close()

		t := authTable{
			cache:     make(map[string]userRec),
			file:      file,
			auth:      make(map[string]string),
			authGroup: auth}
		t.reRead()

		type request struct {
			Username string `json:"username"`
			Password string `json:"password"`
			authUser string
			authPwd  string
		}

		type response struct {
			Authenticated bool     `json:"authenticated"`
			Roles         []string `json:"roles,omitempty"`
		}

		ticker := time.NewTicker(time.Duration(refresh) * time.Second)
		quit := make(chan struct{})
		reqChan := make(chan request)
		resChan := make(chan response)
		go func() {
			for {
				select {
				case <-quit:
					ticker.Stop()
					close(reqChan)
					close(resChan)
					return
				case <-ticker.C:
					t.reRead()
				case req := <-reqChan:
					resp := response{}

					if pwd, exists := t.auth[req.authUser]; exists && pwd == req.authPwd {
						if user, exists := t.cache[req.Username]; exists && user.password == req.Password {
							resp.Authenticated = true
							resp.Roles = user.roles
						}
					}
					resChan <- resp
				}
			}
		}()

		mux := http.NewServeMux()
		mux.HandleFunc("/auth", func(w http.ResponseWriter, r *http.Request) {
			d := json.NewDecoder(r.Body)
			defer r.Body.Close()

			req := request{}
			if err := d.Decode(&req); err != nil {
				return
			}

			authPresent := false
			req.authUser, req.authPwd, authPresent = r.BasicAuth()

			if !authPresent {
				return
			}

			reqChan <- req
			res := <-resChan

			w.Header().Set("Content-Type", "application/json; charset=UTF-8")
			e := json.NewEncoder(w)
			e.Encode(&res)
		})

		return err, mux, quit
	}
}
