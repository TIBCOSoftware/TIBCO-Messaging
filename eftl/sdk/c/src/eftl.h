/*
 * Copyright (c) 2001-$Date: 2020-09-29 20:29:25 +0000 (Tue, 29 Sep 2020) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: eftl.h.config 128906 2020-09-29 20:29:25Z $
 *
 */

#ifndef INCLUDED_TIBEFTL_EFTL_H
#define INCLUDED_TIBEFTL_EFTL_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

/// @cond

#if defined(_WIN32) || defined(_WIN64)
  #define DLL_IMPORT __declspec(dllimport)
  #define DLL_EXPORT __declspec(dllexport)
#else
  #define DLL_IMPORT extern
  #define DLL_EXPORT __attribute__ ((visibility ("default")))
#endif

/// @endcond

/**
 * @mainpage TIBCO eFTL&trade; 6.8.0  
 *
 * @section intro Introduction
 *
 * This documentation describes the C API for TIBCO eFTL&trade;.
 *
 * @section usage User Examples
 *
 * TIBCO eFTL&trade; includes sample application programs; see the directory
 * \c samples.
 *
 * @section Copyrights Copyright Notice
 *
 * Copyright &copy; 2013-2021 TIBCO Software Inc. All Rights Reserved. Confidential & Proprietary.
 *
 * This documentation contains proprietary and confidential information
 * of TIBCO Software Inc. Use, disclosure, or reproduction is prohibited
 * without the prior express written permission of TIBCO Software Inc.
 *
 * @subsection restrights Restricted Rights Legend
 *
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in subparagraph (c)(1)(ii) of the
 * rights in Technical Data and Computer Software clause at
 * DFARS 252. 227-7013.
 *
 * TIBCO Software Inc
 * 3303 Hillview Ave, Palo Alto, CA 94304
 */

/**
 * @file eftl.h
 *
 * @brief This file defines the eFTL API.
 */

/** @name Error Codes
 *
 * Error codes returned by eFTL.
 */

//@{

/** @brief No error has been reported.
*/
#define TIBEFTL_ERR_OK                          (0)

/** @brief Invalid argument.
*/
#define TIBEFTL_ERR_INVALID_ARG                 (1)

/** @brief Memory allocation failed.
*/
#define TIBEFTL_ERR_NO_MEMORY                   (2)

/** @brief Request timeout.
*/
#define TIBEFTL_ERR_TIMEOUT                     (3)

/** @brief Item not found.
*/
#define TIBEFTL_ERR_NOT_FOUND                   (4)

/** @brief Item of wrong type.
*/
#define TIBEFTL_ERR_INVALID_TYPE                (5)

/** @brief Not supported.
*/
#define TIBEFTL_ERR_NOT_SUPPORTED               (6)

/** @brief Not connected.
 */
#define TIBEFTL_ERR_NOT_CONNECTED               (8)

/** @brief Connection lost.
 */
#define TIBEFTL_ERR_CONNECTION_LOST             (9)

/** @brief Connect failed.
 */
#define TIBEFTL_ERR_CONNECT_FAILED              (10)

/** @brief Publish failed.
*/
#define TIBEFTL_ERR_PUBLISH_FAILED              (11)

/** @brief Publish failed, not authorized.
*/
#define TIBEFTL_ERR_PUBLISH_NOT_AUTHORIZED      (12)

/** @brief Subscription failed, not authorized.
*/
#define TIBEFTL_ERR_SUBSCRIPTION_NOT_AUTHORIZED (13)

/** @brief Subscription failed.
*/
#define TIBEFTL_ERR_SUBSCRIPTION_FAILED         (21)

/** @brief Subscription failed, invalid matcher or durable.
*/
#define TIBEFTL_ERR_SUBSCRIPTION_INVALID        (22)

/** @brief Map request failed, not authorized.
*/
#define TIBEFTL_ERR_MAP_REQUEST_NOT_AUTHORIZED  (14)

/** @brief Map request failed.
*/
#define TIBEFTL_ERR_MAP_REQUEST_FAILED          (30)

/** @brief Request not authorized.
*/
#define TIBEFTL_ERR_REQUEST_NOT_AUTHORIZED      (40)

/** @brief Request failed.
*/
#define TIBEFTL_ERR_REQUEST_FAILED              (41)

/** @brief Server is shutting down.
*/
#define TIBEFTL_ERR_GOING_AWAY                  (1001)

/** @brief Published message is too big.
*/
#define TIBEFTL_ERR_MESSAGE_TOO_BIG             (1009)

