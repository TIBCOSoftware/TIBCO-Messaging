//
//  Copyright (c) 2013-$Date: 2020-02-06 12:13:10 -0800 (Thu, 06 Feb 2020) $ TIBCO Software Inc.
//  Licensed under a BSD-style license. Refer to [LICENSE]
//  For more information, please contact:
//  TIBCO Software Inc., Palo Alto, California USA
//
//  $Id: eFTLConnection.h 121481 2020-02-06 20:13:10Z bpeterse $
//

#import <Foundation/Foundation.h>

#import "eFTLSubscriptionListener.h"
#import "eFTLConnectionListener.h"
#import "eFTLCompletionListener.h"
#import "eFTLMessage.h"

/**
 * \file eFTLConnection.h
 *
 * \brief Send messages; subscribe; manage connections; error code constants.
 */

/** 
 * \memberof eFTLConnection
 * \brief Normal close (1000).
 *
 *  Your program closed the connection by calling 
 *  eFTLConnection::disconnect:.
 *  You may now proceed with clean-up. */
extern NSInteger const eFTLConnectionNormal;

/** 
 * \memberof eFTLConnection
 * \brief The server shut down the connection (1001).
 *
 *  Programs may wait, then attempt to reconnect. */
extern NSInteger const eFTLConnectionShutdown;

/** 
 * \memberof eFTLConnection
 * \brief Protocol error (1002).  Please report this error to TIBCO. */
extern NSInteger const eFTLConnectionProtocol;

/** 
 * \memberof eFTLConnection
 * \brief Invalid message from server (1003).  Please report this error to TIBCO. */
extern NSInteger const eFTLConnectionBadData;

/** 
 * \memberof eFTLConnection
 * \brief Connection error (1006). 
 *
 * Programs may attempt to reconnect. */
extern NSInteger const eFTLConnectionError;

/** 
 * \memberof eFTLConnection
 * \brief Invalid network data (1007).  Please report this error to TIBCO. */
extern NSInteger const eFTLConnectionBadPayload;

/** 
 * \memberof eFTLConnection
 * \brief Policy violation (1008).  Please report this error to TIBCO. */
extern NSInteger const eFTLConnectionPolicyViolation;

/** 
 * \memberof eFTLConnection
 * \brief Message too large (1009).
 *
 *  The %eFTL server closed the connection because your program sent
 *  a message that exceeds the server's limit.  (Administrators
 *  configure this limit.)
 */
extern NSInteger const eFTLConnectionMessageTooLarge;

/** 
 * \memberof eFTLConnection
 * \brief %eFTL server error (1011).  Please report this error to TIBCO. */
extern NSInteger const eFTLConnectionServerError;

/** 
 * \memberof eFTLConnection
 * \brief The server is restarting (1012).
 *
 *  Programs may wait, then attempt to reconnect. */
extern NSInteger const eFTLConnectionRestart;

/** 
 * \memberof eFTLConnection
 * \brief SSL handshake failed (1015).
 *
 * The client could not establish a secure connection to the %eFTL
 * server.
 *
 * Possible diagnoses:
 *   - Client rejected server certificate.
 *   - Encryption protocols do not match. */
extern NSInteger const eFTLConnectionFailedSSLHandshake;

/** 
 * \memberof eFTLConnection
 * \brief Force close (4000).
 *
 * The %eFTL server closed the connection because another client with
 * the same client identifier has connected.
 */
extern NSInteger const eFTLConnectionForceClose;

/** 
 * \memberof eFTLConnection
 * \brief Not authenticated (4002).
 *
 * The %eFTL server could not authenticate the client's username and
 * password.
 */
extern NSInteger const eFTLConnectionNotAuthenticated;

/** 
 * \memberof eFTLConnection
 * \brief Bad subscription identifier (20).
 *
 * The server detected an invalid subscription identifier.
 * Please report this error to TIBCO support staff.
 */
extern NSInteger const eFTLBadSubscriptionId;

/** 
 * \memberof eFTLConnection
 * \brief The %eFTL server could not forward the published message (11).
 *
 *  To determine the root cause, examine the server log.
 *
 *  You may attempt to publish again.
 */
