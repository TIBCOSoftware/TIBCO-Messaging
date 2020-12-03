//
//  Copyright (c) 2001-$Date: 2017-01-30 11:23:42 -0800 (Mon, 30 Jan 2017) $ TIBCO Software Inc.
//  Licensed under a BSD-style license. Refer to [LICENSE]
//  For more information, please contact:
//  TIBCO Software Inc., Palo Alto, California USA
//
//  $Id: eFTLMessage.m 91118 2017-01-30 19:23:42Z bpeterse $
//

#import "eFTLMessage.h"

    /**
     * Message field name identifying the destination of a message. 
     * <p>
     * To publish a message on a specific destination include this 
     * message field using eFTLMessage::setString:.
     * <p>
     * To subscribe to messages published on a specific destination,
     * use a matcher that includes this message field name. 
     */
NSString *const eFTLFieldNameDestination = @"_dest";

static NSString *const FTLFieldNameDouble = @"_d_";
static NSString *const FTLFieldNameMillisecond = @"_m_";
static NSString *const FTLFieldNameOpaque = @"_o_";

@interface eFTLMessage ()

@end

@implementation eFTLMessage {
    NSMutableDictionary *_dict;
}

- (id)init
{
    return [self initWithDictionary:[NSMutableDictionary dictionary]];
}

- (id)initWithDictionary:(NSMutableDictionary *)dictionary
{
    self = [super init];
    if (self != nil) {
        _dict = dictionary;
    }
    
    return self;
}

+ (eFTLMessage *)message
{
    return [[self alloc] init];
}

+ (eFTLMessage *)messageWithDictionary:(NSMutableDictionary *)dictionary
{
    return [[self alloc] initWithDictionary:dictionary];
}

- (BOOL)isFieldSet:(NSString *)name
{
    return ([_dict objectForKey:name] != nil);
}

- (eFTLFieldType)getObjectType:(id)object
{
    eFTLFieldType type;
    
    if ([object isKindOfClass:[NSString class]]) {
        type = eFTLFieldTypeString;
    } else if ([object isKindOfClass:[NSNumber class]]) {
        type = eFTLFieldTypeLong;
    } else {
        if ([object objectForKey:FTLFieldNameDouble] != nil)
            type = eFTLFieldTypeDouble;
        else if ([object objectForKey:FTLFieldNameMillisecond] != nil)
            type = eFTLFieldTypeDate;
        else if ([object objectForKey:FTLFieldNameOpaque] != nil)
            type = eFTLFieldTypeOpaque;
        else
            type = eFTLFieldTypeMessage;
    }
    
    return type;
}

- (eFTLFieldType)getFieldType:(NSString *)name
{
    eFTLFieldType type;
    
    id obj = [_dict objectForKey:name];
    
    if ([obj isKindOfClass:[NSArray class]]) {
        NSArray *array = obj;
        if (array.count > 0) {
            type = [self getObjectType:[array objectAtIndex:0]];
            if (type == eFTLFieldTypeString)
                type = eFTLFieldTypeStringArray;
            else if (type == eFTLFieldTypeLong)
                type = eFTLFieldTypeLongArray;
            else if (type == eFTLFieldTypeDouble)
                type = eFTLFieldTypeDoubleArray;
            else if (type == eFTLFieldTypeDate)
                type = eFTLFieldTypeDateArray;
            else if (type == eFTLFieldTypeMessage)
                type = eFTLFieldTypeMessageArray;
        } else {
            type = eFTLFieldTypeUnknown;
        }
    } else {
        type = [self getObjectType:obj];
    }
    
    return type;
}

- (NSArray *)getFieldNames
{
    return [_dict allKeys];
}

- (void)setField:(NSString *)name asString:(NSString *)value
{
    if (value == nil)
        [_dict removeObjectForKey:name];
    else
        [_dict setObject:value forKey:name];
}

- (void)setField:(NSString *)name asStringArray:(NSArray *)value
{
    if (value == nil) {
        [_dict removeObjectForKey:name];
    } else {
        for (id obj in value) {
            if ([obj isKindOfClass:[NSString class]] == NO)
                [NSException raise:NSInvalidArgumentException format:@"Array is not of type %@", [NSString class]];
        }
        
        [_dict setObject:value forKey:name];
    }
}

- (void)setField:(NSString *)name asLong:(NSNumber *)value
{
    if (value == nil)
        [_dict removeObjectForKey:name];
    else
        [_dict setObject:value forKey:name];
}