/** @brief Server is restarting.
*/
#define TIBEFTL_ERR_SERVICE_RESTART             (1012)

/** @brief Server closed the connection.
*/
#define TIBEFTL_ERR_FORCE_CLOSE                 (4000)

/** @brief Server is temporarily unavailable.
*/
#define TIBEFTL_ERR_UNAVAILABLE                 (4001)

/** @brief Authentication failed.
*/
#define TIBEFTL_ERR_AUTHENTICATION              (4002)

//@}

/** @brief Message field name identifying the EMS destination of a message.
 *
 * The destination message field is only required when communicating with
 * EMS. 
 *
 * To publish a message on a specific EMS destination include this
 * message field using @ref tibeftlMessage_SetString().
 *
 * @code
 * tibeftlMessage_SetString(err, msg, TIBEFTL_FIELD_NAME_DESTINATION, "MyDest");
 * @endcode
 *
 * To subscribe to messages published on a specific EMS destination
 * use a subscription matcher that includes this message field name
 * in @ref tibeftl_Subscribe().
 *
 * @code
 * char matcher[64];
 *
 * snprintf(matcher, sizeof(matcher), "{\"%s\":\"%s\"}",
 *         TIBEFTL_FIELD_NAME_DESTINATION, "MyDest");
 *
 * tibeftl_Subscribe(err, conn, matcher, "MyDurable", cb, NULL);
 * @endcode
 *
 * To distinguish between topics and queues the destination name
 * can be prefixed with either "TOPIC:" or "QUEUE:", for example
 * "TOPIC:MyDest" or "QUEUE:MyDest". A destination name with no prefix is
 * a topic.
 */
#define TIBEFTL_FIELD_NAME_DESTINATION  "_dest"

/** @brief eFTL error.
 *
 * A tibeftlErr object is passed to function calls to capture
 * information about failures.
 */
typedef struct tibeftlErrStruct *tibeftlErr;

/** @brief eFTL connection.
 *
 * A tibeftlConnection object represents the connection to the
 * server.
 */
typedef struct tibeftlConnectionStruct *tibeftlConnection;

/** @brief eFTL subscription.
 *
 * A tibeftlSubscription object represents an interest in
 * receiving messages with particular content.
 */
typedef struct tibeftlSubscriptionStruct *tibeftlSubscription;

/** @brief eFTL message.
 *
 * A tibeftlMessage object is used to send information between clients.
 */
typedef struct tibeftlMessageStruct *tibeftlMessage;

/** @brief eFTL key-value map.
 *
 * A tibeftlKVMap object is used to set and get key-value pairs.
 */
typedef struct tibeftlKVMapStruct *tibeftlKVMap;

/** @brief Connection states.
 *
 * @see tibeftlStateCallback
 */
typedef enum tibeftlConnectionStateEnum
{
    STATE_INITIAL,
    STATE_DISCONNECTED,
    STATE_CONNECTING,
    STATE_CONNECTED,
    STATE_DISCONNECTING,
    STATE_RECONNECTING,

} tibeftlConnectionState;

/**
 * @brief Get a printable string that describes the connection state.
 */
DLL_EXPORT
const char*
tibeftlConnectionState_ToString(
    tibeftlConnectionState      state);

/**
 * @brief Connection error callback.
 *
 * A connection error callback is invoked whenever an error occurs
 * on the connection. When the connection to the server is lost,
 * tibeftl_Reconnect() can be called from within this callback
 * to attempt a reconnect to the server.
 *
 * @param conn The connection on which the error occurred.
 * @param err The connection error.
 * @param arg The callback-specific argument.
 *
 * @see tibeftl_Connect
 */
typedef void (*tibeftlErrorCallback)(
    tibeftlConnection           conn,
    tibeftlErr                  err,
    void*                       arg);

/**
 * @brief Connection state callback.
 *
 * A connection state callback is invoked whenever the state
 * of the connection changes.
 *
 * @param conn The connection on which the state changed.
 * @param state The current state of the connection.
 * @param arg The callback-specific argument.
 *
 * @see tibeftlOptions
 */
typedef void (*tibeftlStateCallback)(
    tibeftlConnection           conn,
    tibeftlConnectionState      state,
    void*                       arg);

/**
 * @brief Message callback.
 *
 * A message callback is invoked whenever messages are received that
 * matche a subscription.
 *
 * @param conn The connection on which the message was received.
 * @param sub The subscription associated with the message.
 * @param cnt The number of received messages.
 * @param msg The array of received messages.
 * @param arg The callback-specific argument.
 *
 * @see tibeftl_Subscribe
 */
