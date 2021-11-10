//
// Copyright (c) 2001-2021 TIBCO Software Inc.
// Licensed under a BSD-style license. Refer to [LICENSE]
// For more information, please contact:
// TIBCO Software Inc., Palo Alto, California, USA
//

package eftl

import (
	"crypto/tls"
	"fmt"
	"math"
	"math/rand"
	"net/http"
	"net/url"
	"sort"
	"strconv"
	"strings"
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
	ErrGoingAway        = &eftlError{msg: "server going away"}
	ErrMessageTooBig    = &eftlError{msg: "message too big"}
	ErrNotAuthenticated = &eftlError{msg: "not authenticated"}
	ErrForceClose       = &eftlError{msg: "server has forcibly closed the connection"}
	ErrNotAuthorized    = &eftlError{msg: "not authorized for the operation"}
	ErrBadHandshake     = &eftlError{msg: "bad handshake"}
	ErrNotFound         = &eftlError{msg: "not found"}
	ErrRestart          = &eftlError{msg: "server restart"}
	ErrReconnect        = &eftlError{msg: "reconnect"}
	ErrNotRequest       = &eftlError{msg: "not a request message"}
	ErrNotSupported     = &eftlError{msg: "not supported with this server"}
)

// Error codes
const (
	ErrCodePublishDisallowed      = 12
	ErrCodePublishFailed          = 11
	ErrCodeSubscriptionDisallowed = 13
	ErrCodeSubscriptionFailed     = 21
	ErrCodeSubscriptionInvalid    = 22
	ErrCodeMapRequestDisallowed   = 14
	ErrCodeMapRequestFailed       = 30
	ErrCodeRequestDisallowed      = 40
	ErrCodeRequestFailed          = 41
)

// State of the connection.
type State int

const (
	DISCONNECTED = State(iota)
	CONNECTING
	CONNECTED
	DISCONNECTING
	RECONNECTING
)

func (e State) String() string {
	switch e {
	case DISCONNECTED:
		return "disconnected"
	case CONNECTING:
		return "connecting"
	case CONNECTED:
		return "connected"
	case DISCONNECTING:
		return "disconnecting"
	case RECONNECTING:
		return "reconnecting"
	default:
		return fmt.Sprintf("%d", int(e))
	}
}

// StateChangeHandler is invoked whenever the connection state changes.
type StateChangeHandler func(*Connection, State)

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
	// server to complete. The default is 60 seconds.
	Timeout time.Duration

	// HandshakeTimeout specifies the duration for the websocket handshake
	// with the server to complete. The default is 10 seconds.
	HandshakeTimeout time.Duration

	// AutoReconnectAttempts specifies the number of times the client attempts to
	// automatically reconnect to the server following a loss of connection.
	// The default is 256 attempts.
	AutoReconnectAttempts int64

	// AutoReconnectMaxDelay determines the maximum delay between autoreconnect attempts.
	// Upon loss of a connection, the client will delay for 1 second before attempting to
	// automatically reconnect. Subsequent attempts double the delay duration, up to
	// the maximum value specified. The default is 30 seconds.
	AutoReconnectMaxDelay time.Duration

	// MaxPendingAcks specifies the maximum number of unacknowledged messages
	// allowed for the client. Once reached the client will stop receiving
	// additional messages until previously received messages are acknowledged.
	// If not specified the server's configured value will be used.
	MaxPendingAcks int32

	// OnStateChange is invoked whenever the connection state changes.
	OnStateChange StateChangeHandler
}

// DefaultOptions returns the default connection options.
func DefaultOptions() *Options {
	return &Options{
		Timeout:               DefaultTimeout,
		HandshakeTimeout:      DefaultHandshakeTimeout,
		AutoReconnectAttempts: DefaultReconnectAttempts,
		AutoReconnectMaxDelay: DefaultReconnectMaxDelay,
	}
}

// Connection represents a connection to the server.
type Connection struct {
	URL               *url.URL
	Options           Options
	ErrorChan         chan error
	protocol          int64
	reconnectID       string
	wg                sync.WaitGroup
	mu                sync.Mutex
	ws                *websocket.Conn
	urlList           []*url.URL
	urlIndex          int
	state             State
	timeout           time.Duration
	reqs              requests
	reqSeqNum         int64
	subs              map[string]*Subscription
	subSeqNum         int64
	reconnectAttempts int64
	reconnectTimer    *time.Timer
}