- (void)setField:(NSString *)name asLongArray:(NSArray *)value
{
    if (value == nil) {
        [_dict removeObjectForKey:name];
    } else {
        for (id obj in value) {
            if ([obj isKindOfClass:[NSNumber class]] == NO)
                [NSException raise:NSInvalidArgumentException format:@"Array is not of type %@", [NSNumber class]];
        }
        
        [_dict setObject:value forKey:name];
    }
}

- (void)setField:(NSString *)name asDouble:(NSNumber *)value
{
    if (value == nil)
        [_dict removeObjectForKey:name];
    else
        [_dict setObject:[eFTLMessage dictionaryFromDouble:value] forKey:name];
}

- (void)setField:(NSString *)name asDoubleArray:(NSArray *)value
{
    if (value == nil) {
        [_dict removeObjectForKey:name];
    } else {
        NSMutableArray *array = [NSMutableArray arrayWithCapacity:[value count]];
        
        for (id obj in value) {
            if ([obj isKindOfClass:[NSNumber class]] == NO)
                [NSException raise:NSInvalidArgumentException format:@"Array is not of type %@", [NSNumber class]];
            [array addObject:[eFTLMessage dictionaryFromDouble:obj]];
        }
        
        [_dict setObject:array forKey:name];
    }
}

- (void)setField:(NSString *)name asDate:(NSDate *)value
{
    if (value == nil)
        [_dict removeObjectForKey:name];
    else
        [_dict setObject:[eFTLMessage dictionaryFromDate:value] forKey:name];
}

- (void)setField:(NSString *)name asDateArray:(NSArray *)value
{
    if (value == nil) {
        [_dict removeObjectForKey:name];
    } else {
        NSMutableArray *array = [NSMutableArray arrayWithCapacity:[value count]];
        
        for (id obj in value) {
            if ([obj isKindOfClass:[NSDate class]] == NO)
                [NSException raise:NSInvalidArgumentException format:@"Array is not of type %@", [NSDate class]];
            [array addObject:[eFTLMessage dictionaryFromDate:obj]];
        }
        
        [_dict setObject:array forKey:name];
    }
}

- (void)setField:(NSString *)name asMessage:(eFTLMessage *)value
{
    if (value == nil)
        [_dict removeObjectForKey:name];
    else
        [_dict setObject:[value->_dict copy] forKey:name];
}

- (void)setField:(NSString *)name asMessageArray:(NSArray *)value
{
    if (value == nil) {
        [_dict removeObjectForKey:name];
    } else {
        NSMutableArray *array = [NSMutableArray arrayWithCapacity:[value count]];
        
        for (id obj in value) {
            if ([obj isKindOfClass:[eFTLMessage class]] == NO)
                [NSException raise:NSInvalidArgumentException format:@"Array is not of type %@", [eFTLMessage class]];
            [array addObject:[((eFTLMessage *) obj)->_dict copy]];
        }
        
        [_dict setObject:array forKey:name];
    }
}

- (void)setField:(NSString *)name asOpaque:(NSData *)value
{
    if (value == nil)
        [_dict removeObjectForKey:name];
    else
        [_dict setObject:[eFTLMessage dictionaryFromData:value] forKey:name];
}


- (NSString *)getStringField:(NSString *)name
{
    NSString *value = nil;
    
    id obj = [_dict objectForKey:name];

    if ([self getObjectType:obj] == eFTLFieldTypeString)
        value = obj;

    return value;
}

- (NSArray *)getStringArrayField:(NSString *)name
{
    NSArray *value = nil;
    
    id obj = [_dict objectForKey:name];
    
    if ([obj isKindOfClass:[NSArray class]]) {
        if ([self getObjectType:[obj objectAtIndex:0]] == eFTLFieldTypeString)
            value = obj;
    }
    
    return value;
}

- (NSNumber *)getLongField:(NSString *)name
{
    NSNumber *value = nil;
    
    id obj = [_dict objectForKey:name];

    if ([self getObjectType:obj] == eFTLFieldTypeLong)
        value = obj;
    
    return value;
}

- (NSArray *)getLongArrayField:(NSString *)name
{
    NSArray *value = nil;
    
    id obj = [_dict objectForKey:name];
    
    if ([obj isKindOfClass:[NSArray class]]) {
        if ([self getObjectType:[obj objectAtIndex:0]] == eFTLFieldTypeLong)
            value = obj;
    }
    
    return value;
}

- (NSNumber *)getDoubleField:(NSString *)name
{
    NSNumber *value = nil;
    
    id obj = [_dict objectForKey:name];
    
    if ([self getObjectType:obj] == eFTLFieldTypeDouble)
        value = [eFTLMessage doubleFromDictionary:obj];
    
    return value;
}

