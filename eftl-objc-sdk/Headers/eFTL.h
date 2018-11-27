//
//  Copyright (c) 2013-$Date: 2018-02-05 18:15:48 -0600 (Mon, 05 Feb 2018) $ TIBCO Software Inc.
//  Licensed under a BSD-style license. Refer to [LICENSE]
//  For more information, please contact:
//  TIBCO Software Inc., Palo Alto, California USA
//
//  $Id: eFTL.h 99237 2018-02-06 00:15:48Z bpeterse $
//

#import <Foundation/Foundation.h>

#import "eFTLConnection.h"

/**
 * \file eFTL.h
 *
 * \brief Factory for server connections; method to register trust in
 * server certificates.
 */

/**
 * \memberof eFTL
 * \brief Connect as this user; property name.
 *
 * Programs use this property with the eFTL::connect:properties:listener:
 * call to supply a username credential if the username is not specified
 * with the URL. The server authenticates the username and password.
 */
extern NSString *const eFTLPropertyUsername;

/**
 * \memberof eFTL
 * \brief Connect using this password; property name.
 *
 * Programs use this property with the eFTL::connect:properties:listener:
 * call to supply a password credential if the password is not specified
 * with the URL. The server authenticates the username and password.
 */
extern NSString *const eFTLPropertyPassword;

/**
 * \memberof eFTL
 * \brief Connection attempt timeout; property name.
 *
 * If %eFTL cannot connect to the server within this time limit (in
 * seconds), it stops trying to connect and invokes your
 * eFTLConnectionListener::connection:didDisconnectWithCode:reason:
 * method with an eFTLConnection::eFTLConnectionError code.
 *
 * If you omit this property, the default timeout is 15.0 seconds.
 */
extern NSString *const eFTLPropertyTimeout;

/**
 * \memberof eFTL
 * \brief Connect using this client identifier; property name.
 *
 * Programs use this property to supply a client identifier to the
 * eFTL::connect:properties:listener: call if the client identifier
 * is not specified with the URL. The server uses the client
 * identifier to associate a particular client with a durable
 * subscription.
 *
 * If you omit this property, the server assigns a unique client
 * identifier.
 */
extern NSString *const eFTLPropertyClientId;

/**
 * \memberof eFTL
 * \brief Token for push notifications; property name.
 *
 * Programs use this property to supply an APNS device token 
 * to the eFTL::connect:properties:listener: call.
 *
 * The %eFTL server uses the token to push notifications to a
 * disconnected client when messages are available.
 */
extern NSString *const eFTLPropertyNotificationToken;

/**
 * \memberof eFTL
 * \brief Specify number of autoreconnect attempts for lost connections; property name.
 *
 * Programs use this property to supply the maximum number of autoreconnect attempts for a lost connection
 * to the eFTL::connect:properties:listener: call.
 *
 * If not specified, the default value is 5.
 */
extern NSString *const eFTLPropertyAutoReconnectAttempts;

/**
 * \memberof eFTL
 * \brief Specify the maximum delay between autoreconnect attempts for lost connections; property name.
 *
 * Programs use this property to supply the maximum delay between autoreconnect attempts for a lost connection
 * to the eFTL::connect:properties:listener: call.
 * Following the loss of a connection, the client delays for 1 second before attempting to autoreconnect.
 * Subsequent attempts double the delay time, up to the maximum specified.
 *
 * If not specified, the default value is 30 seconds.
 */
extern NSString *const eFTLPropertyAutoReconnectMaxDelay;

/**
 * \memberof eFTL
 * \brief Create a durable subscription of this type; property name.
 *
 * Programs use this optional property to supply a durable type to the
 * eFTLConnection::subscribeWithMatcher:durable:properties:listener call.
 *
 * The available durable types are eFTL::eFTLDurableTypeShared and
 * eFTL::eFTLDurableTypeLastValue.
 *
 * If not specified the default durable type will be used.
 */
extern NSString *const eFTLPropertyDurableType;

