/*
 * Copyright (c) 2001-$Date: 2018-05-21 11:55:18 -0500 (Mon, 21 May 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: conn.go 101362 2018-05-21 16:55:18Z bpeterse $
 */

package eftl

import (
	"crypto/tls"
	"fmt"
	"io"
	"math"
	"net/http"
	"net/url"
	"sort"
	"strconv"
	"sync"
	"time"

	"github.com/gorilla/websocket"
)

type eftlError struct {
	msg string
}

func (e *eftlError) Error() string { return e.msg }

type requests map[int64]*Completion

// Errors
var (
	ErrTimeout          = &eftlError{msg: "operation timed out"}
	ErrNotConnected     = &eftlError{msg: "not connected"}
	ErrInvalidResponse  = &eftlError{msg: "received invalid response from server"}
	ErrMessageTooBig    = &eftlError{msg: "message too big"}
	ErrNotAuthenticated = &eftlError{msg: "not authenticated"}
	ErrForceClose       = &eftlError{msg: "server has forcibly closed the connection"}
	ErrNotAuthorized    = &eftlError{msg: "not authorized for the operation"}
	ErrBadHandshake     = &eftlError{msg: "bad handshake"}
	ErrNotFound         = &eftlError{msg: "not found"}
)

// Options available to configure the connection.
type Options struct {
	// Username for authenticating with the server if not specified with
	// the URL.
	Username string

	// Password for authenticating with the server if not specified with
	// the URL.
	Password string

	// ClientID specifies an optional client identifier if not specified with
	// the URL. The server will generate a client identifier if one is not
	// specified.
	ClientID string

	// TLSConfig specifies the TLS configuration to use when creating a secure
	// connection to the server.
	TLSConfig *tls.Config

	// Timeout specifies the duration for a synchronous operation with the
	// server to complete. The default is 2 seconds.
	Timeout time.Duration

	// HandshakeTimeout specifies the duration for the websocket handshake
	// with the server to complete. The default is 10 seconds.
	HandshakeTimeout time.Duration

	// AutoReconnectAttempts specifies the number of times the client attempts to
	// automatically reconnect to the server following a loss of connection.
	// The default is 5.
	AutoReconnectAttempts int64

	// AutoReconnectMaxDelay determines the maximum delay between autoreconnect attempts.
	// Upon loss of a connection, the client will delay for 1 second before attempting to
	// automatically reconnect. Subsequent attempts double the delay duration, up to
	// the maximum value specified. The default is 30 seconds.
	AutoReconnectMaxDelay time.Duration
}

// Connection represents a connection to the server.
type Connection struct {
	URL               *url.URL
	Options           Options
	ErrorChan         chan error
	reconnectID       string
	wg                sync.WaitGroup
	mu                sync.Mutex
	ws                *websocket.Conn
	connected         bool
	reqs              requests
	reqSeqNum         int64
	subs              map[string]*Subscription
	subSeqNum         int64
	lastSeqNum        int64
	reconnectAttempts int64
	reconnectTimer    *time.Timer
}

// Options available to configure a subscription.
type SubscriptionOptions struct {
	// Durable subscription type; "shared" or "last-value".
	DurableType string

	// Key field for "last-value" durable subscriptions.
	DurableKey string
}

// Subscription represents an interest in application messages.
// When returned from an asynchronous subscribe operation a non-nil
// Error indicates a subscription failure.
type Subscription struct {
	Matcher          string
	Durable          string
	Options          SubscriptionOptions
	MessageChan      chan Message
	Error            error
	subscriptionID   string
	subscriptionChan chan *Subscription
}

func (sub *Subscription) toProtocol() Message {
	msg := Message{
		"op": opSubscribe,
		"id": sub.subscriptionID,
	}
	if sub.Matcher != "" {
		msg["matcher"] = sub.Matcher
	}
	if sub.Durable != "" {
		msg["durable"] = sub.Durable
	}
	if sub.Options.DurableType != "" {
		msg["type"] = sub.Options.DurableType
	}
	if sub.Options.DurableKey != "" {
		msg["key"] = sub.Options.DurableKey
	}
	return msg
}

// Completion represents a completed operation. When returned
// from an asynchronous operation a non-nil Error indicates
// a failure.
type Completion struct {
	Message        Message
	Error          error
	seqNum         int64
	request        Message
	completionChan chan *Completion
}

// subprotocol used for websocket communications.
const subprotocol = "v1.eftl.tibco.com"

