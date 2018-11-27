//
//  Copyright (c) 2001-$Date: 2018-02-07 15:17:24 -0600 (Wed, 07 Feb 2018) $ TIBCO Software Inc.
//  Licensed under a BSD-style license. Refer to [LICENSE]
//  For more information, please contact:
//  TIBCO Software Inc., Palo Alto, California USA
//
//  $Id: eFTLConnection.m 99274 2018-02-07 21:17:24Z bpeterse $
//

#include <libkern/OSAtomic.h>

#import "SocketRocket/SRWebSocket.h"

#import "eFTL.h"

#import "version.h"

#if TARGET_IPHONE_SIMULATOR
#define LogDebug(dbg, fmt, ...) if(dbg) NSLog((@"%s " fmt), __PRETTY_FUNCTION__, ##__VA_ARGS__)
#else
#define LogDebug(...) while(0)
#endif

// global constant definitions
NSInteger const eFTLConnectionNormal = 1000;
NSInteger const eFTLConnectionShutdown = 1001;
NSInteger const eFTLConnectionProtocol = 1002;
NSInteger const eFTLConnectionBadData = 1003;
NSInteger const eFTLConnectionError = 1006;
NSInteger const eFTLConnectionBadPayload = 1007;
NSInteger const eFTLConnectionPolicyViolation = 1008;
NSInteger const eFTLConnectionMessageTooLarge = 1009;
NSInteger const eFTLConnectionServerError = 1011;
NSInteger const eFTLConnectionFailedSSLHandshake = 1015;
NSInteger const eFTLConnectionForceClose = 4000;
NSInteger const eFTLConnectionNotAuthenticated = 4002;
NSInteger const eFTLBadSubscriptionId = 20;
NSInteger const eFTLPublishFailed = 11;
NSInteger const eFTLPublishDisallowed = 12;
NSInteger const eFTLSubscriptionDisallowed = 13;
NSInteger const eFTLSubscriptionFailed = 21;
NSInteger const eFTLSubscriptionInvalid = 22;

static NSString *const eFTLWebSocketProtocol = @"v1.eftl.tibco.com";

static NSString *const eFTLFieldOpCode       = @"op";
static NSString *const eFTLFieldUsername     = @"user";
static NSString *const eFTLFieldPassword     = @"password";
static NSString *const eFTLFieldClientId     = @"client_id";
static NSString *const eFTLFieldClientType   = @"client_type";
static NSString *const eFTLFieldClientVersion= @"client_version";
static NSString *const eFTLFieldTokenId      = @"id_token";
static NSString *const eFTLFieldTimeout      = @"timeout";
static NSString *const eFTLFieldMaxSize      = @"max_size";
static NSString *const eFTLFieldError        = @"err";
static NSString *const eFTLFieldReason       = @"reason";
static NSString *const eFTLFieldLoginOptions = @"login_options";
static NSString *const eFTLFieldId           = @"id";
static NSString *const eFTLFieldMatcher      = @"matcher";
static NSString *const eFTLFieldDurable      = @"durable";
static NSString *const eFTLFieldDurableType  = @"type";
static NSString *const eFTLFieldDurableKey   = @"key";
static NSString *const eFTLFieldTo           = @"to";
static NSString *const eFTLFieldBody         = @"body";
static NSString *const eFTLFieldSeqNum       = @"seq";
static NSString *const eFTLFieldQos          = @"_qos";
static NSString *const eFTLFieldResume       = @"_resume";
static NSString *const eFTLFieldHeartbeat    = @"heartbeat";

static NSInteger const eFTLOpHeartBeat       = 0;
static NSInteger const eFTLOpLogin           = 1;
static NSInteger const eFTLOpWelcome         = 2;
static NSInteger const eFTLOpSubscribe       = 3;
static NSInteger const eFTLOpSubscribed      = 4;
static NSInteger const eFTLOpUnsubscribe     = 5;
static NSInteger const eFTLOpUnsubscribed    = 6;
static NSInteger const eFTLOpEvent           = 7;
static NSInteger const eFTLOpMessage         = 8;
static NSInteger const eFTLOpAck             = 9;
static NSInteger const eFTLOpError           = 10;
static NSInteger const eFTLOpDisconnect      = 11;
static NSInteger const eFTLOpGoodbye         = 12;

@interface eFTL ()

+ (NSArray *)SSLTrustCertificates;

@end

@interface eFTLMessage ()

@property (nonatomic, readonly)NSString *from;

+ (eFTLMessage *)messageWithDictionary:(NSMutableDictionary *)dictionary;

- (NSMutableDictionary *)dictionary;

@end

@interface eFTLSubscription : NSObject

- (id)initWithIdentifier:(NSString *)identifier matcher:(NSString *)matcher durable:(NSString *)durable
              properties:(NSDictionary *)properties listener:(id<eFTLSubscriptionListener>)listener;