- (NSArray *)getDoubleArrayField:(NSString *)name
{
    NSArray *value = nil;
    
    id obj = [_dict objectForKey:name];
    
    if ([obj isKindOfClass:[NSArray class]]) {
        if ([self getObjectType:[obj objectAtIndex:0]] == eFTLFieldTypeDouble) {
            NSMutableArray *array = [NSMutableArray arrayWithCapacity:[value count]];
            
            for (NSDictionary *dict in obj)
                [array addObject:[eFTLMessage doubleFromDictionary:dict]];
            
            value = [array copy];
        }
    }
    
    return value;
}

- (NSDate *)getDateField:(NSString *)name
{
    NSDate *value = nil;
    
    id obj = [_dict objectForKey:name];
    
    if ([self getObjectType:obj] == eFTLFieldTypeDate)
        value = [eFTLMessage dateFromDictionary:obj];
    
    return value;
}

- (NSArray *)getDateArrayField:(NSString *)name
{
    NSArray *value = nil;
    
    id obj = [_dict objectForKey:name];
    
    if ([obj isKindOfClass:[NSArray class]]) {
        if ([self getObjectType:[obj objectAtIndex:0]] == eFTLFieldTypeDate) {
            NSMutableArray *array = [NSMutableArray arrayWithCapacity:[value count]];
            
            for (NSDictionary *dict in obj)
                [array addObject:[eFTLMessage dateFromDictionary:dict]];
            
            value = [array copy];
        }
    }
    
    return value;
}

- (eFTLMessage *)getMessageField:(NSString *)name
{
    eFTLMessage *value = nil;
    
    id obj = [_dict objectForKey:name];
    
    if ([self getObjectType:obj] == eFTLFieldTypeMessage)
        value = [eFTLMessage messageWithDictionary:obj];
    
    return value;
}

- (NSArray *)getMessageArrayField:(NSString *)name
{
    NSArray *value = nil;
    
    id obj = [_dict objectForKey:name];
    
    if ([obj isKindOfClass:[NSArray class]]) {
        if ([self getObjectType:[obj objectAtIndex:0]] == eFTLFieldTypeMessage) {
            NSMutableArray *array = [NSMutableArray arrayWithCapacity:[value count]];
            
            for (NSMutableDictionary *dict in obj)
                [array addObject:[eFTLMessage messageWithDictionary:dict]];
            
            value = [array copy];
        }
    }
    
    return value;
}

- (NSData *)getOpaqueField:(NSString *)name
{
    NSData *value = nil;
    
    id obj = [_dict objectForKey:name];
    
    if ([self getObjectType:obj] == eFTLFieldTypeOpaque)
        value = [eFTLMessage dataFromDictionary:obj];
    
    return value;
}


- (NSDictionary *)dictionary
{
    return _dict;
}

- (NSString *)description
{
    int arrayCount;
    NSMutableString *desc = [NSMutableString stringWithCapacity:1024];
    [desc appendString:@"{"];
    for (NSString *name in _dict) {
        if (desc.length > 1) {
            [desc appendString:@", "];
        }
        eFTLFieldType type = [self getFieldType:name];
        switch (type) {
            case eFTLFieldTypeString:
                [desc appendFormat:@"%@:string=\"%@\"", name, [self getStringField:name]];
                break;
                
            case eFTLFieldTypeLong:
                [desc appendFormat:@"%@:long=%@", name, [self getLongField:name]];
                break;
                
            case eFTLFieldTypeDouble:
                [desc appendFormat:@"%@:double=%@", name, [self getDoubleField:name]];
                break;
                
            case eFTLFieldTypeDate:
                [desc appendFormat:@"%@:date=%@", name, [self getDateField:name]];
                break;
                
            case eFTLFieldTypeMessage:
                [desc appendFormat:@"%@:message=%@", name, [self getMessageField:name]];
                break;
                
            case eFTLFieldTypeOpaque:
                [desc appendFormat:@"%@:opaque=%@", name, [self getOpaqueField:name]];
                
            case eFTLFieldTypeStringArray:
                arrayCount = 0;
                [desc appendFormat:@"%@:string_array=[", name];
                for (NSString *value in [self getStringArrayField:name]) {
                    if (arrayCount++) {
                        [desc appendFormat:@", \"%@\"", value];
                    } else {
                        [desc appendFormat:@"\"%@\"", value];
                    }
                }
                [desc appendString:@"]"];
                break;
                
            case eFTLFieldTypeLongArray:
                arrayCount = 0;
                [desc appendFormat:@"%@:long_array=[", name];
                 for (NSNumber *value in [self getLongArrayField:name]) {
                     if (arrayCount++) {
                         [desc appendFormat:@", %@", value];
                     } else {
                         [desc appendFormat:@"%@", value];
                     }
                 }
                [desc appendString:@"]"];
                break;
                
            case eFTLFieldTypeDoubleArray:
                arrayCount = 0;
                [desc appendFormat:@"%@:double_array=[", name];
                for (NSNumber *value in [self getDoubleArrayField:name]) {
                    if (arrayCount++) {
                        [desc appendFormat:@", %@", value];
                    } else {
                        [desc appendFormat:@"%@", value];
                    }
                }
                [desc appendString:@"]"];
                break;
                
            case eFTLFieldTypeDateArray:
                arrayCount = 0;
                [desc appendFormat:@"%@:date_array=[", name];
                for (NSDate *value in [self getDateArrayField:name]) {
                    if (arrayCount++) {
                        [desc appendFormat:@", %@", value];
                    } else {
                        [desc appendFormat:@"%@", value];
                    }
                }
                [desc appendString:@"]"];
                break;
                
            case eFTLFieldTypeMessageArray:
                arrayCount = 0;
                [desc appendFormat:@"%@:message_array=[", name];
                for (eFTLMessage *value in [self getMessageArrayField:name]) {
                    if (arrayCount++) {
                        [desc appendFormat:@", %@", value];
                    } else {
                        [desc appendFormat:@"%@", value];
                    }
                }
                [desc appendString:@"]"];
                break;
                
            default:
                [desc appendFormat:@"%@:unknown=%@", name, [_dict objectForKey:name]];
                break;
        }
    }
    [desc appendString:@"}"];
    return desc;
}

