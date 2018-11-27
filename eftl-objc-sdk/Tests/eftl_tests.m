//
//  Copyright (c) 2001-$Date: 2014-04-04 12:41:57 -0500 (Fri, 04 Apr 2014) $ TIBCO Software Inc.
//  Licensed under a BSD-style license. Refer to [LICENSE]
//  For more information, please contact:
//  TIBCO Software Inc., Palo Alto, California USA
//
//  $Id: eftl_tests.m 72885 2014-04-04 17:41:57Z bpeterse $
//

#import "eftl_tests.h"

NSString *const URI = @"ws://localhost:9191/test";

typedef BOOL (^PredicateBlock)();

@interface SubscriptionListener : NSObject <eFTLSubscriptionListener>

@property int messageCount;
@property int subscriptionCount;

@end

@interface RequestListener : NSObject <eFTLSubscriptionListener>

@property eFTLConnection *connection;
@property int messageCount;
@property int subscriptionCount;

@end

@interface ConnectionListener : NSObject <eFTLConnectionListener>

@property eFTLConnection *connection;

- (void)connect:(NSString *)URI options:(NSDictionary *)options;
- (void)reconnect:(NSDictionary *)options;
- (void)disconnect;
- (BOOL)isConnected;

@end

@implementation eftlTests {
    NSMutableDictionary *_props;
    ConnectionListener *_pub;
    ConnectionListener *_sub;
}

- (void)setUp
{
    [super setUp];
    
    // Set-up code here.
    
    _props = [NSMutableDictionary dictionary];
    [_props setObject:@"user" forKey:eFTLPropertyUsername];
    [_props setObject:@"password" forKey:eFTLPropertyPassword];
    [_props setObject:@"10.0" forKey:eFTLPropertyTimeout];
    [_props setObject:@"true" forKey:@"debug"];
    
    _pub = [[ConnectionListener alloc] init];
    _sub = [[ConnectionListener alloc] init];
}

- (void)tearDown
{
    // Tear-down code here.
    [_pub disconnect];
    [_sub disconnect];
    
    _pub = nil;
    _sub = nil;
    
    [super tearDown];
}

- (void)testConnect
{
    [_pub connect:URI options:_props];
    
    [self runLoopUntil:^BOOL{return [_pub isConnected] == YES;} timeout:5.0 okToFail:NO];
    
    STAssertTrue([_pub isConnected], @"Realm is not connected");

    [_pub disconnect];
    
    [self runLoopUntil:^BOOL{return [_pub isConnected] == NO;} timeout:5.0 okToFail:NO];
    
    STAssertFalse([_pub isConnected], @"Realm is connected");
}

- (void)testReconnect
{
    [_pub connect:URI options:_props];
    
    [self runLoopUntil:^BOOL{return [_pub isConnected] == YES;} timeout:5.0 okToFail:NO];
    
    STAssertTrue([_pub isConnected], @"Realm is not connected");
    
    [_pub disconnect];
    
    [self runLoopUntil:^BOOL{return [_pub isConnected] == NO;} timeout:5.0 okToFail:NO];
    
    STAssertFalse([_pub isConnected], @"Realm is connected");
    
    [_pub reconnect:_props];
    
    [self runLoopUntil:^BOOL{return [_pub isConnected] == YES;} timeout:5.0 okToFail:NO];
    
    STAssertTrue([_pub isConnected], @"Realm is not connected");
    
    [_pub disconnect];
    
    [self runLoopUntil:^BOOL{return [_pub isConnected] == NO;} timeout:5.0 okToFail:NO];
    
    STAssertFalse([_pub isConnected], @"Realm is connected");
}