typedef void (*tibeftlMessageCallback)(
    tibeftlConnection           conn,
    tibeftlSubscription         sub,
    int                         cnt,
    tibeftlMessage*             msg,
    void*                       arg);

/** @brief eFTL options.
 */
typedef struct tibeftlOptionsStruct
{
    /** @brief This structure's version. Must be 0.
     */
    int                         ver;

    /** @brief Username used for authentication if not specified
     * with the URL.
     */
    const char*                 username;

    /** @brief Password used for authentication if not specified
     * with the URL.
     */
    const char*                 password;

    /** @brief Unique client identifier if not specified 
     * with the URL.
     */
    const char*                 clientId;

    /**
     * @brief Sets the trust store used for secure connections.
     *
     * The trust store is a file containing one or more PEM encoded
     * certificates used to authenticate the server's certificate on
     * secure connections.
     *
     * If the trust store is not specified the system default CA 
     * certificates will be used.
     */
    const char*                 trustStore;

    /**
     * @brief Sets whether or not to skip server certifiate authentication.
     *
     * Specify \c true to accept any server certificate. This option
     * should only be used during development and testing.
     */
    bool                        trustAll;

    /** @brief Timeout, in milliseconds, for server requests.
     */
    int64_t                     timeout;

    /** @brief Maximum auto-reconnect attempts.
     *
     * A value of 0 disables auto-reconnect. The default values is 256
     * attempts.
     */
    int64_t                     autoReconnectAttempts;

    /** @brief Maximum delay in milliseconds between auto-reconnect attempts.
     *
     * The default value is 30 seconds.
     */
    int64_t                     autoReconnectMaxDelay;

    /** @brief Callback invoked whenever the connection state changes.
     */
    tibeftlStateCallback        stateChangeCallback;

    /** @brief Argument included with the connection state change callback.
     */
    void*                       stateChangeCallbackArg;

    /** 
     * @brief Specifies the maximum number of unacknowledged messages
     * allowed for the client.
     *
     * Once the maximum number of unacknowledged messages is reached the
     * client will stop receiving additional messages until previously
     * received messages are acknowledged.
     *
     * If not specified the server's configured value will be used.
     */
    int                         maxPendingAcks;

} tibeftlOptions;

#define tibeftlOptionsDefault { 2, NULL, NULL, NULL, NULL, false, 10000, 256, 30000, NULL, NULL, 0 }

/** @brief Acknowledge mode.
 *
 * @see tibeftlSubscriptionOptions
 */
typedef enum tibeftlAcknowledgeModeEnum
{
    /** @brief Auto acknowledgment mode.
     *
     * Messages consumed from a subscription with this acknowledgment mode are
     * automatically acknowledged.
     */
    TIBEFTL_ACKNOWLEDGE_MODE_AUTO,
    
    /** @brief Client acknowledgment mode.
     *
     * Messages consumed from a subscription with this acknowledgment mode require
     * explicit acknowledgment by the client.
     *
     * The eFTL server will stop delivering messages to the client once the
     * server's configured maximum unacknowledged messages is reached.
     *
     * @see tibeftl_Acknowledge
     * @see tibeftl_AcknowledgeAll
     */
    TIBEFTL_ACKNOWLEDGE_MODE_CLIENT,
    
    /** @brief None acknowledgment mode.
     *
     * Messages consumed from a subscription with this acknowledgment mode do
     * not require acknowledgment.
     */
    TIBEFTL_ACKNOWLEDGE_MODE_NONE,

} tibeftlAcknowledgeMode;

/** @name Durable Types
 *
 * Durable types.
 */

//@{

/** @brief Shared durable.
*/
#define TIBEFTL_DURABLE_TYPE_SHARED             "shared"

/** @brief Last-value durable.
*/
#define TIBEFTL_DURABLE_TYPE_LAST_VALUE         "last-value"

//@}

/** @brief Subscription options.
 */
typedef struct tibeftlSubscriptionOptionsStruct
{
    /** 
     * @brief Acknowledgment mode.
     *
     * Specifies how messages consumed by the subscription
     * are acknowledged.
     */
    tibeftlAcknowledgeMode      acknowledgeMode;
    
    /** @brief Durable subscription type.
     */
    const char*                 durableType;
    
    /** @brief Key field for "last-value" durable subscriptions.
     */
    const char*                 durableKey;

} tibeftlSubscriptionOptions;