@property (nonatomic) NSString *identifier;
@property (nonatomic) NSString *matcher;
@property (nonatomic) NSString *durable;
@property (nonatomic) NSString *durableType;
@property (nonatomic) NSString *durableKey;
@property (nonatomic) BOOL pending;
@property (nonatomic) id<eFTLSubscriptionListener> listener;

@end

@interface eFTLSubscriptions : NSObject

- (id)init;

- (id)objectForKeyedSubscript:(id <NSCopying>)key;
- (void)setObject:(id)obj forKeyedSubscript:(id<NSCopying>)key;
- (void)removeObjectForKey:(id)key;
- (NSArray *)allValues;

@end

@interface eFTLPublish : NSObject;

- (id)initWithJSON:(NSString *)json seqNum:(NSNumber *)seqNum message:(eFTLMessage *)message listener:(id<eFTLCompletionListener>)listener;

@property (nonatomic) NSString *json;
@property (nonatomic) NSNumber *seqNum;
@property (nonatomic) eFTLMessage *message;
@property (nonatomic) id<eFTLCompletionListener> listener;

@end

@interface eFTLUnacknowledged : NSObject

- (id)init;

- (void)removeObjectsFromKey:(id)key usingBlock:(void (^)(eFTLPublish *publish))block;
- (void)setObject:(id)obj forKeyedSubscript:(id<NSCopying>)key;
- (void)removeAllObjectsUsingBlock:(void (^)(eFTLPublish *publish))block;
- (NSArray *)allValues;

@end

@interface eFTLConnection () <SRWebSocketDelegate>

@end

@implementation NSData (Conversion)

- (NSString *)hexString
{
    const unsigned char *buf = (const unsigned char *)[self bytes];
    
    if (!buf) {
        return [NSString string];
    }
    
    NSUInteger len = [self length];
    NSMutableString *hex = [NSMutableString stringWithCapacity:(len * 2)];
    
    for (int i = 0; i < len; i++) {
        [hex appendFormat:@"%02lx", (unsigned long)buf[i]];
    }
    
    return hex;
}

@end

@implementation eFTLConnection {
    volatile int32_t _subId;
    volatile int32_t _reqId;
    NSURL *_url;
    NSInteger _timeout;
    NSInteger _heartbeat;
    NSInteger _maxSize;
    NSTimer *_timer;
    NSMutableDictionary *_options;
    eFTLSubscriptions *_subscriptions;
    eFTLUnacknowledged *_unacknowledged;
    CFAbsoluteTime _lastActivity;
    SRWebSocket *_webSocket;
    eFTLConnection *_selfRetain;
    NSString *_clientId;
    NSString *_tokenId;
    NSRecursiveLock *_writeLock;
    NSTimer *_reconnectTimer;
    int _reconnectAttempts;
    long _sequenceCounter;
    long _lastSequenceNumber;
    BOOL _connected;
    BOOL _qos;
    BOOL _debug;
    BOOL _voip;
    BOOL _reconnecting;
    id<eFTLConnectionListener> _listener;
}

+ (NSString *)versionString
{
    return eFTLVersionStringLong;
}

+ (eFTLConnection *)connectionWithURL:(NSString *)url listener:(id<eFTLConnectionListener>)listener
{
    return [[self alloc] initWithURL:url listener:listener];
}

- (id)initWithURL:(NSString *)url listener:(id<eFTLConnectionListener>)listener
{
    self = [super init];
    if (self != nil) {
        _url = [NSURL URLWithString:url];
        _listener = listener;
        
        // one-time initializations
        _unacknowledged = [[eFTLUnacknowledged alloc] init];
        _subscriptions = [[eFTLSubscriptions alloc] init];
        _options = [NSMutableDictionary dictionary];
        _writeLock = [[NSRecursiveLock alloc] init];
        
        // default quality of service
        [_options setObject:@"true" forKey:eFTLFieldQos];
    }
    
    return self;
}

- (id)init
{
    return [self initWithURL:nil listener:nil];
}

