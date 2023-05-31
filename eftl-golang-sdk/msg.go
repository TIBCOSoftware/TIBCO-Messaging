//
// Copyright (c) 2001-2020 Cloud Software Group, Inc.
// Licensed under a BSD-style license. Refer to [LICENSE]
//

package eftl

import (
	"bytes"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"math"
	"sort"
	"strconv"
	"strings"
	"time"
)

const headerPrefix = "_eftl:"

const (
	deliveryCountHeader  = headerPrefix + "deliveryCount"
	storeMessageIdHeader = headerPrefix + "storeMessageId"
	sequenceNumberHeader = headerPrefix + "sequenceNumber"
	subscriptionIdHeader = headerPrefix + "subscriptionId"
	replyToHeader        = headerPrefix + "replyTo"
	requestIdHeader      = headerPrefix + "requestId"
)

// Message field name identifying the EMS destination of a message.
//
// The destination message field is only required when communicating with
// EMS.
//
// To publish a message on a specific EMS destination include this
// message field name in the published message.
//  conn.Publish(eftl.Message{eftl.FieldNameDestination: "MyDest", "text": "hello"})
//
// To subscribe to messages published on a specific EMS destination
// use a subscription matcher that includes this message field name.
//  conn.Subscribe("{\"" + eftl.FieldNameDestination + "\": \"MyDest\"}", "", msgCh)
//
// To distinguish between topics and queues the destination name
// can be prefixed with either "TOPIC:" or "QUEUE:", for example
// "TOPIC:MyDest" or "QUEUE:MyDest". A destination name with no prefix
// is a topic.
const FieldNameDestination = "_dest"

func (msg Message) String() string {
	keys := make([]string, 0, len(msg))
	for k := range msg {
		keys = append(keys, k)
	}
	sort.Strings(keys)
	b := new(bytes.Buffer)
	b.WriteString("{")
	for _, k := range keys {
		if strings.HasPrefix(k, headerPrefix) {
			continue
		}
		if b.Len() > 1 {
			b.WriteString(", ")
		}
		v := msg[k]
		switch v := v.(type) {
		default:
			fmt.Fprintf(b, "%s=%v", k, v)
		case string:
			fmt.Fprintf(b, "%s=\"%s\"", k, v)
		}
	}
	b.WriteString("}")
	return b.String()
}

// Message represents application messages that map field names to values.
//
// To create a message, use make:
//  msg := make(eftl.Message)
// Setting a message field value:
//  msg["myFieldName"] = "my field value"
// Getting a message field value:
//  val := msg["myFieldName"]
// Removing a message field value:
//  delete(msg, "myFieldName")
// The following field types are supported:
//  - string, []string
//  - int64 , []int64
//  - float64 , []float64
//  - time.Time, []time.Time
//  - eftl.Message, []eftl.Message
//  - []byte
//
// bool and nil field values are not supported.
type Message map[string]interface{}

// Unique store identifier assigned by the persistence service.
func (msg Message) StoreMessageId() int64 {
	msgId, _ := msg[storeMessageIdHeader].(int64)
	return msgId
}

// Delivery count assigned by the persistence service.
func (msg Message) DeliveryCount() int64 {
	deliveryCount, _ := msg[deliveryCountHeader].(int64)
	return deliveryCount
}

// MarshalJSON encodes the message into JSON.
func (msg Message) MarshalJSON() ([]byte, error) {
	m, err := msg.encode()
	if err != nil {
		return nil, err
	}
	return json.Marshal(m)
}

// UnmarshalJSON decodes the message from JSON.
func (msg Message) UnmarshalJSON(b []byte) error {
	m := make(map[string]interface{})
	if err := json.Unmarshal(b, &m); err != nil {
		return err
	}
	msg.decode(m)
	return nil
}

