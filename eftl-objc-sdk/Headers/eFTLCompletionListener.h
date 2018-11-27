//
//  Copyright (c) 2013-$Date: 2013-10-31 11:40:42 -0700 (Thu, 31 Oct 2013) $ TIBCO Software Inc.
//  Licensed under a BSD-style license. Refer to [LICENSE]
//  For more information, please contact:
//  TIBCO Software Inc., Palo Alto, California USA
//
//  $Id: eFTLSubscriptionListener.h 70484 2013-10-31 18:40:42Z jarchbol $
//

#import <Foundation/Foundation.h>

@class eFTLMessage;

/**
 * \file eFTLCompletionListener.h
 *
 * \brief Completion event handler.
 */

/**
 * \brief Completion event handler.
 *
 * Implement this interface to process completion events.
 *
 * Supply an instance when you call
 * eFTLConnection::publishMessage:listener:.
 */
@protocol eFTLCompletionListener <NSObject>

/**
 * \brief A publish operation has completed successfully.
 *
 * @param message This message has been published.
 */
- (void)messageDidComplete:(eFTLMessage *)message;

/**
 * \brief A publish operation resulted in an error.
 *
 * The message was not forwarded by the eFTL server.
 * 
 * When developing a client application, consider alerting the
 * user to the error.
 *
 * Possible error codes include:
 * \li eFTLConnection#eFTLPublishFailed
 * \li eFTLConnection#eFTLPublishDisallowed
 *
 * @param message This message was <i> not </i> published.
 * @param code This code categorizes the error.
 *             Application programs may use this value
 *             in response logic.
 * @param reason This string provides more detail about the error.
 *               Application programs may use this value
 *               for error reporting and logging.
 */
- (void)message:(eFTLMessage *)message didFailWithCode:(NSInteger)code reason:(NSString *)reason;

@end
