//
//  AppDelegate.m
//  Subscriber
//
//  Created by Brent Petersen on 3/19/16.
//  Copyright © 2016 TIBCO Software. Licensed under a BSD-style license. Refer to [LICENSE]
//

#import "AppDelegate.h"

@interface AppDelegate ()

@end

@implementation AppDelegate


- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    // Override point for customization after application launch.
    
    // In order to receive push notifiations from the eFTL server the
    // application's project must:
    //
    // 1) have the Push Notifications capability enabled
    // 2) have Remote Notifications enabled in the Background Modes capability
    //
    // The application:registerForRemoteNotifications will request a device token from
    // the Apple Push Notification service. If successful, the
    // application:didRegisterForRemoteNotificationsWithDeviceToken: method
    // is called and is passed a device token. This device token must be sent
    // to the eFTL server when connecting or reconnecting by adding it to the
    // connection properties.
    //
/*
    [[UIApplication sharedApplication] registerForRemoteNotifications];
*/
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application {
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

- (void)applicationWillTerminate:(UIApplication *)application {
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

- (void)application:(UIApplication *)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData *)deviceToken {
    NSLog(@"Registered for remote notifications, device token %@", deviceToken);
    
    self.deviceToken = [deviceToken copy];
}

- (void)application:(UIApplication *)application didFailToRegisterForRemoteNotificationsWithError:(NSError *)error {
    NSLog(@"Error registering for remote notficiations: %@", [error localizedDescription]);
}

- (void)application:(UIApplication *)application didReceiveRemoteNotification:(NSDictionary *)userInfo fetchCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler {
    NSLog(@"Received remote notification");
    
    completionHandler(UIBackgroundFetchResultNewData);
}

@end
