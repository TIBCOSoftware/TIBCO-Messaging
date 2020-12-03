//
//  Copyright (c) 2013-$Date: 2015-11-04 13:58:38 -0800 (Wed, 04 Nov 2015) $ TIBCO Software Inc.
//  Licensed under a BSD-style license. Refer to [LICENSE]
//  For more information, please contact:
//  TIBCO Software Inc., Palo Alto, California USA
//
//  $Id: eFTLConnectionListener.h 82810 2015-11-04 21:58:38Z bmahurka $
//

#import <Foundation/Foundation.h>

@class eFTLConnection;

/**
 * \file eFTLConnectionListener.h
 *
 * \brief Connection event handler.
 */

/**
 * \brief Connection event handler.
 *
 * You must implement this protocol to process connection events.
 *
 * Supply an instance when you call
 * eFTL::connect:properties:listener:.
 *
 * Programs that reconnect must maintain a strong reference to
 * the connection object.  Otherwise the %eFTL library releases the
 * connection object upon disconnect.
 * 
 */
@protocol eFTLConnectionListener <NSObject>

/**
 * \brief A new connection to the %eFTL server is ready to use.
 *
 * The %eFTL library invokes this method only after your program calls
 * eFTL::connect:properties:listener: (and not after
 * eFTLConnection::reconnectWithProperties:).
 *
 * @param connection This connection is ready to use.
 */
- (void)connectionDidConnect:(eFTLConnection *)connection;

/**
 * \brief A connection to the %eFTL server has closed.
 *
 * Possible codes include:
 *
 * \li \link eFTLConnection::eFTLConnectionNormal \endlink
 * \li \link eFTLConnection::eFTLConnectionShutdown \endlink
 * \li \link eFTLConnection::eFTLConnectionError \endlink
 * \li \link eFTLConnection::eFTLConnectionPolicyViolation \endlink
 * \li \link eFTLConnection::eFTLConnectionMessageTooLarge \endlink
 * \li \link eFTLConnection::eFTLConnectionServerError \endlink
 * \li \link eFTLConnection::eFTLConnectionForceClose \endlink
 * \li \link eFTLConnection::eFTLConnectionNotAuthenticated \endlink
 *
 * @param connection This connection has closed.
 * @param code This code categorizes the condition.
 *               Your program can use this value in its response logic.
 * @param reason This string provides more detail.  Your program
 *               can use this value in error reporting and
 *               logging.
 */
- (void)connection:(eFTLConnection *)connection didDisconnectWithCode:(NSInteger)code reason:(NSString *)reason;

/**
 * \brief An error prevented an operation.  The connection remains open.
 *
 * Your implementation could alert the user.
 * 
 * Possible codes include:
 * \li \link eFTLConnection::eFTLBadSubscriptionId \endlink
 * \li \link eFTLConnection::eFTLPublishDisallowed \endlink
 *
 * @param connection This connection reported an error.
 * @param code This code categorizes the error.
 *             Your program can use this value in its response logic.
 * @param reason This string provides more detail.  Your program
 *               can use this value in error reporting and
 *               logging.
 */
- (void)connection:(eFTLConnection *)connection didFailWithCode:(NSInteger)code reason:(NSString *)reason;

@optional

/**
 * \brief A connection to the %eFTL server has re-opened and is ready to use.
 * 
 * The %eFTL library invokes this method only after your program calls
 * eFTL::connect:properties:listener: (and not after
 * eFTLConnection::reconnectWithProperties:).
 * 
 * @param connection This connection is ready to use.
 */
- (void)connectionDidReconnect:(eFTLConnection *)connection;

@end