- (void)testBroadcast
{
    [_pub connect:URI options:_props];
    [_sub connect:URI options:_props];
    
    [self runLoopUntil:^BOOL{return [_pub isConnected] == YES;} timeout:5.0 okToFail:NO];
    [self runLoopUntil:^BOOL{return [_sub isConnected] == YES;} timeout:5.0 okToFail:NO];
    
    STAssertTrue([_pub isConnected], @"Realm is not connected");
    STAssertTrue([_sub isConnected], @"Realm is not connected");
    
    SubscriptionListener *listener = [[SubscriptionListener alloc] init];
    
    NSString *subId = [_sub.connection subscribeWithMatcher:nil listener:listener];
    
    [self runLoopUntil:^BOOL{return listener.subscriptionCount == 1;} timeout:5.0 okToFail:NO];
    
    STAssertEquals(listener.subscriptionCount, 1, @"Subscription failed");
    
    eFTLMessage *message = [eFTLMessage message];
    [message setField:@"string" asString:@"foo"];
    [message setField:@"number" asLong:[NSNumber numberWithLong:42]];
    
    [_pub.connection publishMessage:message];
    
    [self runLoopUntil:^BOOL{return listener.messageCount == 1;} timeout:5.0 okToFail:NO];
    
    STAssertEquals(listener.messageCount, 1, @"Broadcast failed");
    
    [_sub.connection unsubscribe:subId];
    
    [_pub.connection publishMessage:message];

    [self runLoopUntil:^BOOL{return listener.messageCount > 1;} timeout:1.0 okToFail:YES];
    
    STAssertEquals(listener.messageCount, 1, @"Unsubscribe failed");
    
    [_pub disconnect];
    [_sub disconnect];
    
    [self runLoopUntil:^BOOL{return [_pub isConnected] == NO;} timeout:5.0 okToFail:NO];
    [self runLoopUntil:^BOOL{return [_sub isConnected] == NO;} timeout:5.0 okToFail:NO];
    
    STAssertFalse([_pub isConnected], @"Realm is connected");
    STAssertFalse([_sub isConnected], @"Realm is connected");
}

- (void)testMatcher
{
    [_pub connect:URI options:_props];
    [_sub connect:URI options:_props];
    
    [self runLoopUntil:^BOOL{return [_pub isConnected] == YES;} timeout:5.0 okToFail:NO];
    [self runLoopUntil:^BOOL{return [_sub isConnected] == YES;} timeout:5.0 okToFail:NO];
    
    STAssertTrue([_pub isConnected], @"Realm is not connected");
    STAssertTrue([_sub isConnected], @"Realm is not connected");
    
    SubscriptionListener *listener = [[SubscriptionListener alloc] init];
    
    NSString *subId = [_sub.connection subscribeWithMatcher:@"{\"string\":\"foo\",\"number\":42}" listener:listener];
    
    [self runLoopUntil:^BOOL{return listener.subscriptionCount == 1;} timeout:5.0 okToFail:NO];
    
    STAssertEquals(listener.subscriptionCount, 1, @"Subscription failed");
    
    eFTLMessage *message = [eFTLMessage message];
    [message setField:@"string" asString:@"foo"];
    [message setField:@"number" asLong:[NSNumber numberWithLong:42]];
    
    [_pub.connection publishMessage:message];

    [self runLoopUntil:^BOOL{return listener.messageCount == 1;} timeout:5.0 okToFail:NO];
    
    STAssertEquals(listener.messageCount, 1, @"Matcher failed");
    
    [_sub.connection unsubscribe:subId];
    
    [_pub.connection publishMessage:message];
    
    [self runLoopUntil:^BOOL{return listener.messageCount > 1;} timeout:1.0 okToFail:YES];
    
    STAssertEquals(listener.messageCount, 1, @"Unsubscribe failed");

    [_pub disconnect];
    [_sub disconnect];
    
    [self runLoopUntil:^BOOL{return [_pub isConnected] == NO;} timeout:5.0 okToFail:NO];
    [self runLoopUntil:^BOOL{return [_sub isConnected] == NO;} timeout:5.0 okToFail:NO];
    
    STAssertFalse([_pub isConnected], @"Realm is connected");
    STAssertFalse([_sub isConnected], @"Realm is connected");
}

