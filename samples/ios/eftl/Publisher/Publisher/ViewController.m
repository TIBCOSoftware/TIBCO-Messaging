//
//  ViewController.m
//  Publisher
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

@interface ViewController () <eFTLConnectionListener, eFTLCompletionListener>

@property eFTLConnection *connection;

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
    NSDictionary *props = @{
                            eFTLPropertyClientId: [[UIDevice currentDevice] name],
                            eFTLPropertyUsername: EFTL_USERNAME,
                            eFTLPropertyPassword: EFTL_PASSWORD
                            };
    
    // Asynchronously connect to the eFTL server.
    [eFTL connect:EFTL_SERVER_URL properties:props listener:self];
}

- (void)reconnect {
    // Asynchronously reconnect to the eFTL server.
    if (self.connection != nil) {
        [self.connection reconnectWithProperties:nil];
    }
}

- (void)disconnect {
    // Asynchronously disconnect from the eFTL server.
    [self.connection disconnect];
}

- (void)publish {
    eFTLMessage *message = [eFTLMessage message];
    
    // Publish messages with a destination field.
    //
    // When connected to an EMS channel the destination field must
    // be present and set to the EMS topic on which to publish the
    // message.
    //
    [message setField:eFTLFieldNameDestination asString:@"sample"];
    
    // Add additional fields to the message.
    [message setField:@"text" asString:@"This is a sample eFTL message"];
    
    // Asynchronously publish the message to the eFTL server.
    [self.connection publishMessage:message listener:self];
}

- (IBAction)buttonPressed:(id)sender {
    // If not yet connected, start the connection to the
    // eFTL server. Once connected a message will be
    // published. Otherwise, publish a message.
    if (self.connection == nil) {
        [self connect];
    } else {
        [self publish];
    }
}

// Invoked when a connection to the eFTL server is successful.
- (void)connectionDidConnect:(eFTLConnection *)connection {
    NSLog(@"Connected to eFTL server");
    self.connection = connection;
    [self publish];
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

// Invoked when the message was successfully published.
- (void)messageDidComplete:(eFTLMessage *)message
{
    NSLog(@"Published message: %@", message);
}

// Invoked when the message was unsuccessfully published.
- (void)message:(eFTLMessage *)message didFailWithCode:(NSInteger)code reason:(NSString *)reason
{
    NSLog(@"Publish failed: %@", reason);
}

@end