- (void)connectWithProperties:(NSDictionary *)properties
{
    @synchronized(self) {
        if (_webSocket == nil) {
            NSTimeInterval timeout = 15.0;
            
            for (NSString *key in properties) {
                
                if ([key isEqualToString:@"debug"]) {
                    _debug = [properties[@"debug"] boolValue];
                }
                else if ([key isEqualToString:@"voip"]) {
                    _voip = [properties[@"voip"] boolValue];
                }
                else if ([key isEqualToString:eFTLPropertyTimeout]) {
                    timeout = [properties[eFTLPropertyTimeout] doubleValue];
                }
                else if ([key isEqualToString:eFTLPropertyAutoReconnectAttempts]) {
                    _options[key] = properties[key];
                }
                else if ([key isEqualToString:eFTLPropertyAutoReconnectMaxDelay]) {
                    _options[key] = properties[key];
                }
                else if ([properties[key] isKindOfClass:[NSData class]]) {
                    _options[key] = [properties[key] hexString];
                }
                else if ([properties[key] isKindOfClass:[NSString class]]) {
                    _options[key] = properties[key];
                }
            }

            LogDebug(_debug, @"Connecting to %@", _url);
            
            if ([[_url scheme] isEqualToString:@"ws"] == NO &&
                [[_url scheme] isEqualToString:@"wss"] == NO)
                [NSException raise:NSInvalidArgumentException format:@"Invalid URL"];

            // remove query string (not supported by older eFTL servers)
            NSURL *url = [NSURL URLWithString:[_url.absoluteString componentsSeparatedByString:@"?"][0]];
            
            NSMutableURLRequest *urlRequest = [NSMutableURLRequest requestWithURL:url cachePolicy:NSURLRequestUseProtocolCachePolicy timeoutInterval:timeout];
            
            NSArray *certificates = [eFTL SSLTrustCertificates];
            if (certificates != nil)
                urlRequest.SR_SSLPinnedCertificates = certificates;
            
            _webSocket = [[SRWebSocket alloc] initWithURLRequest:urlRequest
                                                       protocols:[NSArray arrayWithObject:eFTLWebSocketProtocol]];
            _webSocket.delegate = self;
            _webSocket.voip = _voip;
            
            [_webSocket open];
            
            // retain self until close is called
            _selfRetain = self;
        }
    }
}

- (BOOL)isConnected
{
    return _connected;
}

- (void)handleCloseWithCode:(NSInteger)code reason:(NSString *)reason
{
    _connected = NO;
    
    // clear the unacknowledged messages
    [_unacknowledged removeAllObjectsUsingBlock:^(eFTLPublish *publish) {
        if (publish.listener != nil) {
            [publish.listener message:publish.message didFailWithCode:eFTLPublishFailed reason:@"Closed"];
        }
    }];

    // invoke delegate method
    [_listener connection:self didDisconnectWithCode:code reason:reason];
    
    // release self retained in reconnect
    _selfRetain = nil;
}

- (void)closeWithCode:(NSInteger)code reason:(NSString *)reason
{
    LogDebug(_debug, @"Closing connection: code=%ld, reason=%@", (long)code, reason);
    
    NSMutableDictionary *closeMsg = [NSMutableDictionary dictionary];
    
    closeMsg[eFTLFieldOpCode] = @(eFTLOpDisconnect);
    closeMsg[eFTLFieldReason] = reason;
    
    [self sendJSONObject:closeMsg];

    [_webSocket closeWithCode:code reason:reason];
}

- (NSString *)getClientId
{
    return _clientId;
}

- (void)reconnectWithProperties:(NSDictionary *)properties
{
    if ([self isConnected])
        return;
    
    // set only when auto-reconnecting
    _reconnecting = NO;
    
    [self connectWithProperties:properties];
}

- (void)disconnect
{
    if ([self isConnected]) {
        if ([self cancelReconnect]) {
            [self handleCloseWithCode:eFTLConnectionNormal reason:@"user action"];
        } else {
            [self closeWithCode:eFTLConnectionNormal reason:@"user action"];
        }
    }
}

- (NSString *)subscribeWithMatcher:(NSString *)matcher
                          listener:(id<eFTLSubscriptionListener>)listener
{
    if (![self isConnected])
        return nil;

    id identifier = [NSString stringWithFormat:@"%@.s.%d", _clientId, OSAtomicIncrement32(&(_subId))];
    
    [self subscribeWithIdentifier:identifier matcher:matcher durable:nil properties:nil listener:listener];
    
    return identifier;
}

- (NSString *)subscribeWithMatcher:(NSString *)matcher
                           durable:(NSString *)durable
                          listener:(id<eFTLSubscriptionListener>)listener
{
    if (![self isConnected])
        return nil;

    id identifier = [NSString stringWithFormat:@"%@.s.%d", _clientId, OSAtomicIncrement32(&(_subId))];
    
    [self subscribeWithIdentifier:identifier matcher:matcher durable:durable properties:nil listener:listener];
    
    return identifier;
}

- (NSString *)subscribeWithMatcher:(NSString *)matcher
                           durable:(NSString *)durable
                        properties:(NSDictionary *)properties
                          listener:(id<eFTLSubscriptionListener>)listener
{
    if (![self isConnected])
        return nil;
    
    id identifier = [NSString stringWithFormat:@"%@.s.%d", _clientId, OSAtomicIncrement32(&(_subId))];
    
    [self subscribeWithIdentifier:identifier matcher:matcher durable:durable properties:properties listener:listener];
    
    return identifier;
}

- (void)subscribeWithIdentifier:(NSString *)identifier matcher:(NSString *)matcher durable:(NSString *)durable
                     properties:(NSDictionary *)properties listener:(id<eFTLSubscriptionListener>)listener
{
    eFTLSubscription *subscription = [[eFTLSubscription alloc] initWithIdentifier:identifier matcher:matcher
                                                                          durable:durable properties:properties listener:listener];

    _subscriptions[identifier] = subscription;
    
    [self subscribe:subscription asPending:YES];
}