#define tibeftlSubscriptionOptionsDefault { TIBEFTL_ACKNOWLEDGE_MODE_AUTO, NULL, NULL }

/**
 * @brief Version string of the eFTL library.
 *
 * @return The version string of the eFTL library.
 */
DLL_EXPORT
const char*
tibeftl_Version(void);

/**
 * @brief Connect to the server.
 *
 * Creates a connection to the server.
 *
 * When a pipe-separated list of URLs is specified this call will attempt
 * a connection to each in turn, in a random order, until one is connected.
 *
 * The server URLs have the syntax:
 *
 * \c ws://host:port/channel
 * \c wss://host:port/channel
 *
 * Optionally, the URLs can contain the username, password, and/or
 * client identifier: 
 *
 * \c ws://username:password@host:port/channel?clientId=<identifier>
 * \c wss://username:password@host:port/channel?clientId=<identifier>
 *
 * @param err An eFTL error to capture information about failures.
 * @param url A single URL, or a pipe ('|') separated list of URLs, to the server.
 * @param opts (optional) The options to use when creating the connection.
 * @param errCb (optional) The error callback to be invoked when
 * connection errors occur.
 * @param errCbArg (optional) The error callback argument.
 *
 * @return The connection object.
 */
DLL_EXPORT
tibeftlConnection
tibeftl_Connect(
    tibeftlErr                  err,
    const char*                 url,
    tibeftlOptions*             opts,
    tibeftlErrorCallback        errCb,
    void*                       errCbArg);

/**
 * @brief Disconnect from the server and free all resources associated
 * with the connection, including the connection object itself.
 *
 * @param err An eFTL error to capture information about failures.
 * @param conn The connection object.
 */
DLL_EXPORT
void
tibeftl_Disconnect(
    tibeftlErr                  err,
    tibeftlConnection           conn);

/**
 * @brief Reconnect to the server.
 *
 * Can be called from within the @ref tibeftlErrorCallback.
 *
 * All subscriptions are restored following a successful reconnect.
 *
 * @param err An eFTL error to capture information about failures.
 * @param conn The connection object.
 */
DLL_EXPORT
void
tibeftl_Reconnect(
    tibeftlErr                  err,
    tibeftlConnection           conn);

/**
 * @brief Check whether or not there is a connection to the server.
 *
 * @param err An eFTL error to capture information about failures.
 * @param conn The connection object.
 *
 * @return \c true if there is a connection to the server, \c false otherwise.
 */
DLL_EXPORT
bool
tibeftl_IsConnected(
    tibeftlErr                  err,
    tibeftlConnection           conn);

/**
 * @brief Publish a message to all subscribing clients.
 *
 * @param err An eFTL error to capture information about failures.
 * @param conn The connection object.
 * @param msg The message to publish.
 *
 * @see TIBEFTL_FIELD_NAME_DESTINATION
 */
DLL_EXPORT
void
tibeftl_Publish(
    tibeftlErr                  err,
    tibeftlConnection           conn,
    tibeftlMessage              msg);

/**
 * @brief Publish a request message and wait for a reply.
 *
 * @param err An eFTL error to capture information about failures.
 * @param conn The connection object.
 * @param request The request message to publish.
 * @param timeout Timeout, in milliseconds, to wait for a reply.
 *
 * @return The reply message, or NULL if the request times out.
 *
 * @see tibeftl_SendReply
 * @see TIBEFTL_FIELD_NAME_DESTINATION
 */
DLL_EXPORT
tibeftlMessage
tibeftl_SendRequest(
    tibeftlErr                  err,
    tibeftlConnection           conn,
    tibeftlMessage              request,
    int64_t                     timeout);

/**
 * @brief Send a reply message in response to a request message.
 *
 * @param err An eFTL error to capture information about failures.
 * @param conn The connection object.
 * @param reply The reply message to send.
 * @param request The request message.
 *
 * @see tibeftl_SendRequest
 */
DLL_EXPORT
void
tibeftl_SendReply(
    tibeftlErr                  err,
    tibeftlConnection           conn,
    tibeftlMessage              reply,
    tibeftlMessage              request);

/**
 * @brief Subscribe to messages.
 *
 * A subscription can be narrowed by using a matcher that matches
 * only those messages whose content is of interest.
 *
 * @param err An eFTL error to capture information about failures.
 * @param conn The connection object.
 * @param matcher (optional) The matcher for selecting which
 * messages to receive.
 * @param durable (optional) The name to use when creating durable
 * subscriptions.
 * @param msgCb The message callback to be invoked when messages
 * are received.
 * @param msgCbArg The message callback argument.
 *
 * @return The subscription.
 *
 * @see TIBEFTL_FIELD_NAME_DESTINATION
 */
