//
//  Copyright (c) 2013-$Date: 2016-04-12 13:27:33 -0500 (Tue, 12 Apr 2016) $ TIBCO Software Inc.
//  Licensed under a BSD-style license. Refer to [LICENSE]
//  For more information, please contact:
//  TIBCO Software Inc., Palo Alto, California USA
//
//  $Id: eFTLSubscriptionListener.h 85385 2016-04-12 18:27:33Z dinitz $
//

#import <Foundation/Foundation.h>

/**
 * \file eFTLSubscriptionListener.h
 *
 * \brief Message handler; subscription event handler
 */

/**
 * \brief Subscription event handler.
 *
 * You must implement this interface to process inbound messages and
 * other subscription events.
 *
 * Supply an instance when you call
 * eFTLConnection::subscribeWithMatcher:durable:listener: or
 * eFTLConnection::subscribeWithMatcher:listener:.
 */
@protocol eFTLSubscriptionListener <NSObject>

/**
 * \brief Process inbound messages.
 *
 * The %eFTL library presents inbound messages to this method for
 * processing.  You must implement this method to process the
 * messages.
 *
 * The messages are not thread-safe.  You may access a
 * message in any thread, but in only one thread at a time.
 * 
 * @param messages Inbound messages.
 */
- (void)didReceiveMessages:(NSArray *)messages;

/**
 * \brief Process subscription errors.
 *
 * Subscriptions can fail with the following codes:
 * \li eFTLConnection#eFTLSubscriptionDisallowed
 * \li eFTLConnection#eFTLSubscriptionFailed
 * \li eFTLConnection#eFTLSubscriptionInvalid
 *
 * @param subscription %eFTL could not establish this subscription.
 * @param code This code categorizes the error.
 *             Your program can use this value in its response logic.
 * @param reason This string provides more detail.  Your program
 *               can use this value in error reporting and logging.
 */
- (void)subscription:(NSString *)subscription didFailWithCode:(NSInteger)code reason:(NSString *)reason;

@optional

/**
 * \brief A new subscription is ready to receive messages.
 *
 * Your program receives the subscription identifier through this
 * method.
 *
 * The %eFTL library may invoke this method after the first message
 * arrives.
 *
 * To close the subscription, call eFTLConnection::unsubscribe:
 * with this subscription identifier.
 *
 * @param subscription This subscription is ready.
 */
- (void)subscriptionDidSubscribe:(NSString *)subscription;

@end