- (void)subscribe:(eFTLSubscription *)subscription asPending:(BOOL)pending
{
    LogDebug(_debug, @"Subscription pending: identifier=%@, matcher=%@",
             subscription.identifier, subscription.matcher);
    
    NSMutableDictionary *subscribeMsg = [NSMutableDictionary dictionary];
    
    subscribeMsg[eFTLFieldOpCode] = @(eFTLOpSubscribe);
    subscribeMsg[eFTLFieldId] = subscription.identifier;
    
    if (subscription.matcher)
        subscribeMsg[eFTLFieldMatcher] = subscription.matcher;
    
    if (subscription.durable)
        subscribeMsg[eFTLFieldDurable] = subscription.durable;
    
    if (subscription.durableType)
        subscribeMsg[eFTLFieldDurableType] = subscription.durableType;
    
    if (subscription.durableKey)
        subscribeMsg[eFTLFieldDurableKey] = subscription.durableKey;
    
    subscription.pending = pending;
    
    [self sendJSONObject:subscribeMsg];
}

- (void)unsubscribe:(NSString *)subscriptionId
{
    if (![self isConnected] || subscriptionId == nil)
        return;

    LogDebug(_debug, @"Unsubscribe: identifier=%@", subscriptionId);
    
    NSMutableDictionary *unsubscribeMsg = [NSMutableDictionary dictionary];
    
    unsubscribeMsg[eFTLFieldOpCode] = @(eFTLOpUnsubscribe);
    unsubscribeMsg[eFTLFieldId] = subscriptionId;
    
    [self sendJSONObject:unsubscribeMsg];
    
    [_subscriptions removeObjectForKey:subscriptionId];
}

- (void)unsubscribeAll
{
    for (eFTLSubscription *subscription in [_subscriptions allValues])
        [self unsubscribe:subscription.identifier];
}

- (void)publishMessage:(eFTLMessage *)message
{
    if (![self isConnected])
        return;

    [self sendMessage:message listener:nil];
}

- (void)publishMessage:(eFTLMessage *)message
              listener:(id<eFTLCompletionListener>)listener
{
    if (![self isConnected])
        return;
    
    [self sendMessage:message listener:listener];
}

- (void)sendMessage:(eFTLMessage *)message
           listener:(id<eFTLCompletionListener>)listener
{
    NSMutableDictionary *envelope = [NSMutableDictionary dictionary];
    
    envelope[eFTLFieldOpCode] = @(eFTLOpMessage);
    envelope[eFTLFieldBody] = [message dictionary];

    [_writeLock lock];
    
    NSNumber *seqNum = @(++_sequenceCounter);
        
    if (_qos) {
        envelope[eFTLFieldSeqNum] = seqNum;
    }
    
    NSString *jsonString = [self serializeJSONObject:envelope];
        
    if (_maxSize > 0 && [jsonString length] > _maxSize)
    {
        // cleanup before throwing exception
        [_writeLock unlock];
        
        envelope = nil;
        seqNum = nil;
        jsonString = nil;
        
        [NSException raise:NSInvalidArgumentException format:@"Message exceeds maximum message size of %ld", (long)_maxSize];
    }
    
    eFTLPublish *publish = [[eFTLPublish alloc] initWithJSON:jsonString seqNum:seqNum message:message listener:listener];
        
    _unacknowledged[seqNum] = publish;

    [self sendJSONString:jsonString seqNum:seqNum];
    
    [_writeLock unlock];
}

- (void)sendJSONObject:(id)jsonObject
{
    [self sendJSONString:[self serializeJSONObject:jsonObject] seqNum:0];
}

- (void)sendJSONString:(NSString *)jsonString seqNum:(NSNumber *)seqNum
{
    LogDebug(_debug, @"Send \"%@\"", jsonString);
    
    if (_webSocket != nil && _webSocket.readyState == SR_OPEN) {
        [_webSocket send:jsonString];
        if (_qos == NO && seqNum > 0) {
            [_unacknowledged removeObjectsFromKey:seqNum usingBlock:^(eFTLPublish *publish){
                [publish.listener messageDidComplete:publish.message];
            }];
        }
    }
}

- (NSString *)serializeJSONObject:(id)jsonObj
{
    NSError *error = nil;
    NSString *jsonString = nil;
    
    NSData *jsonData = [NSJSONSerialization dataWithJSONObject:jsonObj options:0 error:&error];
    
    if (error == nil) {
        jsonString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
    } else {
        LogDebug(_debug, @"Serialization error: %@", error);
    }

    return jsonString;
}

- (void)ack:(NSNumber *)seqNum
{
    if (_qos == NO || seqNum == nil)
        return;
    
    NSMutableDictionary *ackMsg = [NSMutableDictionary dictionary];
    
    ackMsg[eFTLFieldOpCode] = @(eFTLOpAck);
    ackMsg[eFTLFieldSeqNum] = seqNum;
    
    [self sendJSONObject:ackMsg];
}