// op codes
const (
	opHeartbeat    = 0
	opLogin        = 1
	opWelcome      = 2
	opSubscribe    = 3
	opSubscribed   = 4
	opUnsubscribe  = 5
	opUnsubscribed = 6
	opEvent        = 7
	opPublish      = 8
	opAck          = 9
	opError        = 10
	opDisconnect   = 11
	opMapSet       = 20
	opMapGet       = 22
	opMapRemove    = 24
	opMapResponse  = 26
)

// defaults
const (
	defaultHandshakeTimeout = 10 * time.Second
	defaultTimeout          = 2 * time.Second
)

// Connect establishes a connection to the server at the specified url.
//
// The url can be in either of these forms:
//   ws://host:port/channel
//   wss://host:port/channel
//
// Optionally, the url can contain the username, password, and/or client identifier:
//   ws://username:password@host:port/channel?clientId=<identifier>
//   wss://username:password@host:port/channel?clientId=<identifier>
//
// Connect is a synchronous operation and will block until either a
// connection has been establish or an error occurs.
//
// The errorChan is used to receive asynchronous connection errors
// once the connection has been established.
func Connect(urlStr string, opts *Options, errorChan chan error) (*Connection, error) {
	if opts == nil {
		opts = &Options{}
	}
	url, err := url.Parse(urlStr)
	if err != nil {
		return nil, err
	}
	// initialize the connection
	conn := &Connection{
		URL:       url,
		Options:   *opts,
		ErrorChan: errorChan,
		reqs:      make(requests),
		subs:      make(map[string]*Subscription),
	}
	// set default values
	if conn.Options.HandshakeTimeout == 0 {
		conn.Options.HandshakeTimeout = defaultHandshakeTimeout
	}
	if conn.Options.Timeout == 0 {
		conn.Options.Timeout = defaultTimeout
	}
	// connect to the server
	err = conn.connect()
	if err != nil {
		return nil, err
	}
	return conn, nil
}

// Reconnect re-establishes the connection to the server following a
// connection error or a disconnect. Reconnect is a synchronous operation
// and will block until either a connection has been established or an
// error occurs. Upon success subscriptions are re-established.
func (conn *Connection) Reconnect() error {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if conn.connected {
		return nil
	}
	// connect to the server
	return conn.connect()
}

// Disconnect closes the connection to the server.
func (conn *Connection) Disconnect() {
	conn.mu.Lock()
	if !conn.connected {
		conn.mu.Unlock()
		return
	}
	// send disconnect message
	conn.sendMessage(Message{
		"op": opDisconnect,
	})
	// cancel the reconnect timer
	if conn.reconnectTimer != nil {
		conn.reconnectTimer.Stop()
	}
	// close the connection to the server
	conn.disconnect()
	conn.mu.Unlock()
	// wait for the dispatcher go routine to end
	conn.wg.Wait()
}

// IsConnected test if the connection is connected to the server.
func (conn *Connection) IsConnected() bool {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	return conn.connected
}

// Publish an application message.
//
// It is recommended to publish messages to a specific destination
// by including the string field "_dest":
//
//     conn.Publish(Message{
//         "_dest": "sample",
//         "text": "Hello, World!",
//     })
//
func (conn *Connection) Publish(msg Message) error {
	completionChan := make(chan *Completion, 1)
	if err := conn.PublishAsync(msg, completionChan); err != nil {
		return err
	}
	select {
	case completion := <-completionChan:
		return completion.Error
	case <-time.After(conn.Options.Timeout):
		return ErrTimeout
	}
}

// Publish an application message asynchronously. The optional
// completionChan will receive notification once the publish
// operation completes.
//
// It is recommended to publish messages to a specific destination
// by including the string field "_dest":
//
//     conn.Publish(Message{
//         "_dest": "sample",
//         "text": "Hello, World!",
//     })
//
func (conn *Connection) PublishAsync(msg Message, completionChan chan *Completion) error {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if !conn.connected {
		return ErrNotConnected
	}
	// register the publish
	conn.reqSeqNum++
	conn.reqs[conn.reqSeqNum] = &Completion{
		Message: msg,
		request: Message{
			"op":   opPublish,
			"seq":  conn.reqSeqNum,
			"body": msg,
		},
		seqNum:         conn.reqSeqNum,
		completionChan: completionChan,
	}
	// send publish message
	return conn.sendMessage(conn.reqs[conn.reqSeqNum].request)
}