DLL_EXPORT
tibeftlSubscription
tibeftl_Subscribe(
    tibeftlErr                  err,
    tibeftlConnection           conn,
    const char*                 matcher,
    const char*                 durable,
    tibeftlMessageCallback      msgCb,
    void*                       msgCbArg);

/**
 * @brief Subscribe to messages.
 *
 * A subscription can be narrowed by using a matcher that matches
 * only those messages whose content is of interest.
 *
 * @param err An eFTL error to capture information about failures.
 * @param conn The connection object.
 * @param matcher (optional) The matcher for selecting which
 * messages to receive.
 * @param durable (optional) The name to use when creating durable
 * subscriptions.
 * @param opts (optional) The options to use when creating the subscription.
 * @param msgCb The message callback to be invoked when messages
 * are received.
 * @param msgCbArg The message callback argument.
 *
 * @return The subscription.
 */
DLL_EXPORT
tibeftlSubscription
tibeftl_SubscribeWithOptions(
    tibeftlErr                  err,
    tibeftlConnection           conn,
    const char*                 matcher,
    const char*                 durable,
    tibeftlSubscriptionOptions* opts,
    tibeftlMessageCallback      msgCb,
    void*                       msgCbArg);

/**
 * @brief Close a subscription and free resources associated 
 * with the subscription object.
 *
 * For durable subscriptions, calling this function will cause the
 * persistence service to stop delivering messages while leaving
 * the durable subscription to continue accumulating persisted
 * messages. Any unacknowledged messages will be made available
 * for redelivery.
 *
 * @param err An eFTL error to capture information about failures.
 * @param conn The connection object.
 * @param sub The subscription to close.
 */
DLL_EXPORT
void
tibeftl_CloseSubscription(
    tibeftlErr                  err,
    tibeftlConnection           conn,
    tibeftlSubscription         sub);

/**
 * @brief Close all subscriptions and free resources associated 
 * with all subscription objects.
 *
 * For durable subscriptions, calling this function will cause the
 * persistence service to stop delivering messages while leaving
 * the durable subscriptions to continue accumulating persisted
 * messages. Any unacknowledged messages will be made available
 * for redelivery.
 *
 * @param err An eFTL error to capture information about failures.
 * @param conn The connection object.
 */
DLL_EXPORT
void
tibeftl_CloseAllSubscriptions(
    tibeftlErr                  err,
    tibeftlConnection           conn);

/**
 * @brief Unsubscribe from messages and free resources associated
 * with the subscription object.
 *
 * For durable subscriptions, calling this function will cause the
 * persistence service to remove the durable subscription, along
 * with any persisted messages.
 *
 * @param err An eFTL error to capture information about failures.
 * @param conn The connection object.
 * @param sub The subscription from which to unsubscribe.
 */
DLL_EXPORT
void
tibeftl_Unsubscribe(
    tibeftlErr                  err,
    tibeftlConnection           conn,
    tibeftlSubscription         sub);

/**
 * @brief Unsubscribe from all messages and free resources associated
 * with all subscription objects.
 *
 * For durable subscriptions, calling this function will cause the
 * persistence service to remove the durable subscriptions, along
 * with any persisted messages.
 *
 * @param err An eFTL error to capture information about failures.
 * @param conn The connection object.
 */
DLL_EXPORT
void
tibeftl_UnsubscribeAll(
    tibeftlErr                  err,
    tibeftlConnection           conn);

/**
 * @brief Acknowledge this message.
 * 
 * Messages consumed from subscriptions with a client acknowledgment mode
 * must be explicitly acknowledged. The eFTL server will stop delivering
 * messages to the client once the server's configured maximum number of
 * unacknowledged messages is reached.
 *
 * @param err An eFTL error to capture information about failures.
 * @param conn The connection object.
 * @param msg The message to acknowledge.
 */
DLL_EXPORT
void
tibeftl_Acknowledge(
    tibeftlErr                  err,
    tibeftlConnection           conn,
    tibeftlMessage              msg);

/**
 * @brief Acknowledge all messages up to and including this message.
 * 
 * Messages consumed from subscriptions with a client acknowledgment mode
 * must be explicitly acknowledged. The eFTL server will stop delivering
 * messages to the client once the server's configured maximum number of
 * unacknowledged messages is reached.
 *
 * @param err An eFTL error to capture information about failures.
 * @param conn The connection object.
 * @param msg The message being acknowledged.
 */
