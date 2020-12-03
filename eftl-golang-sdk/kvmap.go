//
// Copyright (c) 2001-$Date: 2020-10-06 09:36:16 -0700 (Tue, 06 Oct 2020) $ TIBCO Software Inc.
// Licensed under a BSD-style license. Refer to [LICENSE]
// For more information, please contact:
// TIBCO Software Inc., Palo Alto, California, USA
//

package eftl

import (
	"time"
)

// Key-value map allows setting, getting, and removing key-value pairs.
type KVMap struct {
	name string
	conn *Connection
}

// Create a Key-Value map.
func (conn *Connection) KVMap(name string) *KVMap {
	return &KVMap{
		name: name,
		conn: conn,
	}
}

// Remove a Key-Value map.
func (conn *Connection) RemoveKVMap(name string) error {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if !conn.isConnected() {
		return ErrNotConnected
	}
	conn.sendMessage(Message{
		"op":  opMapDestroy,
		"map": name,
	})
	return nil
}

// Set a key-value pair in the map, overwriting any existing value.
func (m *KVMap) Set(key string, msg Message) error {
	completionChan := make(chan *Completion, 1)
	if err := m.SetAsync(key, msg, completionChan); err != nil {
		return err
	}
	select {
	case completion := <-completionChan:
		return completion.Error
	case <-time.After(m.conn.Options.Timeout):
		return ErrTimeout
	}
}

func (m *KVMap) SetAsync(key string, msg Message, completionChan chan *Completion) error {
	conn := m.conn
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if !conn.isConnected() {
		return ErrNotConnected
	}
	// register the request
	conn.reqSeqNum++
	conn.reqs[conn.reqSeqNum] = &Completion{
		Message: msg,
		request: Message{
			"op":    opMapSet,
			"seq":   conn.reqSeqNum,
			"map":   m.name,
			"key":   key,
			"value": msg,
		},
		seqNum:         conn.reqSeqNum,
		completionChan: completionChan,
	}
	// send request message
	return conn.sendMessage(conn.reqs[conn.reqSeqNum].request)
}

// Get a key-value pair from the map.
func (m *KVMap) Get(key string) (Message, error) {
	completionChan := make(chan *Completion, 1)
	if err := m.GetAsync(key, completionChan); err != nil {
		return nil, err
	}
	select {
	case completion := <-completionChan:
		return completion.Message, completion.Error
	case <-time.After(m.conn.Options.Timeout):
		return nil, ErrTimeout
	}
}

func (m *KVMap) GetAsync(key string, completionChan chan *Completion) error {
	conn := m.conn
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if !conn.isConnected() {
		return ErrNotConnected
	}
	// register the request
	conn.reqSeqNum++
	conn.reqs[conn.reqSeqNum] = &Completion{
		request: Message{
			"op":  opMapGet,
			"seq": conn.reqSeqNum,
			"map": m.name,
			"key": key,
		},
		seqNum:         conn.reqSeqNum,
		completionChan: completionChan,
	}
	// send request message
	return conn.sendMessage(conn.reqs[conn.reqSeqNum].request)
}

// Remove a key-value pair from the map.
func (m *KVMap) Remove(key string) error {
	completionChan := make(chan *Completion, 1)
	if err := m.RemoveAsync(key, completionChan); err != nil {
		return err
	}
	select {
	case completion := <-completionChan:
		return completion.Error
	case <-time.After(m.conn.Options.Timeout):
		return ErrTimeout
	}
}

func (m *KVMap) RemoveAsync(key string, completionChan chan *Completion) error {
	conn := m.conn
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if !conn.isConnected() {
		return ErrNotConnected
	}
	// register the request
	conn.reqSeqNum++
	conn.reqs[conn.reqSeqNum] = &Completion{
		request: Message{
			"op":  opMapRemove,
			"seq": conn.reqSeqNum,
			"map": m.name,
			"key": key,
		},
		seqNum:         conn.reqSeqNum,
		completionChan: completionChan,
	}
	// send request message
	return conn.sendMessage(conn.reqs[conn.reqSeqNum].request)
}