extern NSInteger const eFTLPublishFailed;

/** 
 * \memberof eFTLConnection
 * \brief User cannot publish messages (12).
 *
 *  The user does not have permission to publish.
 *
 *  Administrators configure permission to publish.
 */
extern NSInteger const eFTLPublishDisallowed;

/** 
 * \memberof eFTLConnection
 * \brief User cannot subscribe (13).
 *
 *  The user does not have permission to subscribe.
 *
 *  Administrators configure permission to subscribe.
 */
extern NSInteger const eFTLSubscriptionDisallowed;

/** 
 * \memberof eFTLConnection
 * \brief The %eFTL server could not establish the subscription (21).
 *
 * You may attempt to subscribe again.  It is good practice that
 * administrators check the server log to determine the root
 * cause.
 */
extern NSInteger const eFTLSubscriptionFailed;

/** 
 * \memberof eFTLConnection
 * \brief The client supplied an invalid matcher or durable name in
 * the subscribe call (22).
 */
extern NSInteger const eFTLSubscriptionInvalid;

/**
 * \brief A connection object represents a program's connection to an
 * %eFTL server.
 *
 * Programs use connection objects to send messages, and subscribe to
 * messages.
 *
 * Programs receive connection objects through eFTLConnectionListener
 * callbacks.
 */
@interface eFTLConnection : NSObject

/**
 * \brief Gets the client identifier for this connection.
 *
 * @return The client's identifier.
 * @see eFTL::connect:properties:listener:
 */
- (NSString *)getClientId;

/**
 * \brief Reopen a closed connection.
 *
 * You may call this method within your
 * eFTLConnectionListener::connection:didDisconnectWithCode:reason: method.
 *
 * This call returns immediately; connecting continues asynchronously.
 * When the connection is ready to use, the %eFTL library calls your
 * eFTLConnectionListener::connectionDidReconnect: callback.
 *
 * Reconnecting automatically re-activates all
 * subscriptions on the connection.
 * The %eFTL library invokes your
 * eFTLSubscriptionListener::subscriptionDidSubscribe: callback
 * for each successful resubscription.
 *
 * @param properties These properties affect the connection attempt.
 *                   You must supply username and password
 *                   credentials each time.  All other properties
 *                   remain stored from earlier connect and reconnect
 *                   calls.  New values overwrite stored values.
 * @see eFTL::eFTLPropertyClientId
 * @see eFTL::eFTLPropertyUsername
 * @see eFTL::eFTLPropertyPassword
 * @see eFTL::eFTLPropertyTimeout
 * @see eFTL::eFTLPropertyNotificationToken
 */
- (void)reconnectWithProperties:(NSDictionary *)properties;

/**
 * \brief Disconnect from the %eFTL server.
 *
 * Programs may disconnect to free server resources.
 *
 * This call returns immediately; disconnecting continues
 * asynchronously.  When the connection has
 * closed, the %eFTL library calls your
 * eFTLConnectionListener::connection:didDisconnectWithCode:reason:
 * callback.
 */
- (void)disconnect;

/**
 * \brief Determine whether this connection to the %eFTL server is open
 * or closed.
 *
 * @return \c YES if this connection is open; \c NO otherwise.
 */
- (BOOL)isConnected;

/**
 * \brief Subscribe to messages.
 * 
 * Register a subscription for one-to-many messages.
 *
 * This call returns immediately; subscribing continues
 * asynchronously.  When the subscription is
 * ready to receive messages, the %eFTL library calls your 
 * eFTLSubscriptionListener::subscriptionDidSubscribe: callback.
 *
 * A matcher can narrow subscription interest in the inbound
 * message stream.
 *
 * @param matcher The subscription uses this content matcher to
 *                narrow the message stream. 
 * @param listener This listener defines callback methods for
 *                 successful subscription, message arrival, and
 *                 errors.
 *
 * @return An identifier that represents the new subscription.
 *
 * @see eFTLConnection::unsubscribe:
 * @see eFTLSubscriptionListener
 */
- (NSString *)subscribeWithMatcher:(NSString *)matcher
                          listener:(id<eFTLSubscriptionListener>)listener;