+ (NSDictionary *)dictionaryFromDouble:(NSNumber *)number
{
    NSDictionary *dict = nil;
    
    if (number != nil) {
        NSMutableDictionary *mutableDict = [NSMutableDictionary dictionaryWithCapacity:1];
        if (isnan([number doubleValue]) || isinf([number doubleValue]))
            [mutableDict setValue:[number stringValue] forKey:FTLFieldNameDouble];
        else
            [mutableDict setValue:number forKey:FTLFieldNameDouble];
        dict = [mutableDict copy];
    }
    
    return dict;
}

+ (NSNumber *)doubleFromDictionary:(NSDictionary *)dict
{
    NSNumber *number = nil;
    
    if (dict != nil) {
        if ([[dict objectForKey:FTLFieldNameDouble] isKindOfClass:[NSNumber class]]) {
            number = [dict objectForKey:FTLFieldNameDouble];
        } else if ([[dict objectForKey:FTLFieldNameDouble] isKindOfClass:[NSString class]]) {
            NSString *str = [dict objectForKey:FTLFieldNameDouble];
            number = [NSNumber numberWithDouble:strtod(str.UTF8String, NULL)];
        }
    }
    
    return number;
}

+ (NSDictionary *)dictionaryFromDate:(NSDate *)date
{
    NSDictionary *dict = nil;
    
    if (date != nil) {
        NSTimeInterval seconds = [date timeIntervalSince1970];
        NSNumber *milliseconds = [NSNumber numberWithLongLong:(seconds*1000)];
        NSMutableDictionary *mutableDict = [NSMutableDictionary dictionaryWithCapacity:1];
        [mutableDict setValue:milliseconds forKey:FTLFieldNameMillisecond];
        dict = [mutableDict copy];
    }
    
    return dict;
}

+ (NSDate *)dateFromDictionary:(NSDictionary *)dict
{
    NSDate *date = nil;
    
    if (dict != nil) {
        NSNumber *milliseconds = [dict objectForKey:FTLFieldNameMillisecond];
        NSTimeInterval timeInterval = [milliseconds doubleValue] / 1000.0;
        date = [NSDate dateWithTimeIntervalSince1970:timeInterval];
    }
    
    return date;
}

+ (NSDictionary *)dictionaryFromData:(NSData *)data
{
    NSDictionary *dict = nil;
    
    if (data != nil) {
        NSString *str = [data base64EncodedStringWithOptions:0];
        NSMutableDictionary *mutableDict = [NSMutableDictionary dictionaryWithCapacity:1];
        [mutableDict setValue:str forKey:FTLFieldNameOpaque];
        dict = [mutableDict copy];
    }
    
    return dict;
}

+ (NSData *)dataFromDictionary:(NSDictionary *)dict
{
    NSData *data = nil;
    
    if (dict != nil) {
        NSString *str = [dict objectForKey:FTLFieldNameOpaque];
        data = [[NSData alloc] initWithBase64EncodedString:str options:0];
    }
    
    return data;
}

@end