- (void)handleTimer:(NSTimer *)timer
{
    CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();
    
    if (now - _lastActivity > _timeout) {
        LogDebug(_debug, @"Connection timeout");
        
        [self closeWithCode:eFTLConnectionError reason:@"connection timeout"];
    }
}

- (void)handleWelcome:(NSDictionary *)jsonObj
{
    BOOL didConnect = (_clientId == nil);
    BOOL didReconnect = (_clientId != nil && !_reconnecting);
    
    _clientId  = jsonObj[eFTLFieldClientId];
    _tokenId   = jsonObj[eFTLFieldTokenId];
    _timeout   = [jsonObj[eFTLFieldTimeout] integerValue];
    _heartbeat = [jsonObj[eFTLFieldHeartbeat] integerValue];
    _maxSize   = [jsonObj[eFTLFieldMaxSize] integerValue];
    _qos       = [jsonObj[eFTLFieldQos] boolValue];
    
    BOOL resume = [jsonObj[eFTLFieldResume] boolValue];
    
    LogDebug(_debug, @"Received welcome: clientId=%@, tokenId=%@, timeout=%ld, maxSize=%ld, qos=%d, resume=%d",
             _clientId, _tokenId, (long)_timeout, (long)_maxSize, _qos, resume);
 
    if (_heartbeat > 0) {   
        _timer = [NSTimer scheduledTimerWithTimeInterval:_heartbeat
                                                  target:self
                                                selector:@selector(handleTimer:)
                                                userInfo:nil
                                                 repeats:YES];
    }
    
    _connected = YES;
    
    // reset reconnect attempts
    _reconnectAttempts = 0;
    
    // repair the subscriptions
    for (eFTLSubscription *subscription in [_subscriptions allValues])
        [self subscribe:subscription asPending:didReconnect];
    
    [_writeLock lock];
    
    if (resume) {
        // resend the unacknowledged messages
        for (eFTLPublish *publish in [_unacknowledged allValues])
            [self sendJSONString:publish.json seqNum:publish.seqNum];
    } else {
        // clear unacknowledged messages
        [_unacknowledged removeAllObjectsUsingBlock:^(eFTLPublish *publish) {
            if (publish.listener != nil) {
                [publish.listener message:publish.message didFailWithCode:eFTLPublishFailed reason:@"Reconnect"];
            }
        }];
        
        // reset the last sequence number
        _lastSequenceNumber = 0;
    }
    
    _reconnecting = NO;
    
    [_writeLock unlock];
    
    // invoke connect/reconnect callback if not auto-reconnecting
    if (didConnect)
        [_listener connectionDidConnect:self];
    else if (didReconnect && [_listener respondsToSelector:@selector(connectionDidReconnect:)])
        [_listener connectionDidReconnect:self];
}

- (void)handleSubscribed:(NSDictionary *)jsonObj
{
    NSString *identifier = jsonObj[eFTLFieldId];
 
    LogDebug(_debug, @"Received subscribe: identifier=%@", identifier);
    
    eFTLSubscription *subscription;
    
    subscription = _subscriptions[identifier];
    
    if (subscription != nil && subscription.pending) {
        subscription.pending = NO;
        if ([subscription.listener respondsToSelector:@selector(subscriptionDidSubscribe:)])
            [subscription.listener subscriptionDidSubscribe:identifier];
    }
}

- (void)handleUnsubscribed:(NSDictionary *)jsonObj
{
    NSString *identifier = jsonObj[eFTLFieldId];
    NSInteger code = [jsonObj[eFTLFieldError] integerValue];
    NSString *reason = jsonObj[eFTLFieldReason];
    
    LogDebug(_debug, @"Received unsubscribe: identifier=%@, code=%ld, reason=\"%@\"",
             identifier, (long)code, reason);
    
    eFTLSubscription *subscription;
    
    subscription = _subscriptions[identifier];
    [_subscriptions removeObjectForKey:identifier];
    
    if (subscription != nil)
        [subscription.listener subscription:identifier didFailWithCode:code reason:reason];
}

- (void)handleEvents:(NSArray *)jsonObj
{
    NSMutableArray *array = [NSMutableArray arrayWithCapacity:[jsonObj count]];
    eFTLSubscription *prevSubscription = nil;
    NSNumber *lastSeqNum = nil;
    
    for (NSDictionary *dict in jsonObj) {
        NSString *identifier = dict[eFTLFieldTo];
        NSNumber *seqNum = dict[eFTLFieldSeqNum];
        NSMutableDictionary *body = dict[eFTLFieldBody];

        // The message will be processed if qos is not enabled, there is no
        // sequence number, or if the sequence number is greater than the last
        // received sequence number.

        if (_qos == NO || seqNum == nil || seqNum.longValue > _lastSequenceNumber) {
        
            eFTLSubscription *subscription = nil;
        
            subscription = _subscriptions[identifier];
        
            if (subscription != nil) {
                if (prevSubscription != nil && prevSubscription != subscription) {
                    [prevSubscription.listener didReceiveMessages:array];
                    [array removeAllObjects];
                }
            
                prevSubscription = subscription;
                
                [array addObject:[eFTLMessage messageWithDictionary:body]];
            }
            
            // track the last received sequence number only if qos is enabled
            if (_qos && seqNum != nil)
                _lastSequenceNumber = seqNum.longValue;
        }
        
        if (seqNum != nil)
            lastSeqNum = seqNum;
    }
    
    if (prevSubscription != nil && [array count] > 0) {
        [prevSubscription.listener didReceiveMessages:array];
        [array removeAllObjects];
    }
    
    // Send an acknowledgement for the last sequence number in the array.
    // The server will acknowledge all messages less than or equal to
    // this sequence number.
    
    [self ack:lastSeqNum];
}