- (void)testRequestReply
{
    [_pub connect:URI options:_props];
    [_sub connect:URI options:_props];
    
    [self runLoopUntil:^BOOL{return [_pub isConnected] == YES;} timeout:5.0 okToFail:NO];
    [self runLoopUntil:^BOOL{return [_sub isConnected] == YES;} timeout:5.0 okToFail:NO];
    
    STAssertTrue([_pub isConnected], @"Realm is not connected");
    STAssertTrue([_sub isConnected], @"Realm is not connected");

    RequestListener *requestListener = [[RequestListener alloc] init];
    
    requestListener.connection = _pub.connection;
    
    [_pub.connection subscribeWithMatcher:nil listener:requestListener];
    
    [self runLoopUntil:^BOOL{return requestListener.subscriptionCount == 1;} timeout:5.0 okToFail:NO];
    
    SubscriptionListener *replyListener = [[SubscriptionListener alloc] init];
    
    [_sub.connection setDefaultReplyListener:replyListener];
    
    [self runLoopUntil:^BOOL{return replyListener.subscriptionCount == 1;} timeout:5.0 okToFail:NO];
    
    STAssertEquals(replyListener.subscriptionCount, 1, @"Subscription failed");
    
    eFTLMessage *message = [eFTLMessage message];
    [message setField:@"string" asString:@"foo"];
    [message setField:@"number" asLong:[NSNumber numberWithLong:42]];
    [message setField:@"double" asDouble:[NSNumber numberWithDouble:3.14]];

    [_sub.connection sendRequest:message];

    // wait for request
    [self runLoopUntil:^BOOL{return requestListener.messageCount == 1;} timeout:5.0 okToFail:NO];
    
    // wait for reply
    [self runLoopUntil:^BOOL{return replyListener.messageCount == 1;} timeout:5.0 okToFail:NO];
    
    STAssertEquals(replyListener.messageCount, 1, @"Send request failed");
    
    [_sub.connection setDefaultReplyListener:nil];

    [_sub.connection sendRequest:message];
    
    // wait for request
    [self runLoopUntil:^BOOL{return requestListener.messageCount > 1;} timeout:1.0 okToFail:YES];
    
    // wait for reply
    [self runLoopUntil:^BOOL{return replyListener.messageCount > 1;} timeout:1.0 okToFail:YES];
    
    STAssertEquals(replyListener.messageCount, 1, @"Send request failed");
    
    [_pub disconnect];
    [_sub disconnect];
    
    [self runLoopUntil:^BOOL{return [_pub isConnected] == NO;} timeout:5.0 okToFail:NO];
    [self runLoopUntil:^BOOL{return [_sub isConnected] == NO;} timeout:5.0 okToFail:NO];
    
    STAssertFalse([_pub isConnected], @"Realm is connected");
    STAssertFalse([_sub isConnected], @"Realm is connected");
}