// Subscribe registers interest in application messages.
// A content matcher can be used to register interest in certain messages.
// A durable name can be specified to create a durable subscription.
// Messages are received on the messageChan.
//
// It is recommended to subscribe to messages published to a specific
// destination by creating a content matcher with the string field "_dest":
//
//     conn.Subscribe("{\"_dest\": \"sample\"}", "", messageChan)
//
func (conn *Connection) Subscribe(matcher string, durable string, messageChan chan Message) (*Subscription, error) {
	subscriptionChan := make(chan *Subscription, 1)
	if err := conn.SubscribeAsync(matcher, durable, messageChan, subscriptionChan); err != nil {
		return nil, err
	}
	select {
	case sub := <-subscriptionChan:
		return sub, sub.Error
	case <-time.After(conn.Options.Timeout):
		return nil, ErrTimeout
	}
}

// Subscribe registers interest in application messages asynchronously.
// A content matcher can be used to register interest in certain messages.
// A durable name can be specified to create a durable subscription.
// Messages are received on the messageChan. The subscriptionChan
// will receive notification once the subscribe operation completes.
//
// It is recommended to subscribe to messages published to a specific
// destination by creating a content matcher with the string field "_dest":
//
//     conn.Subscribe("{\"_dest\": \"sample\"}", "", messageChan)
//
func (conn *Connection) SubscribeAsync(matcher string, durable string, messageChan chan Message, subscriptionChan chan *Subscription) error {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if !conn.connected {
		return ErrNotConnected
	}
	// register the subscription
	conn.subSeqNum++
	sid := strconv.FormatInt(conn.subSeqNum, 10)
	sub := &Subscription{
		Matcher:          matcher,
		Durable:          durable,
		MessageChan:      messageChan,
		subscriptionID:   sid,
		subscriptionChan: subscriptionChan,
	}
	conn.subs[sid] = sub
	// send subscribe protocol
	return conn.sendMessage(sub.toProtocol())
}

// Subscribe registers interest in application messages.
// A content matcher can be used to register interest in certain messages.
// A durable name can be specified to create a durable subscription.
// Messages are received on the messageChan.
//
func (conn *Connection) SubscribeWithOptions(matcher string, durable string, options SubscriptionOptions, messageChan chan Message) (*Subscription, error) {
	subscriptionChan := make(chan *Subscription, 1)
	if err := conn.SubscribeWithOptionsAsync(matcher, durable, options, messageChan, subscriptionChan); err != nil {
		return nil, err
	}
	select {
	case sub := <-subscriptionChan:
		return sub, sub.Error
	case <-time.After(conn.Options.Timeout):
		return nil, ErrTimeout
	}
}

// Subscribe registers interest in application messages asynchronously.
// A content matcher can be used to register interest in certain messages.
// A durable name can be specified to create a durable subscription.
// Messages are received on the messageChan. The subscriptionChan
// will receive notification once the subscribe operation completes.
//
func (conn *Connection) SubscribeWithOptionsAsync(matcher string, durable string, options SubscriptionOptions, messageChan chan Message, subscriptionChan chan *Subscription) error {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if !conn.connected {
		return ErrNotConnected
	}
	// register the subscription
	conn.subSeqNum++
	sid := strconv.FormatInt(conn.subSeqNum, 10)
	sub := &Subscription{
		Matcher:          matcher,
		Durable:          durable,
		Options:          options,
		MessageChan:      messageChan,
		subscriptionID:   sid,
		subscriptionChan: subscriptionChan,
	}
	conn.subs[sid] = sub
	// send subscribe protocol
	return conn.sendMessage(sub.toProtocol())
}

// Unsubscribe unregisters the subscription.
func (conn *Connection) Unsubscribe(sub *Subscription) error {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if !conn.connected {
		return ErrNotConnected
	}
	// send unsubscribe protocol
	conn.sendMessage(Message{
		"op": opUnsubscribe,
		"id": sub.subscriptionID,
	})
	// unregister the subscription
	delete(conn.subs, sub.subscriptionID)
	return nil
}

// UnsubscribeAll unregisters all subscriptions.
func (conn *Connection) UnsubscribeAll() error {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if !conn.connected {
		return ErrNotConnected
	}
	for _, sub := range conn.subs {
		// send unsubscribe protocol
		conn.sendMessage(Message{
			"op": opUnsubscribe,
			"id": sub.subscriptionID,
		})
		// unregister the subscription
		delete(conn.subs, sub.subscriptionID)
	}
	return nil
}