DLL_EXPORT
void
tibeftl_AcknowledgeAll(
    tibeftlErr                  err,
    tibeftlConnection           conn,
    tibeftlMessage              msg);

/**
 * @brief Create a key-value map.
 *
 * @param err An eFTL error to capture information about failures.
 * @param conn The connection object.
 * @param name The key-value map name.
 *
 * @return The key-value map.
 */ 
DLL_EXPORT
tibeftlKVMap
tibeftl_CreateKVMap(
    tibeftlErr                  err,
    tibeftlConnection           conn,
    const char*                 name);

/**
 * @brief Remove a key-value map.
 *
 * @param err An eFTL error to capture information about failures.
 * @param conn The connection object.
 * @param name The key-value map name.
 *
 * @return The key-value map.
 */ 
DLL_EXPORT
void
tibeftl_RemoveKVMap(
    tibeftlErr                  err,
    tibeftlConnection           conn,
    const char*                 name);

/**
 * @brief Destroy a key-value map.
 *
 * @param err An eFTL error to capture information about failures.
 * @param map The key-value map object.
 */
DLL_EXPORT
void
tibeftlKVMap_Destroy(
    tibeftlErr                  err,
    tibeftlKVMap                map);

/**
 * @brief Set a key-value pair in a map.
 *
 * @param err An eFTL error to capture information about failures.
 * @param map The key-value map object.
 * @param key The key name.
 * @param msg The value to set.
 */
DLL_EXPORT
void
tibeftlKVMap_Set(
    tibeftlErr                  err,
    tibeftlKVMap                map,
    const char*                 key,
    tibeftlMessage              msg);

/**
 * @brief Get a key-value pair from a map.
 *
 * @param err An eFTL error to capture information about failures.
 * @param map The key-value map.
 * @param key The key name.
 *
 * @return The value, or \c NULL if not set.
 */
DLL_EXPORT
tibeftlMessage
tibeftlKVMap_Get(
    tibeftlErr                  err,
    tibeftlKVMap                map,
    const char*                 key);

/**
 * @brief Remove a key-value pair from a map.
 *
 * @param err An eFTL error to capture information about failures.
 * @param map The key-value map.
 * @param key The key name.
 */
DLL_EXPORT
void
tibeftlKVMap_Remove(
    tibeftlErr                  err,
    tibeftlKVMap                map,
    const char*                 key);

/**
 * @brief Create an error object.
 *
 * @return The error object.
 */
DLL_EXPORT
tibeftlErr
tibeftlErr_Create(void);

/**
 * @brief Destroy an error object.
 *
 * @param err The error object.
 */
DLL_EXPORT
void
tibeftlErr_Destroy(
    tibeftlErr                  err);

/**
 * @brief Check if an error has been set.
 *
 * @param err The error object.
 *
 * @return \c true if the error is set, \c false otherwise.
 */
DLL_EXPORT
bool
tibeftlErr_IsSet(
    tibeftlErr                  err);

/**
 * @brief Get the error code.
 *
 * @param err The error object.
 *
 * @return The error code.
 */
DLL_EXPORT
int
tibeftlErr_GetCode(
    tibeftlErr                  err);

/**
 * @brief Get the error description.
 *
 * @param err The error object.
 *
 * @return The error description.
 */
DLL_EXPORT
const char*
tibeftlErr_GetDescription(
    tibeftlErr                  err);

/**
 * @brief Set an error.
 *
 * @param err The error object.
 * @param code The error code.
 * @param desc The error description.
 */
DLL_EXPORT
void
tibeftlErr_Set(
    tibeftlErr                  err,
    int                         code,
    const char*                 desc);

/**
 * @brief Clear an error.
 *
 * @param err The error object.
 */
DLL_EXPORT
void
tibeftlErr_Clear(
    tibeftlErr                  err);

/** @brief eFTL message field types.
 */
typedef enum tibeftlFieldType_e
{
    TIBEFTL_FIELD_TYPE_UNKNOWN,
    TIBEFTL_FIELD_TYPE_STRING,
    TIBEFTL_FIELD_TYPE_STRING_ARRAY,
    TIBEFTL_FIELD_TYPE_LONG,
    TIBEFTL_FIELD_TYPE_LONG_ARRAY,
    TIBEFTL_FIELD_TYPE_DOUBLE,
    TIBEFTL_FIELD_TYPE_DOUBLE_ARRAY,
    TIBEFTL_FIELD_TYPE_TIME,
    TIBEFTL_FIELD_TYPE_TIME_ARRAY,
    TIBEFTL_FIELD_TYPE_MESSAGE,
    TIBEFTL_FIELD_TYPE_MESSAGE_ARRAY,
    TIBEFTL_FIELD_TYPE_OPAQUE

} tibeftlFieldType;