- (void)testReconnectWithSubscriptions
{
    [_pub connect:URI options:_props];
    [_sub connect:URI options:_props];
    
    [self runLoopUntil:^BOOL{return [_pub isConnected] == YES;} timeout:5.0 okToFail:NO];
    [self runLoopUntil:^BOOL{return [_sub isConnected] == YES;} timeout:5.0 okToFail:NO];
    
    STAssertTrue([_pub isConnected], @"Realm is not connected");
    STAssertTrue([_sub isConnected], @"Realm is not connected");
    
    RequestListener *requestListener = [[RequestListener alloc] init];
    
    requestListener.connection = _pub.connection;
    
    [_pub.connection subscribeWithMatcher:nil listener:requestListener];
    
    [self runLoopUntil:^BOOL{return requestListener.subscriptionCount == 1;} timeout:5.0 okToFail:NO];
    
    SubscriptionListener *listener = [[SubscriptionListener alloc] init];

    [_sub.connection setDefaultReplyListener:listener];
    [_sub.connection subscribeWithMatcher:@"{\"string\":\"foo\",\"number\":42}" listener:listener];
    
    [self runLoopUntil:^BOOL{return listener.subscriptionCount == 2;} timeout:5.0 okToFail:NO];
    
    STAssertEquals(listener.subscriptionCount, 2, @"Subscription failed");
    
    eFTLMessage *message = [eFTLMessage message];
    [message setField:@"string" asString:@"foo"];
    [message setField:@"number" asLong:[NSNumber numberWithLong:42]];
    [message setField:@"double" asDouble:[NSNumber numberWithDouble:3.14]];

    [_pub.connection publishMessage:message];
    
    [self runLoopUntil:^BOOL{return listener.messageCount == 1;} timeout:5.0 okToFail:NO];
    
    STAssertEquals(listener.messageCount, 1, @"Matcher failed");
    
    [_sub.connection sendRequest:message];
    
    [self runLoopUntil:^BOOL{return listener.messageCount == 2;} timeout:5.0 okToFail:NO];
    
    STAssertEquals(listener.messageCount, 2, @"Send request failed");
    
    [_sub disconnect];
    
    [self runLoopUntil:^BOOL{return [_sub isConnected] == NO;} timeout:5.0 okToFail:NO];
    
    STAssertFalse([_sub isConnected], @"Realm is connected");
    
    [_sub reconnect:_props];
    
    [self runLoopUntil:^BOOL{return [_sub isConnected] == YES;} timeout:5.0 okToFail:NO];
    
    STAssertTrue([_sub isConnected], @"Realm is not connected");
    
    [self runLoopUntil:^BOOL{return listener.subscriptionCount == 4;} timeout:5.0 okToFail:NO];
    
    STAssertEquals(listener.subscriptionCount, 4, @"Re-subscription failed");
    
    [_pub.connection publishMessage:message];
    
    [self runLoopUntil:^BOOL{return listener.messageCount == 3;} timeout:5.0 okToFail:NO];
    
    STAssertEquals(listener.messageCount, 3, @"Matcher failed");
    
    [_sub.connection sendRequest:message];
    
    [self runLoopUntil:^BOOL{return listener.messageCount == 4;} timeout:5.0 okToFail:NO];
    
    STAssertEquals(listener.messageCount, 4, @"Send request failed");
    
    [_pub disconnect];
    [_sub disconnect];
    
    [self runLoopUntil:^BOOL{return [_pub isConnected] == NO;} timeout:5.0 okToFail:NO];
    [self runLoopUntil:^BOOL{return [_sub isConnected] == NO;} timeout:5.0 okToFail:NO];
    
    STAssertFalse([_pub isConnected], @"Realm is connected");
    STAssertFalse([_sub isConnected], @"Realm is connected");
}

- (void)testHeavyBroadcast
{
    [_pub connect:URI options:_props];
    [_sub connect:URI options:_props];
    
    [self runLoopUntil:^BOOL{return [_pub isConnected] == YES;} timeout:5.0 okToFail:NO];
    [self runLoopUntil:^BOOL{return [_sub isConnected] == YES;} timeout:5.0 okToFail:NO];
    
    STAssertTrue([_pub isConnected], @"Realm is not connected");
    STAssertTrue([_sub isConnected], @"Realm is not connected");
    
    SubscriptionListener *listener = [[SubscriptionListener alloc] init];
    
    [_sub.connection subscribeWithMatcher:nil listener:listener];
    
    [self runLoopUntil:^BOOL{return listener.subscriptionCount == 1;} timeout:5.0 okToFail:NO];
    
    STAssertEquals(listener.subscriptionCount, 1, @"Subscription failed");
    
    eFTLMessage *message = [eFTLMessage message];
    [message setField:@"string" asString:@"foo"];
    [message setField:@"number" asLong:[NSNumber numberWithLong:42]];
    [message setField:@"double" asDouble:[NSNumber numberWithDouble:3.14]];

    for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 10; j++)
            [_pub.connection publishMessage:message];
        
        // keep throttling from kicking in
        [NSThread sleepForTimeInterval:0.01];
    }
    
    [self runLoopUntil:^BOOL{return listener.messageCount == 1000;} timeout:5.0 okToFail:NO];
    
    STAssertEquals(listener.messageCount, 1000, @"Broadcast failed");
    
    [_pub disconnect];
    [_sub disconnect];
    
    [self runLoopUntil:^BOOL{return [_pub isConnected] == NO;} timeout:5.0 okToFail:NO];
    [self runLoopUntil:^BOOL{return [_sub isConnected] == NO;} timeout:5.0 okToFail:NO];
    
    STAssertFalse([_pub isConnected], @"Realm is connected");
    STAssertFalse([_sub isConnected], @"Realm is connected");
}