func (conn *Connection) connect() error {
	// create websocket connection
	d := &websocket.Dialer{
		HandshakeTimeout: conn.Options.HandshakeTimeout,
		Subprotocols:     []string{subprotocol},
		TLSClientConfig:  conn.Options.TLSConfig,
	}
	u := &url.URL{
		Scheme: conn.URL.Scheme,
		Host:   conn.URL.Host,
		Path:   conn.URL.Path,
	}
	ws, resp, err := d.Dial(u.String(), nil)
	if err == websocket.ErrBadHandshake {
		if resp.StatusCode == http.StatusNotFound {
			return ErrNotFound
		} else {
			return ErrBadHandshake
		}
	} else if err != nil {
		return err
	}
	conn.ws = ws
	// send login message
	msg := Message{
		"op":             opLogin,
		"client_type":    "golang",
		"client_version": Version,
		"login_options": Message{
			"_qos":    "true",
			"_resume": "true",
		},
	}
	if conn.URL.User != nil {
		msg["user"] = conn.URL.User.Username()
	} else if conn.Options.Username != "" {
		msg["user"] = conn.Options.Username
	}
	if conn.URL.User != nil {
		msg["password"], _ = conn.URL.User.Password()
	} else if conn.Options.Password != "" {
		msg["password"] = conn.Options.Password
	}
	if conn.URL.Query().Get("clientId") != "" {
		msg["client_id"] = conn.URL.Query().Get("clientId")
	} else if conn.Options.ClientID != "" {
		msg["client_id"] = conn.Options.ClientID
	}
	if conn.reconnectID != "" {
		msg["id_token"] = conn.reconnectID
	}
	err = conn.sendMessage(msg)
	if err != nil {
		conn.ws.Close()
		return err
	}
	// set a read deadline
	conn.ws.SetReadDeadline(time.Now().Add(conn.Options.Timeout))
	defer conn.ws.SetReadDeadline(time.Time{})
	// receive welcome message
	msg, err = conn.nextMessage()
	if err != nil {
		conn.ws.Close()
		return err
	}
	// op code
	if op, ok := msg["op"].(int64); !ok || op != opWelcome {
		conn.ws.Close()
		return ErrInvalidResponse
	}
	// client id
	if val, ok := msg["client_id"].(string); ok {
		conn.Options.ClientID = val
	}
	// token id
	if val, ok := msg["id_token"].(string); ok {
		conn.reconnectID = val
	}
	// resume
	resume := false
	if val, ok := msg["_resume"].(string); ok {
		resume, _ = strconv.ParseBool(val)
	}
	// mark the connection as connected
	conn.connected = true
	// reset the reconnect attempts
	conn.reconnectAttempts = 0
	// re-establish subscriptions
	for _, sub := range conn.subs {
		conn.sendMessage(sub.toProtocol())
	}
	if resume {
		// re-send unacknowledged messages
		conn.reqs.iterate(func(comp *Completion) {
			conn.sendMessage(comp.request)
		})
	} else {
		conn.lastSeqNum = 0
	}
	// process incoming messages
	conn.wg.Add(1)
	go conn.dispatch()
	return nil
}

func (conn *Connection) disconnect() error {
	// mark the connection as not connected
	conn.connected = false
	// disconnect from the server
	return conn.ws.Close()
}

func (conn *Connection) dispatch() {
	defer conn.wg.Done()
	for {
		// read the next message
		msg, err := conn.nextMessage()
		if err != nil {
			// only unexpected errors will trigger a reconnect
			if _, ok := err.(*eftlError); ok {
				conn.handleDisconnect(err)
			} else {
				conn.handleReconnect(err)
			}
			break
		}
		// process the message
		if op, ok := msg["op"].(int64); ok {
			switch op {
			case opHeartbeat:
				conn.handleHeartbeat(msg)
			case opEvent:
				conn.handleMessage(msg)
			case opSubscribed:
				conn.handleSubscribed(msg)
			case opUnsubscribed:
				conn.handleUnsubscribed(msg)
			case opAck:
				conn.handleAck(msg)
			case opError:
				conn.handleError(msg)
			case opMapResponse:
				conn.handleMapResponse(msg)
			}
		}
	}
}

func (conn *Connection) handleReconnect(err error) {
	if conn.reconnectAttempts < conn.Options.AutoReconnectAttempts {
		// exponential backoff truncated to max delay
		dur := time.Duration(math.Pow(2.0, float64(conn.reconnectAttempts))) * time.Second
		if dur > conn.Options.AutoReconnectMaxDelay {
			dur = conn.Options.AutoReconnectMaxDelay
		}
		conn.reconnectAttempts++
		conn.reconnectTimer = time.AfterFunc(dur, func() {
			if e := conn.connect(); e != nil {
				conn.handleReconnect(err)
			}
		})
	} else {
		conn.handleDisconnect(err)
	}
}