type AcknowledgeMode string

const (
	AcknowledgeModeAuto   AcknowledgeMode = "auto"
	AcknowledgeModeClient                 = "client"
	AcknowledgeModeNone                   = "none"
)

const (
	DurableTypeShared    string = "shared"
	DurableTypeLastValue        = "last-value"
)

// Options available to configure a subscription.
type SubscriptionOptions struct {
	// Message acknowledgment mode; "auto", "client", or "none".
	//
	// The default message acknowledgement mode is "auto".
	//
	// Messages consumed from a subscription with an acknowledgment mode
	// of "client" require explicit acknowledgment by the client. The eFTL
	// server will stop delivering messages to the client once the
	// server's configured maximum unacknowledged messages is reached.
	AcknowledgeMode AcknowledgeMode

	// Optional durable subscription type; "shared" or "last-value".
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
	lastSeqNum       int64
	subscriptionID   string
	subscriptionChan chan *Subscription
	pending          bool
}

func (sub *Subscription) autoAck() bool {
	return sub.Options.AcknowledgeMode == "" ||
		sub.Options.AcknowledgeMode == AcknowledgeModeAuto
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
	if sub.Options.AcknowledgeMode != "" {
		msg["ack"] = string(sub.Options.AcknowledgeMode)
	}
	if sub.Options.DurableType != "" {
		msg["type"] = string(sub.Options.DurableType)
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

const protocolVer = 1

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
	opRequest      = 13
	opRequestReply = 14
	opReply        = 15
	opMapCreate    = 16
	opMapDestroy   = 18
	opMapSet       = 20
	opMapGet       = 22
	opMapRemove    = 24
	opMapResponse  = 26
)

// Default constants.
const (
	DefaultTimeout           = 60 * time.Second
	DefaultHandshakeTimeout  = 10 * time.Second
	DefaultReconnectAttempts = 256
	DefaultReconnectMaxDelay = 30 * time.Second
)

// Connect establishes a connection to the server at the specified url.
//
// When a pipe-separated list of URLs is specified this call will attempt
// a connection to each in turn, in a random order, until one is connected.
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
		opts = DefaultOptions()
	}
	urlList, err := parseURLString(urlStr)
	if err != nil {
		return nil, err
	}
	// initialize the connection
	conn := &Connection{
		Options:   *opts,
		ErrorChan: errorChan,
		reqs:      make(requests),
		subs:      make(map[string]*Subscription),
		urlList:   urlList,
	}
	// set default values
	if conn.Options.Timeout == 0 {
		conn.Options.Timeout = DefaultTimeout
	}
	if conn.Options.HandshakeTimeout == 0 {
		conn.Options.HandshakeTimeout = DefaultHandshakeTimeout
	}
	conn.mu.Lock()
	defer conn.mu.Unlock()
	// connect to the server
	conn.setState(CONNECTING)
	for _, url := range conn.urlList {
		if err = conn.connect(url); err == nil {
			return conn, nil // success
		}
	}
	conn.setState(DISCONNECTED)
	return nil, err
}

// Reconnect re-establishes the connection to the server following a
// connection error or a disconnect. Reconnect is a synchronous operation
// and will block until either a connection has been established or an
// error occurs. Upon success subscriptions are re-established.
func (conn *Connection) Reconnect() error {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if conn.isConnected() {
		return nil
	}
	var err error
	// connect to the server
	conn.setState(CONNECTING)
	for _, url := range conn.urlList {
		if err = conn.connect(url); err == nil {
			return nil // success
		}
	}
	conn.setState(DISCONNECTED)
	return err
}

// Disconnect closes the connection to the server.
func (conn *Connection) Disconnect() {
	conn.mu.Lock()
	if !conn.isConnected() {
		conn.mu.Unlock()
		return
	}
	conn.setState(DISCONNECTING)
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
	return conn.isConnected()
}

// Publish a request message and wait for a reply.
func (conn *Connection) SendRequest(request Message, timeout time.Duration) (Message, error) {
	completionChan := make(chan *Completion, 1)
	if err := conn.SendRequestAsync(request, completionChan); err != nil {
		return nil, err
	}
	select {
	case completion := <-completionChan:
		return completion.Message, completion.Error
	case <-time.After(timeout):
		return nil, ErrTimeout
	}
}

