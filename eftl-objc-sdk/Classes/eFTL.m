//
//  Copyright (c) 2001-$Date: 2018-02-05 16:15:48 -0800 (Mon, 05 Feb 2018) $ TIBCO Software Inc.
//  Licensed under a BSD-style license. Refer to [LICENSE]
//  For more information, please contact:
//  TIBCO Software Inc., Palo Alto, California USA
//
//  $Id: eFTL.m 99237 2018-02-06 00:15:48Z bpeterse $
//

#import "eFTL.h"

// Connection properties
NSString *const eFTLPropertyNotificationToken     = @"notification_token";
NSString *const eFTLPropertyNotificationType      = @"notification_type";
NSString *const eFTLPropertyClientId              = @"client_id";
NSString *const eFTLPropertyUsername              = @"user";
NSString *const eFTLPropertyPassword              = @"password";
NSString *const eFTLPropertyTimeout               = @"timeout";
NSString *const eFTLPropertyAutoReconnectAttempts = @"autoreconenct_attempts";
NSString *const eFTLPropertyAutoReconnectMaxDelay = @"autoreconnect_max_delay";
NSString *const eFTLPropertyDurableType           = @"type";
NSString *const eFTLPropertyDurableKey            = @"key";

NSString *const eFTLDurableTypeShared             = @"shared";
NSString *const eFTLDurableTypeLastValue          = @"last-value";

static NSArray *eFTLTrustCertificiates = nil;

@interface eFTLConnection ()

+ (NSString *)versionString;
+ (eFTLConnection *)connectionWithURL:(NSString *)url listener:(id<eFTLConnectionListener>)listener;
- (void)connectWithProperties:(NSDictionary *)properties;

@end

@implementation eFTL

+ (NSString *)version
{
    return [eFTLConnection versionString];
}

+ (void)setSSLTrustCertificates:(NSArray *)certificates
{
    eFTLTrustCertificiates = [NSArray arrayWithArray:certificates];
}

+ (NSArray *)SSLTrustCertificates
{
    return eFTLTrustCertificiates;
}

+ (void)connect:(NSString *)url properties:(NSDictionary *)properties listener:(id<eFTLConnectionListener>)listener
{
    eFTLConnection *connection = [eFTLConnection connectionWithURL:url listener:listener];
    
    [connection connectWithProperties:properties];
}

@end
