//
//  ViewController.m
//  Subscriber
//
//  Created by Brent Petersen on 3/19/16.
//  Copyright © 2016 TIBCO Software. Licensed under a BSD-style license. Refer to [LICENSE]
//

#import "ViewController.h"
#import "AppDelegate.h"

#import <eFTL/eFTL.h>

static NSString *const EFTL_SERVER_URL = @"ws://localhost:9191/channel";
static NSString *const EFTL_USERNAME = @"user";
static NSString *const EFTL_PASSWORD = @"password";

@interface ViewController () <eFTLConnectionListener, eFTLSubscriptionListener>

@property eFTLConnection *connection;
@property BOOL subscribed;
@property (weak, nonatomic) IBOutlet UIButton *subscribeButton;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
    
    NSLog(@"%@", [eFTL version]);
    
    // Close the connection to the eFTL server when the application
    // moves into the background. Reconnect to the eFTL server when
    // the application moves into the foreground.
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(reconnect)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(disconnect)
                                                 name:UIApplicationDidEnterBackgroundNotification
                                               object:nil];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)connect {
    AppDelegate *app = (AppDelegate *)[[UIApplication sharedApplication] delegate];

    NSDictionary *props;
    
    if (app.deviceToken == nil) {
        props = @{
                  eFTLPropertyClientId: [[UIDevice currentDevice] name],
                  eFTLPropertyUsername: EFTL_USERNAME,
                  eFTLPropertyPassword: EFTL_PASSWORD
                  };
    } else {
        props = @{
                  eFTLPropertyClientId: [[UIDevice currentDevice] name],
                  eFTLPropertyUsername: EFTL_USERNAME,
                  eFTLPropertyPassword: EFTL_PASSWORD,
                  eFTLPropertyNotificationToken: app.deviceToken
                  };
    }

    [eFTL connect:EFTL_SERVER_URL properties:props listener:self];
}

- (void)reconnect {
    // Asynchronously reconnect to the eFTL server.
    [self.connection reconnectWithProperties:nil];
}

- (void)disconnect {
    // Asynchronously disconnect from the eFTL server.
    [self.connection disconnect];
}

- (void)subscribe {
    
    // Create a subscription matcher for messages containing a
    // destination field that matches the destination "sample".
    //
    // Specify a durable name to create a durable subscription.
    // 
    // When connected to an FTL channel the content matcher
    // can be used to match any field in a published message.
    // Only matching messages will be received by the
    // subscription. The content matcher can match on string
    // and integer fields, or test for the presence or absence
    // of a field by setting it's value to the boolean true or
    // false.
    //
    // When connected to an EMS channel the content matcher
    // must only contain the destination field "_dest" set to
    // the EMS topic on which to subscribe.
    //
    NSString *matcher = [NSString stringWithFormat:@"{\"%@\":\"%@\"}",
                         eFTLFieldNameDestination, @"sample"];
    
    // Asynchronously subscribe to messages with a content matcher.
    [self.connection subscribeWithMatcher:matcher durable:@"sample" listener:self];
}

- (void)unsubscribe {
    // Asynchronously unsubscribe from all subscriptions.
    [self.connection unsubscribeAll];
    [self.subscribeButton setTitle:@"Subscribe" forState:UIControlStateNormal];
    NSLog(@"Unsubscribed");
    self.subscribed = NO;
}

- (IBAction)buttonPressed:(id)sender {
    // If not yet connected, start the connection to the
    // eFTL server. Once connected, the subscription will
    // be made. Otherwise, either subscribe or unsubscribe
    // depending on the current state.
    if (self.connection == nil) {
        [self connect];
    } else if (self.subscribed){
        [self unsubscribe];
    } else {
        [self subscribe];
    }
}

// Invoked when a connection to the eFTL server is successful.
- (void)connectionDidConnect:(eFTLConnection *)connection {
    NSLog(@"Connected to eFTL server");
    self.connection = connection;
    [self subscribe];
}

// Invoked when reconnected to the eFTL server.
- (void)connectionDidReconnect:(eFTLConnection *)connection {
    NSLog(@"Reconnected to eFTl server");
}

// Invoked when a connection to the eFTL server is unsuccessful or lost.
- (void)connection:(eFTLConnection *)connection didDisconnectWithCode:(NSInteger)code reason:(NSString *)reason {
    NSLog(@"Disconnected from eFTL server: %@", (reason == nil ? @"entered background" : reason));
}

// Invoked when the eFTL server responds with an error.
- (void)connection:(eFTLConnection *)connection didFailWithCode:(NSInteger)code reason:(NSString *)reason {
    NSLog(@"Error from eFTL server: %@", reason);
}

// Invoked when the subscription receives messages.
- (void)didReceiveMessages:(NSArray *)messages {
    for (id message in messages) {
        NSLog(@"Received message: %@", message);
    }
}

// Invoked when the subscription is successful.
- (void)subscriptionDidSubscribe:(NSString *)subscription {
    NSLog(@"Subscribed");
    [self.subscribeButton setTitle:@"Unsubscribe" forState:UIControlStateNormal];
    self.subscribed = YES;
}

// Invoked when the subscription is unsuccessful.
- (void)subscription:(NSString *)subscription didFailWithCode:(NSInteger)code reason:(NSString *)reason {
    NSLog(@"Subscription failed: %@", reason);
}

@end