/**
 * \memberof eFTL
 * \brief Specify the key field of a eFTL::DurableTypeLastValue durable
 * subscription; property name.
 *
 * Programs use this property to supply a key field to the
 * eFTLConnection::subscribeWithMatcher:durable:properties:listener call
 * when the eFTL::eFTLPropertyDurableType is of type eFTL::eFTLDurableTypeLastValue.
 *
 * Note that the supplied key field must be a part of the durable
 * subscription's matcher.
 */
extern NSString *const eFTLPropertyDurableKey;

/**
 * \memberof eFTL
 * \brief Shared durable type.
 *
 * Multiple cooperating subscribers can use the same shared durable
 * to each receive a portion of the subscription's messages.
 */
extern NSString *const eFTLDurableTypeShared;

/**
 * \memberof eFTL
 * \brief Last-value durable type.
 *
 * A last-value durable subscription stores only the most recent message
 * for each unique value of the key field.
 */
extern NSString *const eFTLDurableTypeLastValue;

/**
 * \brief Programs use class EFTL to connect to an %eFTL server.
 */
@interface eFTL : NSObject

/**
 * \brief Get the version of the %eFTL Objective-C client library.
 *
 * @return The version of the %eFTL Objective-C client library.
 */
+ (NSString *)version;

/**
 * \brief Set the SSL trust certificates that the client library uses
 * to verify certificates from the %eFTL server.
 *
 * You cannot set trust certificates after the first
 * \link eFTL::connect:properties:listener: \endlink call.
 *
 * If you do not set the SSL trust certificates, then the application
 * trusts any server certificate.
 *
 * This example illustrates loading a certificate from the
 * application's main bundle:
 *
 * \code{.m}
 *     NSString *path = [[NSBundle mainBundle] pathForResource:@"ca"
 *                                             ofType:@"cer"]
 *     NSData *data = [NSData dataWithContentsOfFile:path];
 *     SecCertificateRef certificate =
 *         SecCertificateCreateWithData(NULL, (__bridge CFDataRef)data);
 *     NSArray *certificates = 
 *         [NSArray arrayWithObject:(__bridge id)(certificate)];
 *     [eFTL setSSLTrustCertificates:certificates];
 * \endcode
 *
 * @param certificates Use the trust certificates in this array,
 * or `nil` to remove an array of trust certificates.
 */
+ (void)setSSLTrustCertificates:(NSArray *)certificates;

/**
 * \brief Connect to an %eFTL server.
 *
 * This call returns immediately; connecting continues asynchronously.
 * When the connection is ready to use, the %eFTL library calls your
 * eFTLConnectionListener::connectionDidConnect: method, passing an
 * eFTLConnection object that you can use to publish and subscribe.
 *
 * A program that uses more than one server channel must connect
 * separately to each channel.
 *
 * @param url The call connects to the %eFTL server at this URL.
 *            The URL can be in either of these forms:
 *             \code
 * ws://host:port/channel}
 * wss://host:port/channel}
 *             \endcode
 *            Optionally, the URL can contain the username, password,
 *            and/or client identifier:
 *             \code
 * ws://username:password@host:port/channel?clientId=<identifier>}
 * wss://username:password@host:port/channel?clientId=<identifier>}
 *             \endcode
 * @param properties These properties affect the connection attempt:
 *                    \li \link eFTL::eFTLPropertyClientId \endlink
 *                    \li \link eFTL::eFTLPropertyUsername \endlink
 *                    \li \link eFTL::eFTLPropertyPassword \endlink
 *                    \li \link eFTL::eFTLPropertyTimeout \endlink
 *                    \li \link eFTL::eFTLPropertyNotificationToken \endlink
 * @param listener Connection events invoke methods of this listener.
 * @exception NSInvalidArgumentException The URL is invalid.
 * @see eFTLConnectionListener
 */
+ (void)connect:(NSString *)url properties:(NSDictionary *)properties listener:(id<eFTLConnectionListener>)listener;

@end