/**
 * \brief Create a durable subscriber to messages.
 *
 * Register a durable subscription for one-to-many messages.
 *
 * This call returns immediately; subscribing continues
 * asynchronously.  When the subscription is
 * ready to receive messages, the %eFTL library calls your
 * eFTLSubscriptionListener::subscriptionDidSubscribe: callback.
 *
 * A matcher can narrow subscription interest in the inbound
 * message stream.
 *
 * @param matcher The subscription uses this content matcher to
 *                narrow the message stream.
 * @param durable The subscription uses this durable name. 
 * @param listener This listener defines callback methods for
 *                 successful subscription, message arrival, and
 *                 errors.
 *
 * @return An identifier that represents the new subscription.
 *
 * @see eFTLConnection::unsubscribe:
 * @see eFTLSubscriptionListener
 */
- (NSString *)subscribeWithMatcher:(NSString *)matcher
                           durable:(NSString *)durable
                          listener:(id<eFTLSubscriptionListener>)listener;

/**
 * \brief Create a durable subscriber to messages.
 *
 * Register a durable subscription for one-to-many messages.
 *
 * This call returns immediately; subscribing continues
 * asynchronously.  When the subscription is
 * ready to receive messages, the %eFTL library calls your
 * eFTLSubscriptionListener::subscriptionDidSubscribe: callback.
 *
 * A matcher can narrow subscription interest in the inbound
 * message stream.
 *
 * @param matcher The subscription uses this content matcher to
 *                narrow the message stream.
 * @param durable The subscription uses this durable name.
 * @param properties These properties affect the subscription.
 * @param listener This listener defines callback methods for
 *                 successful subscription, message arrival, and
 *                 errors.
 *
 * @return An identifier that represents the new subscription.
 *
 * @see eFTLConnection::unsubscribe:
 * @see eFTLSubscriptionListener
 */
- (NSString *)subscribeWithMatcher:(NSString *)matcher
                           durable:(NSString *)durable
                        properties:(NSDictionary *)properties
                          listener:(id<eFTLSubscriptionListener>)listener;

/**
 * \brief Close a subscription.
 *
 * Programs receive subscription identifiers through their
 * eFTLSubscriptionListener::subscriptionDidSubscribe: methods.
 *
 * @param subscription Close this subscription.
 *
 * @see eFTLConnection::subscribeWithMatcher:listener:
 */
- (void)unsubscribe:(NSString *)subscription;

/**
 * \brief Close all subscriptions.
 *
 * @see eFTLConnection::subscribeWithMatcher:listener:
 */
- (void)unsubscribeAll;

/**
 * \brief Publish a one-to-many message to all subscribing clients.
 *
 * It is good practice to publish each message to a specific
 * destination by using the message field name 
 * eFTLMessage::eFTLFieldNameDestination.
 *
 * To direct a message to a specific destination,
 * add a string field to the message; for example:
 * @code
 * [message setField:eFTLFieldNameDestination asString:@"myTopic"];
 * @endcode
 *
 * @param message Publish this message.
 * @exception NSInvalidArgumentException The message exceeds the
 * maximum message size.
 */
- (void)publishMessage:(eFTLMessage *)message;

/**
 * \brief Publish a one-to-many message to all subscribing clients.
 *
 * This call returns immediately; publishing continues
 * asynchronously.  When the publish completes successfully,
 * the %eFTL library calls your
 *  eFTLCompletionListener::messageDidComplete: callback.
 *
 * It is good practice to publish each message to a specific
 * destination by using the message field name
 * eFTLMessage::eFTLFieldNameDestination.
 *
 * To direct a message to a specific destination,
 * add a string field to the message; for example:
 * @code
 * [message setField:eFTLFieldNameDestination asString:@"myTopic"];
 * @endcode
 *
 * @param message Publish this message.
 * @param listener This listener defines callback methods for
 *                 successful completion and errors.
 * @exception NSInvalidArgumentException The message exceeds the
 * maximum message size.
 */
- (void)publishMessage:(eFTLMessage *)message
              listener:(id<eFTLCompletionListener>)listener;

@end