/**
 * @brief Create a message object.
 *
 * @param err An eFTL error to capture information about failures.
 *
 * @return The message object.
 */
DLL_EXPORT
tibeftlMessage
tibeftlMessage_Create(
    tibeftlErr                  err);

/**
 * @brief Destroy a message object.
 *
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 */
DLL_EXPORT
void
tibeftlMessage_Destroy(
    tibeftlErr                  err,
    tibeftlMessage              msg);

/**
 * @brief Get a printable string that describes the contents of a
 * message.
 *
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 * @param buffer (optional) The buffer into which the printable string is stored.
 * @param size The size of the buffer in bytes.
 *
 * @return The actual length of the printable string, including
 * the terminating \c NULL character, in bytes.
 */
DLL_EXPORT
size_t
tibeftlMessage_ToString(
    tibeftlErr                  err,
    tibeftlMessage              msg,
    char*                       buffer,
    size_t                      size);

/**
 * @brief Get the message's unique store identifier
 * assigned by the persistence service.
 *
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 *
 * @return The message identifier.
 */
DLL_EXPORT
int64_t
tibeftlMessage_GetStoreMessageId(
    tibeftlErr                  err,
    tibeftlMessage              msg);

/**
 * @brief Get the message's delivery count 
 * assigned by the persistence service.
 *
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 *
 * @return The message delivery count.
 */
DLL_EXPORT
int64_t
tibeftlMessage_GetDeliveryCount(
    tibeftlErr                  err,
    tibeftlMessage              msg);

/**
 * @brief Check whether a field is set in a message.
 * 
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 * @param field The field name.
 *
 * @return \c true if the field is set, \c false otherwise.
 */
DLL_EXPORT
bool
tibeftlMessage_IsFieldSet(
    tibeftlErr                  err,
    tibeftlMessage              msg,
    const char*                 field);

/**
 * @brief Get the type of a field in a message.
 * 
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 * @param field The field name.
 *
 * @return The type of the field.
 */
DLL_EXPORT
tibeftlFieldType
tibeftlMessage_GetFieldType(
    tibeftlErr                  err,
    tibeftlMessage              msg,
    const char*                 field);

/**
 * @brief Get the next field in a message.
 * 
 * Calling this function repeatedly will iterate over all of the
 * fields in a message. 
 *
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 * @param field The name of the next field.
 * @param type The type of the next field.
 *
 * @return \c true if there are additional fields,
 * \c false if the current field is the last field.
 */
DLL_EXPORT
bool
tibeftlMessage_NextField(
    tibeftlErr                  err,
    tibeftlMessage              msg,
    const char**                field,
    tibeftlFieldType*           type);

/**
 * @brief Clear a field from a message.
 * 
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 * @param field The name of the field to clear.
 */
DLL_EXPORT
void
tibeftlMessage_ClearField(
    tibeftlErr                  err,
    tibeftlMessage              msg,
    const char*                 field);

/**
 * @brief Clear all fields from a message.
 * 
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 */
DLL_EXPORT
void
tibeftlMessage_ClearAllFields(
    tibeftlErr                  err,
    tibeftlMessage              msg);

/**
 * @brief Set a string field in a message.
 * 
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 * @param field The name of the field to set.
 * @param value The value of the field to set.
 */
DLL_EXPORT
void
tibeftlMessage_SetString(
    tibeftlErr                  err,
    tibeftlMessage              msg,
    const char*                 field,
    const char*                 value);

/**
 * @brief Set a long field in a message.
 * 
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 * @param field The name of the field to set.
 * @param value The value of the field to set.
 */
DLL_EXPORT
void
tibeftlMessage_SetLong(
    tibeftlErr                  err,
    tibeftlMessage              msg,
    const char*                 field,
    int64_t                     value);

/**
 * @brief Set a double field in a message.
 * 
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 * @param field The name of the field to set.
 * @param value The value of the field to set.
 */
DLL_EXPORT
void
tibeftlMessage_SetDouble(
    tibeftlErr                  err,
    tibeftlMessage              msg,
    const char*                 field,
    double                      value);

/**
 * @brief Set a time field in a message.
 * 
 * Time is the number of milliseconds since the Epoch, 
 * 1970-01-01 00:00:00 +0000 (UTC).
 *
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 * @param field The name of the field to set.
 * @param value The value of the field to set.
 */