- (void)handleEvent:(NSDictionary *)jsonObj
{
    NSString *identifier = jsonObj[eFTLFieldTo];
    NSNumber *seqNum = jsonObj[eFTLFieldSeqNum];
    NSMutableDictionary *body = jsonObj[eFTLFieldBody];
    
    // The message will be processed if qos is not enabled, there is no
    // sequence number, or if the sequence number is greater than the last
    // received sequence number.
    
    if (_qos == NO || seqNum == nil || seqNum.longValue > _lastSequenceNumber) {
    
        eFTLSubscription *subscription;
    
        subscription = _subscriptions[identifier];
    
        if (subscription != nil)
            [subscription.listener didReceiveMessages:[NSArray arrayWithObject:[eFTLMessage messageWithDictionary:body]]];
        
        // track the last received sequence number only if qos is enabled
        if (_qos && seqNum != nil)
            _lastSequenceNumber = seqNum.longValue;
    }
    
    [self ack:seqNum];
}

- (void)handleError:(NSDictionary *)jsonObj
{
    NSInteger code = [jsonObj[eFTLFieldError] integerValue];
    NSString *reason = jsonObj[eFTLFieldReason];
    
    LogDebug(_debug, @"Received error: code=%ld, reason=\"%@\"", (long)code, reason);
    
    [_listener connection:self didFailWithCode:code reason:reason];
}

- (void)handleGoodbye:(NSDictionary *)jsonObj
{
    NSInteger code = [jsonObj[eFTLFieldError] integerValue];
    NSString *reason = jsonObj[eFTLFieldReason];
    
    LogDebug(_debug, @"Received goodbye: code=%ld, reason=\"%@\"", (long)code, reason);
    
    [self closeWithCode:code reason:reason];
}

- (void)handleAck:(NSDictionary *)jsonObj
{
    NSNumber *seqNum = jsonObj[eFTLFieldSeqNum];
    NSNumber *code = jsonObj[eFTLFieldError];
    NSString *reason = jsonObj[eFTLFieldReason];
    
    LogDebug(_debug, @"Received ack: seq=%ld", seqNum.longValue);
    
    [_unacknowledged removeObjectsFromKey:seqNum usingBlock:^(eFTLPublish *publish){
        if (publish.listener != nil) {
            if (code == nil) {
                [publish.listener messageDidComplete:publish.message];
            } else {
                [publish.listener message:publish.message didFailWithCode:code.integerValue reason:reason];
            }
        } else if (code != nil) {
            [_listener connection:self didFailWithCode:code.integerValue reason:reason];
        }
    }];
}

- (void)handleHeartbeat:(NSDictionary *)jsonObj
{
    LogDebug(_debug, @"Received heartbeat");
    
    [self sendJSONObject:jsonObj];
}

- (void)webSocket:(SRWebSocket *)webSocket didReceiveMessage:(id)message
{
    _lastActivity = CFAbsoluteTimeGetCurrent();

    NSError *error;
    
    id jsonObj = [NSJSONSerialization JSONObjectWithData:[message dataUsingEncoding:NSUTF8StringEncoding]
                                                 options:NSJSONReadingMutableContainers
                                                   error:&error];
    
    if ([jsonObj isKindOfClass:[NSArray class]]) {
        [self handleEvents:jsonObj];
    } else if ([jsonObj isKindOfClass:[NSDictionary class]]){
        NSNumber *opCode = jsonObj[eFTLFieldOpCode];
        
        switch (opCode.integerValue) {
            case eFTLOpHeartBeat:
                [self handleHeartbeat:jsonObj];
                break;
            case eFTLOpWelcome:
                [self handleWelcome:jsonObj];
                break;
            case eFTLOpSubscribed:
                [self handleSubscribed:jsonObj];
                break;
            case eFTLOpUnsubscribed:
                [self handleUnsubscribed:jsonObj];
                break;
            case eFTLOpEvent:
                [self handleEvent:jsonObj];
                break;
            case eFTLOpError:
                [self handleError:jsonObj];
                break;
            case eFTLOpGoodbye:
                [self handleGoodbye:jsonObj];
                break;
            case eFTLOpAck:
                [self handleAck:jsonObj];
                break;
            default:
                LogDebug(_debug, @"Received unexpected op code %ld", (long)opCode.integerValue);
                break;
        }
    } else {
        [self closeWithCode:eFTLConnectionBadData reason:@"invalid data"];
    }
}

