//
// Copyright (c) 2001-$Date: 2020-06-10 10:59:09 -0700 (Wed, 10 Jun 2020) $ TIBCO Software Inc.
// Licensed under a BSD-style license. Refer to [LICENSE]
// For more information, please contact:
// TIBCO Software Inc., Palo Alto, California, USA
//

package eftl_test

import (
	"fmt"
	"time"

	"github.com/TIBCOSoftware/TIBCO-Messaging/eftl/sdk/golang/eftl"
)

// Connect to the server.
func ExampleConnect() {
	errChan := make(chan error)

	opts := eftl.DefaultOptions()
	opts.Username = "username"
	opts.Password = "password"

	// connect to the server
	conn, err := eftl.Connect("ws://localhost:9191/channel", opts, errChan)
	if err != nil {
		fmt.Println("connect failed:", err)
		return
	}

	// disconnect from the server when done with the connection
	defer conn.Disconnect()

	// listen for asynnchronous errors
	go func() {
		for err := range errChan {
			fmt.Println("connection error:", err)
		}
	}()
	// Output:
}

// Reconnect to the server.
func ExampleConnection_Reconnect() {
	// connect to the server
	conn, err := eftl.Connect("ws://localhost:9191/channel", nil, nil)
	if err != nil {
		fmt.Println("connect failed:", err)
		return
	}

	// disconnect from the server when done with the connection
	defer conn.Disconnect()

	// disconnect from the server
	conn.Disconnect()

	// reconnect to the server
	err = conn.Reconnect()
	if err != nil {
		fmt.Println("reconnect failed:", err)
	}
	// Output:
}

// Publish messages.
func ExampleConnection_Publish() {
	// connect to the server
	conn, err := eftl.Connect("ws://localhost:9191/channel", nil, nil)
	if err != nil {
		fmt.Println("connect failed:", err)
		return
	}

	// disconnect from the server when done with the connection
	defer conn.Disconnect()

	// publish a message
	err = conn.Publish(eftl.Message{
		"type":  "hello",
		"long":  99,
		"float": 0.99,
		"time":  time.Now(),
		"message": eftl.Message{
			"bytes": []byte("this is an embedded message"),
		},
	})
	if err != nil {
		fmt.Println("publish failed:", err)
	}
	// Output:
}

// Publish messages asynchronously.
func ExampleConnection_PublishAsync() {
	// connect to the server
	conn, err := eftl.Connect("ws://localhost:9191/channel", nil, nil)
	if err != nil {
		fmt.Println("connect failed:", err)
		return
	}

	// disconnect from the server when done with the connection
	defer conn.Disconnect()

	// create a completion channel
	compChan := make(chan *eftl.Completion, 1)

	// publish a message
	err = conn.PublishAsync(eftl.Message{
		"type":  "hello",
		"long":  99,
		"float": 0.99,
		"time":  time.Now(),
		"message": eftl.Message{
			"bytes": []byte("this is an embedded message"),
		},
	}, compChan)
	if err != nil {
		fmt.Println("publish failed:", err)
		return
	}

	// wait for publish operation to complete
	select {
	case comp := <-compChan:
		if comp.Error != nil {
			fmt.Println("publish completion failed:", err)
		}
	case <-time.After(5 * time.Second):
		fmt.Println("publish completion failed: timeout")
	}
	// Output:
}

// Subscribe to messages.
func ExampleConnection_Subscribe() {
	errChan := make(chan error)

	// connect to the server
	conn, err := eftl.Connect("ws://localhost:9191/channel", nil, errChan)
	if err != nil {
		fmt.Println("connect failed:", err)
		return
	}

	// disconnect from the server when done with the connection
	defer conn.Disconnect()

	// create a message channel
	msgChan := make(chan eftl.Message)

	// subscribe to messages
	sub, err := conn.Subscribe("{\"type\": \"hello\"}", "", msgChan)
	if err != nil {
		fmt.Println("subscribe failed:", err)
		return
	}

	// unsubscribe when done
	defer conn.Unsubscribe(sub)

	// receive messages
	for {
		select {
		case msg := <-msgChan:
			fmt.Println(msg)
		case err := <-errChan:
			fmt.Println("connection error:", err)
			return
		}
	}
}