func (msg Message) encode() (map[string]interface{}, error) {
	m := make(map[string]interface{})
	for k, v := range msg {
		if strings.HasPrefix(k, headerPrefix) {
			continue
		}
		switch v := v.(type) {
		default:
			return nil, fmt.Errorf("unsupported type for field '%s'", k)
		case string:
			m[k] = v
		case []string:
			m[k] = v
		case int, int8, int16, int32, int64:
			m[k] = v
		case []int, []int8, []int16, []int32, []int64:
			m[k] = v
		case uint, uint8, uint16, uint32, uint64:
			m[k] = v
		case []uint, []uint16, []uint32, []uint64:
			m[k] = v
		case []byte:
			m[k] = map[string]string{"_o_": base64.StdEncoding.EncodeToString(v)}
		case float32:
			m[k] = map[string]float32{"_d_": v}
		case []float32:
			s := make([]map[string]float32, 0, len(v))
			for _, t := range v {
				s = append(s, map[string]float32{"_d_": t})
			}
			m[k] = s
		case float64:
			if math.IsNaN(v) || math.IsInf(v, 0) {
				m[k] = map[string]string{"_d_": fmt.Sprint(v)}
			} else {
				m[k] = map[string]float64{"_d_": v}
			}
		case []float64:
			s := make([]interface{}, 0, len(v))
			for _, t := range v {
				if math.IsNaN(t) || math.IsInf(t, 0) {
					s = append(s, map[string]string{"_d_": fmt.Sprint(t)})
				} else {
					s = append(s, map[string]float64{"_d_": t})
				}
			}
			m[k] = s
		case time.Time:
			m[k] = map[string]int64{"_m_": v.UnixNano() / 1000000}
		case []time.Time:
			s := make([]map[string]int64, 0, len(v))
			for _, t := range v {
				s = append(s, map[string]int64{"_m_": t.UnixNano() / 1000000})
			}
			m[k] = s
		case map[string]interface{}:
			enc, err := Message(v).encode()
			if err != nil {
				return nil, err
			}
			m[k] = enc
		case []map[string]interface{}:
			s := make([]map[string]interface{}, 0, len(v))
			for _, t := range v {
				enc, err := Message(t).encode()
				if err != nil {
					return nil, err
				}
				s = append(s, enc)
			}
			m[k] = s
		case Message:
			enc, err := v.encode()
			if err != nil {
				return nil, err
			}
			m[k] = enc
		case []Message:
			s := make([]map[string]interface{}, 0, len(v))
			for _, t := range v {
				enc, err := t.encode()
				if err != nil {
					return nil, err
				}
				s = append(s, enc)
			}
			m[k] = s
		case bool:
			if v {
				m[k] = 1
			} else {
				m[k] = 0
			}
		case nil:
			// skip
		}
	}
	return m, nil
}

func (msg Message) decode(m map[string]interface{}) Message {
	for k, v := range m {
		switch v := v.(type) {
		default:
			msg[k] = v
		case float64:
			msg[k] = int64(v)
		case []interface{}:
			if len(v) > 0 {
				switch t := v[0].(type) {
				case float64:
					s := make([]int64, 0, len(v))
					for _, elem := range v {
						i, _ := elem.(float64)
						s = append(s, int64(i))
					}
					msg[k] = s
				case string:
					s := make([]string, 0, len(v))
					for _, elem := range v {
						i, _ := elem.(string)
						s = append(s, i)
					}
					msg[k] = s
				case map[string]interface{}:
					if _, exists := t["_o_"].(string); exists {
						s := make([][]byte, 0, len(v))
						for _, elem := range v {
							if el, ok := elem.(map[string]interface{}); ok {
								i, _ := el["_o_"].(string)
								d, _ := base64.StdEncoding.DecodeString(i)
								s = append(s, d)
							}
						}
						msg[k] = s
					} else if _, exists := t["_d_"].(float64); exists {
						s := make([]float64, 0, len(v))
						for _, elem := range v {
							if el, ok := elem.(map[string]interface{}); ok {
								d, _ := el["_d_"].(float64)
								s = append(s, d)
							}
						}
						msg[k] = s
					} else if _, exists := t["_m_"].(float64); exists {
						s := make([]time.Time, 0, len(v))
						for _, elem := range v {
							if el, ok := elem.(map[string]interface{}); ok {
								d, _ := el["_m_"].(float64)
								s = append(s, time.Unix(0, int64(d)*int64(time.Millisecond)))
							}
						}
						msg[k] = s
					} else {
						s := make([]Message, 0, len(v))
						for _, elem := range v {
							if el, ok := elem.(map[string]interface{}); ok {
								s = append(s, make(Message).decode(el))
							}
						}
						msg[k] = s
					}
				}
			}
		case map[string]interface{}:
			if t, exists := v["_o_"].(string); exists {
				msg[k], _ = base64.StdEncoding.DecodeString(t)
			} else if t, exists := v["_d_"].(float64); exists {
				msg[k] = t
			} else if t, exists := v["_d_"].(string); exists {
				msg[k], _ = strconv.ParseFloat(t, 64)
			} else if t, exists := v["_m_"].(float64); exists {
				msg[k] = time.Unix(0, int64(t)*int64(time.Millisecond))
			} else {
				msg[k] = make(Message).decode(v)
			}
		}
	}
	return msg
}