- (void)webSocketDidOpen:(SRWebSocket *)webSocket
{
    LogDebug(_debug, @"Connection opened");

    NSString *username = _options[eFTLPropertyUsername];
    NSString *password = _options[eFTLPropertyPassword];
    NSString *clientId = _options[eFTLPropertyClientId];
    
    if (_url.user != nil) {
        username = _url.user;
    }
    if (_url.password != nil) {
        password = _url.password;
    }
    if (_url.query != nil) {
        for (NSString *pair in [_url.query componentsSeparatedByString:@"&"]) {
            NSArray *query = [pair componentsSeparatedByString:@"="];
            if ([query[0] isEqualToString:@"clientId"] && [query count] == 2) {
                clientId = query[1];
            }
        }
    }
    
    NSMutableDictionary *loginMsg = [NSMutableDictionary dictionary];
    
    loginMsg[eFTLFieldOpCode] = @(eFTLOpLogin);
    loginMsg[eFTLFieldClientType] = @"obj-c";
    loginMsg[eFTLFieldClientVersion] = eFTLVersionStringShort;
    loginMsg[eFTLFieldUsername] = (username != nil ? username : @"");
    loginMsg[eFTLFieldPassword] = (password != nil ? password : @"");
    
    NSMutableDictionary *loginOptions = [NSMutableDictionary dictionary];
    
    for (id key in _options) {
        if ([key isEqualToString:eFTLPropertyUsername] ||
            [key isEqualToString:eFTLPropertyPassword] ||
            [key isEqualToString:eFTLPropertyClientId]) {
            continue;
        }
        id value = [_options objectForKey:key];
        [loginOptions setValue:value forKey:key];
    }
    
    // set resume when auto-reconnecting
    loginOptions[eFTLFieldResume] = (_reconnecting ? @"true" : @"false");

    loginMsg[eFTLFieldLoginOptions] = loginOptions;
    
    if (_clientId != nil && _tokenId != nil) {
        loginMsg[eFTLFieldClientId] = _clientId;
        loginMsg[eFTLFieldTokenId] = _tokenId;
    } else if (clientId != nil) {
        loginMsg[eFTLFieldClientId] = clientId;
    }

    [self sendJSONObject:loginMsg];
}

- (void)webSocket:(SRWebSocket *)webSocket didCloseWithCode:(NSInteger)code reason:(NSString *)reason wasClean:(BOOL)wasClean
{
    LogDebug(_debug, @"Connection closed: code=%ld, reason=\"%@\", clean=%d", (long)code, reason, wasClean);

    // stop the connection timer
    [_timer invalidate];
    _timer = nil;
    
    // done with the WebSocket connection
    _webSocket.delegate = nil;
    _webSocket = nil;
    
    if (wasClean || [self scheduleReconnect] == NO) {
        [self handleCloseWithCode:code reason:reason];
    }
}

- (void)webSocket:(SRWebSocket *)webSocket didFailWithError:(NSError *)error
{
    LogDebug(_debug, @"Connection failed: %@", error);

    // stop the connection timer
    [_timer invalidate];
    _timer = nil;
    
    // done with the WebSocket connection
    _webSocket.delegate = nil;
    _webSocket = nil;
    
    if ([self scheduleReconnect] == NO) {
        [self handleCloseWithCode:[error code] reason:[error localizedDescription]];
    }
}

- (void)reconnect:(NSTimer *)timer
{
    // set only when auto-reconnectings
    _reconnecting = YES;
    
    [self connectWithProperties:nil];
}

- (BOOL)scheduleReconnect
{
    BOOL reconnecting = NO;
    
    if (_connected && _reconnectAttempts < [self autoReconnectAttempts])
    {
        double backoff = MIN(pow(2.0, _reconnectAttempts++), [self autoReconnectMaxDelay]);
        
        LogDebug(_debug, @"Reconnecting in %.2f seconds", backoff);

        _reconnectTimer = [NSTimer scheduledTimerWithTimeInterval:backoff
                                                           target:self
                                                         selector:@selector(reconnect:)
                                                         userInfo:nil
                                                          repeats:NO];
        reconnecting = YES;
    }
    
    return reconnecting;
}

- (BOOL)cancelReconnect
{
    BOOL cancelled = NO;
    
    if ([_reconnectTimer isValid])
    {
        [_reconnectTimer invalidate];
        _reconnectTimer = nil;
        
        cancelled = YES;
    }
    
    return cancelled;
}

- (int)autoReconnectAttempts
{
    int autoReconnectAttempts = 5; // defaults to 5 attempts
    
    NSNumber *autoReconnectAttemptsOption = _options[eFTLPropertyAutoReconnectAttempts];
    if (autoReconnectAttemptsOption != nil)
        autoReconnectAttempts = autoReconnectAttemptsOption.intValue;
    
    return autoReconnectAttempts;
}