func (conn *Connection) handleDisconnect(err error) {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if conn.connected {
		conn.disconnect()
		// send notification to the error channel
		if conn.ErrorChan != nil {
			conn.ErrorChan <- err
		}
		// clear pending completions
		conn.reqs.iterate(func(comp *Completion) {
			comp.Error = err
			if comp.completionChan != nil {
				comp.completionChan <- comp
			}
		})
		conn.reqs = make(requests)
	}
}

func (conn *Connection) handleHeartbeat(msg Message) {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	conn.sendMessage(msg)
}

func (conn *Connection) handleMessage(msg Message) {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	seq, _ := msg["seq"].(int64)
	body, _ := msg["body"].(Message)
	if sid, ok := msg["to"].(string); ok {
		if seq == 0 || seq > conn.lastSeqNum {
			if sub, ok := conn.subs[sid]; ok {
				if sub.MessageChan != nil {
					sub.MessageChan <- body
				}
			}
			if seq > 0 {
				conn.lastSeqNum = seq
			}
		}
	}
	if seq > 0 {
		// acknowledge message receipt
		conn.sendMessage(Message{
			"op":  opAck,
			"seq": seq,
		})
	}
}

func (conn *Connection) handleSubscribed(msg Message) {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if sid, ok := msg["id"].(string); ok {
		if sub, ok := conn.subs[sid]; ok {
			if sub.subscriptionChan != nil {
				sub.subscriptionChan <- sub
			}
		}
	}
}

func (conn *Connection) handleUnsubscribed(msg Message) {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if sid, ok := msg["id"].(string); ok {
		if sub, ok := conn.subs[sid]; ok {
			errCode, _ := msg["err"].(int64)
			if errCode == 12 {
				sub.Error = ErrNotAuthorized
			} else {
				reason, _ := msg["reason"].(string)
				sub.Error = fmt.Errorf("%d: %s", errCode, reason)
			}
			delete(conn.subs, sid)
			if sub.subscriptionChan != nil {
				sub.subscriptionChan <- sub
			}
		}
	}
}

func (conn *Connection) handleAck(msg Message) {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if seq, ok := msg["seq"].(int64); ok {
		var err error
		if errCode, ok := msg["err"].(int64); ok {
			if errCode == 12 {
				err = ErrNotAuthorized
			} else {
				reason, _ := msg["reason"].(string)
				err = fmt.Errorf("%d: %s", errCode, reason)
			}
		}
		if comp, ok := conn.reqs[seq]; ok {
			comp.Error = err
			if comp.completionChan != nil {
				comp.completionChan <- comp
			}
			delete(conn.reqs, seq)
		}
	}
}

func (conn *Connection) handleError(msg Message) {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	errCode, _ := msg["err"].(int64)
	reason, _ := msg["reason"].(string)
	if conn.ErrorChan != nil {
		conn.ErrorChan <- fmt.Errorf("%d: %s", errCode, reason)
	}
}

func (conn *Connection) handleMapResponse(msg Message) {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if seq, ok := msg["seq"].(int64); ok {
		var err error
		if errCode, ok := msg["err"].(int64); ok {
			if errCode == 14 {
				err = ErrNotAuthorized
			} else {
				reason, _ := msg["reason"].(string)
				err = fmt.Errorf("%d: %s", errCode, reason)
			}
		}
		if comp, ok := conn.reqs[seq]; ok {
			comp.Error = err
			if value, ok := msg["value"].(Message); ok {
				comp.Message = value
			}
			if comp.completionChan != nil {
				comp.completionChan <- comp
			}
			delete(conn.reqs, seq)
		}
	}
}

func (conn *Connection) sendMessage(msg Message) error {
	return conn.ws.WriteJSON(msg)
}

func (conn *Connection) nextMessage() (msg Message, err error) {
	msg = make(Message)
	err = conn.ws.ReadJSON(&msg)
	// translate a websocket.CloseError
	if closeErr, ok := err.(*websocket.CloseError); ok {
		switch closeErr.Code {
		case websocket.CloseAbnormalClosure:
			err = io.ErrUnexpectedEOF
		case websocket.CloseMessageTooBig:
			err = ErrMessageTooBig
		case 4000:
			err = ErrForceClose
		case 4002:
			err = ErrNotAuthenticated
		default:
			err = &eftlError{msg: closeErr.Error()}
		}
	}
	return
}

func (reqs requests) iterate(fn func(comp *Completion)) {
	var keys []int64
	for k := range reqs {
		keys = append(keys, k)
	}
	sort.Slice(keys, func(i, j int) bool { return keys[i] < keys[j] })
	for _, k := range keys {
		fn(reqs[k])
	}
}