DLL_EXPORT
void
tibeftlMessage_SetTime(
    tibeftlErr                  err,
    tibeftlMessage              msg,
    const char*                 field,
    int64_t                     value);

/**
 * @brief Set a message field in a message.
 * 
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 * @param field The name of the field to set.
 * @param value The value of the field to set.
 */
DLL_EXPORT
void
tibeftlMessage_SetMessage(
    tibeftlErr                  err,
    tibeftlMessage              msg,
    const char*                 field,
    tibeftlMessage              value);

/**
 * @brief Set an opaque field in a message.
 * 
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 * @param field The name of the field to set.
 * @param value The value of the field to set.
 * @param size The size of the field to set in bytes.
 */
DLL_EXPORT
void
tibeftlMessage_SetOpaque(
    tibeftlErr                  err,
    tibeftlMessage              msg,
    const char*                 field,
    void*                       value,
    size_t                      size);

/**
 * @brief Set an array field in a message.
 * 
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 * @param type The type of the field to set.
 * @param field The name of the field to set.
 * @param value The value of the field to set.
 * @param size The number of elements in the array.
 */
DLL_EXPORT
void
tibeftlMessage_SetArray(
    tibeftlErr                  err,
    tibeftlMessage              msg,
    tibeftlFieldType            type,
    const char*                 field,
    const void* const           value,
    int                         size);

/**
 * @brief Get a string field from a message.
 * 
 * The returned string is valid only for the lifetime of the message.
 * The returned string must not be modified or freed.
 *
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 * @param field The name of the field to get.
 * 
 * @return The value of the field.
 */
DLL_EXPORT
const char*
tibeftlMessage_GetString(
    tibeftlErr                  err,
    tibeftlMessage              msg,
    const char*                 field);

/**
 * @brief Get a long field from a message.
 * 
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 * @param field The name of the field to get.
 * 
 * @return The value of the field.
 */
DLL_EXPORT
int64_t
tibeftlMessage_GetLong(
    tibeftlErr                  err,
    tibeftlMessage              msg,
    const char*                 field);

/**
 * @brief Get a double field from a message.
 * 
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 * @param field The name of the field to get.
 * 
 * @return The value of the field.
 */
DLL_EXPORT
double
tibeftlMessage_GetDouble(
    tibeftlErr                  err,
    tibeftlMessage              msg,
    const char*                 field);

/**
 * @brief Get a time field from a message.
 * 
 * Time is the number of milliseconds since the Epoch, 
 * 1970-01-01 00:00:00 +0000 (UTC).
 *
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 * @param field The name of the field to get.
 * 
 * @return The value of the field.
 */
DLL_EXPORT
int64_t
tibeftlMessage_GetTime(
    tibeftlErr                  err,
    tibeftlMessage              msg,
    const char*                 field);

/**
 * @brief Get a message field from a message.
 * 
 * The returned message is valid only for the lifetime of the message.
 * The returned message must not be modified or freed.
 * 
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 * @param field The name of the field to get.
 * 
 * @return The value of the field.
 */
DLL_EXPORT
tibeftlMessage
tibeftlMessage_GetMessage(
    tibeftlErr                  err,
    tibeftlMessage              msg,
    const char*                 field);

/**
 * @brief Get an opaque field from a message.
 *
 * The returned opaque is valid only for the lifetime of the message.
 * The returned opaque must not be modified or freed.
 * 
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 * @param field The name of the field to get.
 * @param size The size of the field in bytes.
 * 
 * @return The value of the field.
 */
DLL_EXPORT
void*
tibeftlMessage_GetOpaque(
    tibeftlErr                  err,
    tibeftlMessage              msg,
    const char*                 field,
    size_t*                     size);

/**
 * @brief Get an array field from a message.
 * 
 * The returned array is valid only for the lifetime of the message.
 * The returned array must not be modified or freed.
 *
 * @param err An eFTL error to capture information about failures.
 * @param msg The message object.
 * @param type The field type to get.
 * @param field The name of the field to get.
 * @param size The number of elements in the array.
 * 
 * @return The value of the field.
 */
DLL_EXPORT
void*
tibeftlMessage_GetArray(
    tibeftlErr                  err,
    tibeftlMessage              msg,
    tibeftlFieldType            type,
    const char*                 field,
    int*                        size);

#if defined(__cplusplus)
}
#endif

#endif /* INCLUDED_TIBEFTL_EFTL_H */