- (double)autoReconnectMaxDelay
{
    double autoReconnectMaxDelay = 30.0; // defaults to 30 seconds
    
    NSNumber *autoReconnectMaxDelayOption = _options[eFTLPropertyAutoReconnectMaxDelay];
    if (autoReconnectMaxDelayOption != nil)
        autoReconnectMaxDelay = autoReconnectMaxDelayOption.doubleValue;
    
    return autoReconnectMaxDelay;
}

@end

@implementation eFTLPublish

- (id)initWithJSON:(NSString *)json seqNum:(NSNumber *)seqNum message:(eFTLMessage *)message listener:(id<eFTLCompletionListener>)listener
{
    self = [super init];
    if (self != nil) {
        _json = json;
        _seqNum = seqNum;
        _message = message;
        _listener = listener;
    }
    
    return self;
}

@end

@implementation eFTLSubscription

- (id)initWithIdentifier:(NSString *)identifier matcher:(NSString *)matcher durable:(NSString *)durable properties:(NSDictionary *)properties listener:(id<eFTLSubscriptionListener>)listener
{
    self = [super init];
    if (self != nil) {
        _identifier = [identifier copy];
        _matcher = [matcher copy];
        _durable = [durable copy];
        _durableType = [properties[eFTLPropertyDurableType] copy];
        _durableKey = [properties[eFTLPropertyDurableKey] copy];
        _listener = listener;
        _pending = NO;
    }
    
    return self;
}

@end

@implementation eFTLSubscriptions {
    OSSpinLock _spinLock;
    NSMutableDictionary *_dictionary;
}

- (id)init
{
    self = [super init];
    if (self != nil) {
        _spinLock = OS_SPINLOCK_INIT;
        _dictionary = [NSMutableDictionary dictionary];
    }
    
    return self;
}

- (id)objectForKeyedSubscript:(id<NSCopying>)key
{
    id obj;
    
    OSSpinLockLock(&_spinLock);
    
    obj = _dictionary[key];
    
    OSSpinLockUnlock(&_spinLock);
    
    return obj;
}

- (void)setObject:(id)obj forKeyedSubscript:(id<NSCopying>)key
{
    OSSpinLockLock(&_spinLock);
    
    _dictionary[key] = obj;
    
    OSSpinLockUnlock(&_spinLock);
}

- (void)removeObjectForKey:(id)key
{
    OSSpinLockLock(&_spinLock);
    
    [_dictionary removeObjectForKey:key];
    
    OSSpinLockUnlock(&_spinLock);
}

- (NSArray *)allValues
{
    NSArray *values;
    
    OSSpinLockLock(&_spinLock);
    
    values = [_dictionary allValues];
    
    OSSpinLockUnlock(&_spinLock);
    
    return values;
}

@end

@implementation eFTLUnacknowledged {
    OSSpinLock _spinLock;
    NSMutableDictionary *_dictionary;
    NSMutableOrderedSet *_keys;
}

- (id)init
{
    self = [super init];
    if (self != nil) {
        _spinLock = OS_SPINLOCK_INIT;
        _dictionary = [NSMutableDictionary dictionary];
        _keys = [[NSMutableOrderedSet alloc] init];
    }
    
    return self;
}

- (void)setObject:(id)obj forKeyedSubscript:(id<NSCopying>)key
{
    OSSpinLockLock(&_spinLock);
    
    if ([_dictionary objectForKey:key] == nil)
        [_keys addObject:key];
    [_dictionary setObject:obj forKey:key];
    
    OSSpinLockUnlock(&_spinLock);
}

- (void)removeObjectsFromKey:(id)key usingBlock:(void (^)(eFTLPublish *publish))block
{
    OSSpinLockLock(&_spinLock);
    
    id firstKey = [_keys firstObject];
    while (firstKey != nil && [firstKey longValue] <= [key longValue]) {
        eFTLPublish *publish = _dictionary[firstKey];
        if (publish != nil) {
            block(publish);
        }
        [_dictionary removeObjectForKey:firstKey];
        [_keys removeObjectAtIndex:0];
        firstKey = [_keys firstObject];
    }
    
    OSSpinLockUnlock(&_spinLock);
}

- (void)removeAllObjectsUsingBlock:(void (^)(eFTLPublish *publish))block
{
    OSSpinLockLock(&_spinLock);
    
    for (id key in _keys.array) {
        eFTLPublish *publish = _dictionary[key];
        if (publish != nil) {
            block(publish);
        }
    }
    
    [_dictionary removeAllObjects];
    [_keys removeAllObjects];
    
    OSSpinLockUnlock(&_spinLock);
}

- (NSArray *)allValues
{
    NSArray *values;
    
    OSSpinLockLock(&_spinLock);
    
    values = [_dictionary objectsForKeys:[_keys array] notFoundMarker:[NSNull null]];
    
    OSSpinLockUnlock(&_spinLock);
    
    return values;
}

@end