// Subscribe to messages asynchronously.
func ExampleConnection_SubscribeAsync() {
	errChan := make(chan error)

	// connect to the server
	conn, err := eftl.Connect("ws://localhost:9191/channel", nil, errChan)
	if err != nil {
		fmt.Println("connect failed:", err)
		return
	}

	// disconnect from the server when done with the connection
	defer conn.Disconnect()

	// create a subscription and message channel
	subChan := make(chan *eftl.Subscription)
	msgChan := make(chan eftl.Message)

	// subscribe to messages
	err = conn.SubscribeAsync("{\"type\": \"hello\"}", "", msgChan, subChan)
	if err != nil {
		fmt.Println("subscribe failed:", err)
		return
	}

	// wait for subsribe operation to complete and receive messages
	for {
		select {
		case sub := <-subChan:
			if sub.Error != nil {
				fmt.Println("subscription failed:", sub.Error)
				return
			}
			// unsubscribe when done
			defer conn.Unsubscribe(sub)
		case err := <-errChan:
			fmt.Println("connection error:", err)
			return
		case msg := <-msgChan:
			fmt.Println(msg)
		}
	}
}

// Publish request messages and wait for a reply.
func ExampleConnection_SendRequest() {
	// connect to the server
	conn, err := eftl.Connect("ws://localhost:9191/channel", nil, nil)
	if err != nil {
		fmt.Println("connect failed:", err)
		return
	}

	// disconnect from the server when done with the connection
	defer conn.Disconnect()

	// publish a request message and wait for a reply.
	reply, err := conn.SendRequest(eftl.Message{
		"type": "request",
		"text": "this is a request message",
	}, 10*time.Second)
	if err == eftl.ErrTimeout {
		fmt.Println("request timeout")
	} else if err != nil {
		fmt.Println("request failed:", err)
	} else {
		fmt.Println("received reply:", reply)
	}
	// Output:
}

// Publish request messages asynchronously and receive a reply.
func ExampleConnection_SendRequestAsync() {
	// connect to the server
	conn, err := eftl.Connect("ws://localhost:9191/channel", nil, nil)
	if err != nil {
		fmt.Println("connect failed:", err)
		return
	}

	// disconnect from the server when done with the connection
	defer conn.Disconnect()

	// create a completion channel
	compChan := make(chan *eftl.Completion, 1)

	// publish a request message
	err = conn.SendRequestAsync(eftl.Message{
		"type": "request",
		"text": "this is a request message",
	}, compChan)
	if err != nil {
		fmt.Println("request failed:", err)
		return
	}

	// wait for request to complete
	select {
	case comp := <-compChan:
		if comp.Error != nil {
			fmt.Println("request failed:", err)
		} else {
			fmt.Println("received reply:", comp.Message)
		}
	case <-time.After(10 * time.Second):
		fmt.Println("request failed: timeout")
	}
	// Output:
}

// Set a key-value pair in a map.
func ExampleKVMap_Set() {
	// connect to the server
	conn, err := eftl.Connect("ws://localhost:9191/channel", nil, nil)
	if err != nil {
		fmt.Println("connect failed:", err)
		return
	}

	// disconnect from the server when done with the connection
	defer conn.Disconnect()

	// set a key-value pair in a map
	err = conn.KVMap("MyMap").Set("MyKey", eftl.Message{
		"text": "sample text",
	})
	if err != nil {
		fmt.Println("key-value set failed:", err)
	}
	// Output:
}