// Publish a request message asynchronously. The completionChan will
// receive notification once the reply has been received.
func (conn *Connection) SendRequestAsync(request Message, completionChan chan *Completion) error {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if !conn.isConnected() {
		return ErrNotConnected
	}
	if conn.protocol < 1 {
		return ErrNotSupported
	}
	// register the publish
	conn.reqSeqNum++
	conn.reqs[conn.reqSeqNum] = &Completion{
		request: Message{
			"op":   opRequest,
			"seq":  conn.reqSeqNum,
			"body": request,
		},
		seqNum:         conn.reqSeqNum,
		completionChan: completionChan,
	}
	// send publish message
	conn.sendMessage(conn.reqs[conn.reqSeqNum].request)
	return nil
}

// Send a reply message in response to a request message.
func (conn *Connection) SendReply(reply, request Message) error {
	completionChan := make(chan *Completion, 1)
	if err := conn.SendReplyAsync(reply, request, completionChan); err != nil {
		return err
	}
	select {
	case completion := <-completionChan:
		return completion.Error
	case <-time.After(conn.Options.Timeout):
		return ErrTimeout
	}
}

// Send a reply message asynchronously in response to a request message.
// The optional completionChan will receive notification once the send
// operation completes.
func (conn *Connection) SendReplyAsync(reply, request Message, completionChan chan *Completion) error {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if !conn.isConnected() {
		return ErrNotConnected
	}
	if conn.protocol < 1 {
		return ErrNotSupported
	}
	replyTo, ok := request[replyToHeader].(string)
	if !ok {
		return ErrNotRequest
	}
	reqId, _ := request[requestIdHeader].(int64)
	// register the publish
	conn.reqSeqNum++
	conn.reqs[conn.reqSeqNum] = &Completion{
		Message: reply,
		request: Message{
			"op":   opReply,
			"seq":  conn.reqSeqNum,
			"to":   replyTo,
			"req":  reqId,
			"body": reply,
		},
		seqNum:         conn.reqSeqNum,
		completionChan: completionChan,
	}
	// send publish message
	conn.sendMessage(conn.reqs[conn.reqSeqNum].request)
	return nil
}