- (void)runLoopUntil:(PredicateBlock)predicate timeout:(NSTimeInterval)timeout okToFail:(BOOL)okToFail;
{
    NSDate *timeoutDate = [NSDate dateWithTimeIntervalSinceNow:timeout];
    
    NSTimeInterval timeoutTime = [timeoutDate timeIntervalSinceReferenceDate];
    NSTimeInterval currentTime;
    
    for (currentTime = [NSDate timeIntervalSinceReferenceDate];
         !predicate() && currentTime < timeoutTime;
         currentTime = [NSDate timeIntervalSinceReferenceDate]) {
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
    }
    
    if (okToFail == NO)
        STAssertTrue(currentTime <= timeoutTime, @"Timed out");
}

@end

@implementation SubscriptionListener

- (void)didReceiveMessages:(NSArray *)messages
{
    _messageCount += [messages count];
    
    for (eFTLMessage *message in messages)
        NSLog(@"SubscriptionListener: %@", message);
}

- (void)subscriptionDidSubscribe:(NSString *)subscription
{
    _subscriptionCount++;
}

- (void)subscription:(NSString *)subscription didFailWithCode:(NSInteger)code reason:(NSString *)reason
{
    NSLog(@"Subscription error: %ld - %@", (long)code, reason);
}

@end

@implementation RequestListener

- (void)didReceiveMessages:(NSArray *)messages
{
    _messageCount += [messages count];
    
    for (eFTLMessage *message in messages) {
        NSLog(@"RequestListener: %@", message);
        if ([message isRequestMessage] == YES)
            [self.connection sendReply:message forRequest:message];
    }
}

- (void)subscriptionDidSubscribe:(NSString *)subscription
{
    _subscriptionCount++;
}

- (void)subscription:(NSString *)subscription didFailWithCode:(NSInteger)code reason:(NSString *)reason
{
    NSLog(@"Subscription error: %ld - %@", (long)code, reason);
}

@end

@implementation ConnectionListener

- (void)connect:(NSString *)URI options:(NSDictionary *)options
{
    if ([URI hasPrefix:@"wss"]) {
        NSMutableArray *array = [NSMutableArray array];
        
        NSBundle *bundle = [NSBundle bundleForClass:[self class]];
        
        NSString *path = [bundle pathForResource:@"server" ofType:@"cer"];
        NSData *data = [[NSData alloc] initWithContentsOfFile:path];
        SecCertificateRef cert = SecCertificateCreateWithData(NULL, (__bridge CFDataRef)data);
        [array addObject:(__bridge id)(cert)];
        
        [eFTL setSSLTrustCertificates:array];
    }
    
    [eFTL connect:URI properties:options listener:self];
}

- (void)disconnect
{
    [self.connection disconnect];
}

- (void)reconnect:(NSDictionary *)options
{
    [self.connection reconnectWithProperties:options];
}

- (BOOL)isConnected
{
    return [self.connection isConnected];
}

- (void)connectionDidConnect:(eFTLConnection *)connection
{
    self.connection = connection;
}

- (void)connection:(eFTLConnection *)connection didDisconnectWithCode:(NSInteger)code reason:(NSString *)reason
{
}

- (void)connection:(eFTLConnection *)connection didFailWithCode:(NSInteger)code reason:(NSString *)reason
{
}

@end