// Set a key-value pair in a map asynchronously.
func ExampleKVMap_SetAsync() {
	// connect to the server
	conn, err := eftl.Connect("ws://localhost:9191/channel", nil, nil)
	if err != nil {
		fmt.Println("connect failed:", err)
		return
	}

	// disconnect from the server when done with the connection
	defer conn.Disconnect()

	// create a completion channel
	compChan := make(chan *eftl.Completion, 1)

	// set a key-value pair in a map
	err = conn.KVMap("MyMap").SetAsync("MyKey", eftl.Message{
		"text": "sample text",
	}, compChan)
	if err != nil {
		fmt.Println("key-value set failed:", err)
		return
	}

	// wait for key-value map operation to complete
	select {
	case comp := <-compChan:
		if comp.Error != nil {
			fmt.Println("key-value set completion failed:", err)
		}
	case <-time.After(5 * time.Second):
		fmt.Println("key-value set completion failed: timeout")
	}
	// Output:
}

// Get a key-value pair from a map.
func ExampleKVMap_Get() {
	// connect to the server
	conn, err := eftl.Connect("ws://localhost:9191/channel", nil, nil)
	if err != nil {
		fmt.Println("connect failed:", err)
		return
	}

	// disconnect from the server when done with the connection
	defer conn.Disconnect()

	// get a key-value pair from a map
	if msg, err := conn.KVMap("MyMap").Get("MyKey"); err != nil {
		fmt.Println("key-value get failed:", err)
	} else if msg == nil {
		fmt.Println("key-value get: not found")
	} else {
		fmt.Println("key-value get:", msg["text"])
	}
	// Output:
	// key-value get: sample text
}

// Get a key-value pair from a map asynchronously.
func ExampleKVMap_GetAsync() {
	// connect to the server
	conn, err := eftl.Connect("ws://localhost:9191/channel", nil, nil)
	if err != nil {
		fmt.Println("connect failed:", err)
		return
	}

	// disconnect from the server when done with the connection
	defer conn.Disconnect()

	// create a completion channel
	compChan := make(chan *eftl.Completion, 1)

	// get a key-value pair from a map
	err = conn.KVMap("MyMap").GetAsync("MyKey", compChan)
	if err != nil {
		fmt.Println("key-value get failed:", err)
		return
	}

	// wait for key-value map operation to complete
	select {
	case comp := <-compChan:
		if comp.Error != nil {
			fmt.Println("key-value get completion failed:", err)
		} else if comp.Message == nil {
			fmt.Println("key-value get: not found")
		} else {
			fmt.Println("key-value get:", comp.Message["text"])
		}
	case <-time.After(5 * time.Second):
		fmt.Println("key-value get completion failed: timeout")
	}
	// Output:
	// key-value get: sample text
}

// Remove a key-value pair from a map.
func ExampleKVMap_Remove() {
	// connect to the server
	conn, err := eftl.Connect("ws://localhost:9191/channel", nil, nil)
	if err != nil {
		fmt.Println("connect failed:", err)
		return
	}

	// disconnect from the server when done with the connection
	defer conn.Disconnect()

	// remove a key-value pair from a map
	if err := conn.KVMap("MyMap").Remove("MyKey"); err != nil {
		fmt.Println("key-value reomve failed:", err)
	}
	// Output:
}

// Remove a key-value pair from a map asynchronously.
func ExampleKVMap_RemoveAsync() {
	// connect to the server
	conn, err := eftl.Connect("ws://localhost:9191/channel", nil, nil)
	if err != nil {
		fmt.Println("connect failed:", err)
		return
	}

	// disconnect from the server when done with the connection
	defer conn.Disconnect()

	// create a completion channel
	compChan := make(chan *eftl.Completion, 1)

	// remove a key-value pair from a map
	err = conn.KVMap("MyMap").RemoveAsync("MyKey", compChan)
	if err != nil {
		fmt.Println("key-value remove failed:", err)
		return
	}

	// wait for key-value map operation to complete
	select {
	case comp := <-compChan:
		if comp.Error != nil {
			fmt.Println("key-value remove completion failed:", err)
		}
	case <-time.After(5 * time.Second):
		fmt.Println("key-value remove completion failed: timeout")
	}
	// Output:
}