// Publish an application message.
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
func (conn *Connection) PublishAsync(msg Message, completionChan chan *Completion) error {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if !conn.isConnected() {
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
	conn.sendMessage(conn.reqs[conn.reqSeqNum].request)
	return nil
}

// Subscribe registers interest in application messages.
// A content matcher can be used to register interest in certain messages.
// A durable name can be specified to create a durable subscription.
// Messages are received on the messageChan.
func (conn *Connection) Subscribe(matcher string, durable string, messageChan chan Message) (*Subscription, error) {
	return conn.SubscribeWithOptions(matcher, durable, SubscriptionOptions{}, messageChan)
}

// Subscribe registers interest in application messages asynchronously.
// A content matcher can be used to register interest in certain messages.
// A durable name can be specified to create a durable subscription.
// Messages are received on the messageChan. The subscriptionChan
// will receive notification once the subscribe operation completes.
func (conn *Connection) SubscribeAsync(matcher string, durable string, messageChan chan Message, subscriptionChan chan *Subscription) error {
	return conn.SubscribeWithOptionsAsync(matcher, durable, SubscriptionOptions{}, messageChan, subscriptionChan)
}

// Subscribe registers interest in application messages.
// A content matcher can be used to register interest in certain messages.
// A durable name can be specified to create a durable subscription.
// Messages are received on the messageChan.
//
func (conn *Connection) SubscribeWithOptions(matcher string, durable string, options SubscriptionOptions, messageChan chan Message) (*Subscription, error) {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if !conn.isConnected() {
		return nil, ErrNotConnected
	}
	subscriptionChan := make(chan *Subscription, 1)
	conn.subscribe(matcher, durable, options, messageChan, subscriptionChan)
	select {
	case sub := <-subscriptionChan:
		return sub, sub.Error
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
	if !conn.isConnected() {
		return ErrNotConnected
	}
	conn.subscribe(matcher, durable, options, messageChan, subscriptionChan)
	return nil
}

// Close the subscription. For durable subscriptions, the persistence service
// will not remove the durable subscription allowing the durable subscription
// to continue accumulating persisted messages. Any unacknowledged messages
// will be made available for redelivery.
func (conn *Connection) CloseSubscription(sub *Subscription) error {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if !conn.isConnected() {
		return ErrNotConnected
	}
	if conn.protocol < 1 {
		return ErrNotSupported
	}
	// send unsubscribe protocol
	conn.sendMessage(Message{
		"op":  opUnsubscribe,
		"id":  sub.subscriptionID,
		"del": "false",
	})
	// unregister the subscription
	delete(conn.subs, sub.subscriptionID)
	return nil
}

// Close all subscriptions. For durable subscriptions, the persistence service
// will not remove the durable subscriptions allowing the durable subscriptions
// to continue accumulating persisted messages. Any unacknowledged messages
// will be made available for redelivery.
func (conn *Connection) CloseAllSubscriptions() error {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if !conn.isConnected() {
		return ErrNotConnected
	}
	if conn.protocol < 1 {
		return ErrNotSupported
	}
	for _, sub := range conn.subs {
		// send unsubscribe protocol
		conn.sendMessage(Message{
			"op":  opUnsubscribe,
			"id":  sub.subscriptionID,
			"del": "false",
		})
		// unregister the subscription
		delete(conn.subs, sub.subscriptionID)
	}
	return nil
}

// Unsubscribe unregisters the subscription. For durable subscriptions,
// the persistence service will remove the durable subscription as well,
// along with any persisted messsages.
func (conn *Connection) Unsubscribe(sub *Subscription) error {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if !conn.isConnected() {
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

// UnsubscribeAll unregisters all subscriptions. For durable subscriptions,
// the persistence service will remove the durable subscription as well,
// along with any persisted messages.
func (conn *Connection) UnsubscribeAll() error {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if !conn.isConnected() {
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

// Acknowledge this message.
func (conn *Connection) Acknowledge(msg Message) error {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if !conn.isConnected() {
		return ErrNotConnected
	}
	seq, ok := msg[sequenceNumberHeader].(int64)
	if !ok {
		return nil
	}
	// send acknowledge protocol
	conn.sendMessage(Message{
		"op":  opAck,
		"seq": seq,
	})
	return nil
}

// Acknowledge all messages up to and including this message.
func (conn *Connection) AcknowledgeAll(msg Message) error {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if !conn.isConnected() {
		return ErrNotConnected
	}
	seq, ok := msg[sequenceNumberHeader].(int64)
	if !ok {
		return nil
	}
	sid, ok := msg[subscriptionIdHeader].(string)
	if !ok {
		return nil
	}
	// send acknowledge protocol
	conn.sendMessage(Message{
		"op":  opAck,
		"seq": seq,
		"id":  sid,
	})
	return nil
}

func (conn *Connection) setState(state State) {
	if conn.state != state {
		conn.state = state
		// optional user callback
		if conn.Options.OnStateChange != nil {
			conn.Options.OnStateChange(conn, conn.state)
		}
	}
}

func (conn *Connection) isConnected() bool {
	return conn.state == CONNECTED || conn.state == RECONNECTING
}

func (conn *Connection) subscribe(matcher string, durable string, options SubscriptionOptions, messageChan chan Message, subscriptionChan chan *Subscription) *Subscription {
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
		pending:          true,
	}
	conn.subs[sid] = sub
	// send subscribe protocol
	conn.sendMessage(sub.toProtocol())
	return sub
}

func (conn *Connection) connect(uri *url.URL) error {
	// create websocket connection
	d := &websocket.Dialer{
		HandshakeTimeout: conn.Options.HandshakeTimeout,
		Subprotocols:     []string{subprotocol},
		TLSClientConfig:  conn.Options.TLSConfig,
	}
	u := &url.URL{
		Scheme: uri.Scheme,
		Host:   uri.Host,
		Path:   uri.Path,
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
		"protocol":       protocolVer,
		"client_type":    "golang",
		"client_version": Version,
		"login_options": Message{
			"_qos":    "true",
			"_resume": "true",
		},
	}
	if uri.User != nil {
		msg["user"] = uri.User.Username()
	} else if conn.Options.Username != "" {
		msg["user"] = conn.Options.Username
	}
	if uri.User != nil {
		msg["password"], _ = uri.User.Password()
	} else if conn.Options.Password != "" {
		msg["password"] = conn.Options.Password
	}
	if uri.Query().Get("clientId") != "" {
		msg["client_id"] = uri.Query().Get("clientId")
	} else if conn.Options.ClientID != "" {
		msg["client_id"] = conn.Options.ClientID
	}
	if conn.Options.MaxPendingAcks > 0 {
		msg["max_pending_acks"] = conn.Options.MaxPendingAcks
	}
	if conn.reconnectID != "" {
		msg["id_token"] = conn.reconnectID
	}
	err = conn.sendMessage(msg)
	if err != nil {
		conn.ws.Close()
		return err
	}
	// receive welcome message
	msg, err = conn.nextMessage(conn.Options.Timeout)
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
	// protocol
	if val, ok := msg["protocol"].(int64); ok {
		conn.protocol = val
	}
	// token id
	if val, ok := msg["id_token"].(string); ok {
		conn.reconnectID = val
	}
	// timeout
	if val, ok := msg["timeout"].(int64); ok {
		conn.timeout = time.Duration(val) * time.Second
	}
	// resume
	resume := false
	if val, ok := msg["_resume"].(string); ok {
		resume, _ = strconv.ParseBool(val)
	}
	// mark the connection as connected
	conn.setState(CONNECTED)
	// reset the reconnect attempts
	conn.reconnectAttempts = 0
	// re-establish subscriptions
	for _, sub := range conn.subs {
		if !resume {
			sub.lastSeqNum = 0
		}
		conn.sendMessage(sub.toProtocol())
	}
	// re-send unacknowledged messages
	conn.reqs.iterate(func(comp *Completion) {
		conn.sendMessage(comp.request)
	})
	// process incoming messages
	conn.wg.Add(1)
	go conn.dispatch()
	return nil
}

func (conn *Connection) disconnect() error {
	// mark the connection as not connected
	conn.setState(DISCONNECTED)
	// disconnect from the server
	return conn.ws.Close()
}

func (conn *Connection) dispatch() {
	defer conn.wg.Done()
	for {
		// read the next message
		msg, err := conn.nextMessage(conn.timeout)
		if err != nil {
			conn.mu.Lock()
			if conn.isConnected() {
				conn.disconnect()
				// if a websocket close code is received from the
				// server only reconnect if the code is a server restart
				if e, ok := err.(*eftlError); ok && e != ErrRestart {
					conn.handleDisconnect(err)
				} else {
					conn.handleReconnect(err)
				}
			}
			conn.mu.Unlock()
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
			case opRequestReply:
				conn.handleReply(msg)
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
		conn.setState(RECONNECTING)
		// add jitter by applying a randomness factor of 0.5
		jitter := rand.Float64() + 0.5
		// exponential backoff truncated to max delay
		backoff := time.Duration(math.Pow(2.0, float64(conn.reconnectAttempts))*jitter) * time.Second
		if backoff > conn.Options.AutoReconnectMaxDelay || backoff <= 0 {
			backoff = conn.Options.AutoReconnectMaxDelay
		}
		conn.reconnectAttempts++
		conn.reconnectTimer = time.AfterFunc(backoff, func() {
			conn.mu.Lock()
			defer conn.mu.Unlock()
			for _, url := range conn.urlList {
				if e := conn.connect(url); e == nil {
					return
				}
			}
			conn.setState(DISCONNECTED)
			conn.handleReconnect(err)
		})
	} else {
		conn.handleDisconnect(err)
	}
}

func (conn *Connection) handleDisconnect(err error) {
	// send notification to the error channel
	if conn.ErrorChan != nil {
		conn.ErrorChan <- err
	}
	// clear pending completions
	conn.reqs.iterate(func(comp *Completion) {
		comp.Error = err
		if comp.completionChan != nil {
			select {
			case comp.completionChan <- comp:
			default:
			}
		}
	})
	conn.reqs = make(requests)
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
	sid, _ := msg["to"].(string)
	replyTo, _ := msg["reply_to"].(string)
	reqId, _ := msg["req"].(int64)
	msgId, _ := msg["sid"].(int64)
	deliveryCount, _ := msg["cnt"].(int64)
	if sub, ok := conn.subs[sid]; ok {
		if seq == 0 || seq > sub.lastSeqNum {
			if msgId != 0 {
				body[storeMessageIdHeader] = msgId
			}
			if deliveryCount != 0 {
				body[deliveryCountHeader] = deliveryCount
			}
			if !sub.autoAck() && seq != 0 {
				body[sequenceNumberHeader] = seq
				body[subscriptionIdHeader] = sid
			}
			if replyTo != "" {
				body[replyToHeader] = replyTo
				body[requestIdHeader] = reqId
			}
			if sub.MessageChan != nil {
				sub.MessageChan <- body
			}
			if sub.autoAck() && seq != 0 {
				sub.lastSeqNum = seq
			}
		}
		if sub.autoAck() && seq != 0 {
			// acknowledge message receipt
			conn.sendMessage(Message{
				"op":  opAck,
				"seq": seq,
				"id":  sid,
			})
		}
	}
}

func (conn *Connection) handleSubscribed(msg Message) {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if sid, ok := msg["id"].(string); ok {
		if sub, ok := conn.subs[sid]; ok {
			if sub.subscriptionChan != nil && sub.pending {
				sub.pending = false
				select {
				case sub.subscriptionChan <- sub:
				default:
				}
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
			if errCode == ErrCodeSubscriptionDisallowed {
				sub.Error = ErrNotAuthorized
			} else {
				reason, _ := msg["reason"].(string)
				sub.Error = fmt.Errorf("%d: %s", errCode, reason)
			}
			if errCode == ErrCodeSubscriptionInvalid {
				// remove the subscription only if it's untryable
				delete(conn.subs, sid)
			}
			sub.pending = true
			if sub.subscriptionChan != nil {
				select {
				case sub.subscriptionChan <- sub:
				default:
				}
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
			if errCode == ErrCodePublishDisallowed {
				err = ErrNotAuthorized
			} else {
				reason, _ := msg["reason"].(string)
				err = fmt.Errorf("%d: %s", errCode, reason)
			}
		}
		if comp, ok := conn.reqs[seq]; ok {
			comp.Error = err
			if comp.completionChan != nil {
				select {
				case comp.completionChan <- comp:
				default:
				}
			}
			delete(conn.reqs, seq)
		}
	}
}

func (conn *Connection) handleReply(msg Message) {
	conn.mu.Lock()
	defer conn.mu.Unlock()
	if seq, ok := msg["seq"].(int64); ok {
		var err error
		if errCode, ok := msg["err"].(int64); ok {
			if errCode == ErrCodeRequestDisallowed {
				err = ErrNotAuthorized
			} else {
				reason, _ := msg["reason"].(string)
				err = fmt.Errorf("%d: %s", errCode, reason)
			}
		}
		if comp, ok := conn.reqs[seq]; ok {
			comp.Error = err
			if body, ok := msg["body"].(Message); ok {
				comp.Message = body
			}
			if comp.completionChan != nil {
				select {
				case comp.completionChan <- comp:
				default:
				}
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
			if errCode == ErrCodeMapRequestDisallowed {
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
				select {
				case comp.completionChan <- comp:
				default:
				}
			}
			delete(conn.reqs, seq)
		}
	}
}

func (conn *Connection) sendMessage(msg Message) error {
	return conn.ws.WriteJSON(msg)
}

func (conn *Connection) nextMessage(timeout time.Duration) (msg Message, err error) {
	// set the read deadline for non-zero timeouts
	if timeout > 0 {
		conn.ws.SetReadDeadline(time.Now().Add(timeout))
	} else {
		conn.ws.SetReadDeadline(time.Time{})
	}
	// read the next message
	msg = make(Message)
	err = conn.ws.ReadJSON(&msg)
	// translate a websocket.CloseError
	if closeErr, ok := err.(*websocket.CloseError); ok {
		switch closeErr.Code {
		case websocket.CloseGoingAway:
			err = ErrGoingAway
		case websocket.CloseMessageTooBig:
			err = ErrMessageTooBig
		case websocket.CloseServiceRestart:
			err = ErrRestart
		case 4000:
			err = ErrForceClose
		case 4002:
			err = ErrNotAuthenticated
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

func parseURLString(urlStr string) ([]*url.URL, error) {
	urlList := make([]*url.URL, 0)
	for _, str := range strings.Split(urlStr, "|") {
		url, err := url.Parse(str)
		if err != nil {
			return nil, err
		}
		urlList = append(urlList, url)
	}
	// shuffle the list
	rand.Seed(time.Now().UnixNano())
	for i := len(urlList) - 1; i > 0; i-- {
		j := rand.Intn(i + 1)
		urlList[i], urlList[j] = urlList[j], urlList[i]
	}
	return urlList, nil
}
