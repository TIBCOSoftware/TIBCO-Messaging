//
//  Copyright (c) 2013-$Date: 2017-01-30 13:23:42 -0600 (Mon, 30 Jan 2017) $ TIBCO Software Inc.
//  Licensed under a BSD-style license. Refer to [LICENSE]
//  For more information, please contact:
//  TIBCO Software Inc., Palo Alto, California USA
//
//  $Id: eFTLMessage.h 91118 2017-01-30 19:23:42Z bpeterse $
//

#import <Foundation/Foundation.h>

/**
 * \file eFTLMessage.h
 *
 * \brief Messages and fields
 */

/**
 * \memberof eFTLMessage
 * \brief Message field name identifying the destination on which
 * messages are published.
 *
 * To publish a message on a specific destination include this
 * message field using eFTLMessage::setField:asString:.
 *
 * To subscribe to messages published on a specific destination
 * use a matcher that includes this message field name.
 */
extern NSString *const eFTLFieldNameDestination;

/**
 * \memberof eFTLMessage
 * \brief Message field types.
 *
 * Enumerates the legal types for %eFTL message fields.
 */
typedef enum {
    /** Unknown field type. */
    eFTLFieldTypeUnknown,
    /** String field type. */
    eFTLFieldTypeString,
    /** Long field type. */
    eFTLFieldTypeLong,
    /** Double field type. */
    eFTLFieldTypeDouble,
    /** Date field type. */
    eFTLFieldTypeDate,
    /** Message field type. */
    eFTLFieldTypeMessage,
    /** Opaque field type. */
    eFTLFieldTypeOpaque,
    /** String array field type. */
    eFTLFieldTypeStringArray,
    /** Long array field type. */
    eFTLFieldTypeLongArray,
    /** Double array field type. */
    eFTLFieldTypeDoubleArray,
    /** Date array field type. */
    eFTLFieldTypeDateArray,
    /** Message array field type. */
    eFTLFieldTypeMessageArray
} eFTLFieldType;

/**
 * \brief Message objects contain typed fields that map names to values.
 */
@interface eFTLMessage : NSObject

/**
 * \brief Create an eFTLMessage.
 *
 * @return A new eFTLMessage.
 */
+ (eFTLMessage *)message;

/**
 * \brief Get the names of all fields present in a message.
 *
 * @return An array of field names.
 */
- (NSArray *)getFieldNames;

/**
 * \brief Determine whether a field is present in the message.
 *
 * @param name The method checks for this field.
 * @return \c YES if the field is present; \c NO otherwise.
 */
- (BOOL)isFieldSet:(NSString *)name;

/**
 * \brief Get the type of a message field.
 *
 * @param name The name of the field.
 * @return The type of the field, if present; \c nil otherwise.
 */
- (eFTLFieldType)getFieldType:(NSString *)name;

/**
 * \brief Set a string field in the message.
 *
 * @param name The call sets this field.
 * @param string The call sets this value.
 *               To remove the field, supply \c nil.
 */
- (void)setField:(NSString *)name asString:(NSString *)string;

/**
 * \brief Set a string array field in a message.
 *
 * This method makes an independent copy of the array and strings, and
 * adds the copy to the message.  After this method returns you can
 * safely modify the original array and its contents.
 *
 * @param name The call sets this field.
 * @param stringArray The call sets this value.
 *              (Null is not a legal value within this array.)
 *               To remove the field, supply \c nil.
 */
- (void)setField:(NSString *)name asStringArray:(NSArray *)stringArray;

/**
 * \brief Set a long field in a message.
 *
 * @param name The call sets this field.
 * @param number The call sets this value.
 *               To remove the field, supply \c nil.
 */
- (void)setField:(NSString *)name asLong:(NSNumber *)number;

/**
 * \brief Set a long array field in a message.
 *
 * This method makes an independent copy of the array, and
 * adds the copy to the message.  
 * After this method returns you can safely modify the
 * original array.
 *
 * @param name The call sets this field.
 * @param numberArray The call sets this value.
 *               To remove the field, supply \c nil.
 */
- (void)setField:(NSString *)name asLongArray:(NSArray *)numberArray;

/**
 * \brief Set a double field in a message.
 *
 * @param name The call sets this field.
 * @param number The call sets this value.
 *               To remove the field, supply \c nil.
 */
- (void)setField:(NSString *)name asDouble:(NSNumber *)number;

/**
 * \brief Set a double array field in a message.
 *
 * This method makes an independent copy of the array, and
 * adds the copy to the message.  
 * After this method returns you can safely modify the
 * original array.
 *
 * @param name The call sets this field.
 * @param numberArray The call sets this value.
 *               To remove the field, supply \c nil.
 */
- (void)setField:(NSString *)name asDoubleArray:(NSArray *)numberArray;

/**
 * \brief Set a date field in a message.
 *
 * @param name The call sets this field.
 * @param date The call sets this value.
 *               To remove the field, supply \c nil.
 */
- (void)setField:(NSString *)name asDate:(NSDate *)date;

/**
 * \brief Set a date array field in a message.
 *
 * This method makes an independent copy of the array, and
 * adds the copy to the message.  
 * After this method returns you can safely modify the
 * original array.
 *
 * @param name The call sets this field.
 * @param dateArray The call sets this value.
 *               To remove the field, supply \c nil.
 */
- (void)setField:(NSString *)name asDateArray:(NSArray *)dateArray;

/**
 * \brief Set a sub-message field in a message.
 *
 * This method makes an independent copy of the sub-message, and
 * adds the copy to the message.  
 * After this method returns you can safely modify the
 * original sub-message and its contents.
 *
 * @param name The call sets this field.
 * @param message The call sets this value.
 *               To remove the field, supply \c nil.
 */
- (void)setField:(NSString *)name asMessage:(eFTLMessage *)message;

/**
 * \brief Set a message array field in a message.
 *
 * This method makes an independent copy of the array and messages,
 * and adds the copy to the message.  After this method returns you
 * can safely modify the original array and its contents.
 *
 * @param name The call sets this field.
 * @param messageArray The call sets this value.
 *               To remove the field, supply \c nil.
 */
- (void)setField:(NSString *)name asMessageArray:(NSArray *)messageArray;

/**
 * \brief Set an opaque field in a message.
 *
 * @param name The call sets this field.
 * @param data The call sets this value.
 *               To remove the field, supply \c nil.
 */
- (void)setField:(NSString *)name asOpaque:(NSData *)data;

/**
 * \brief Get the value of a string field from a message.
 *
 * @param name Get this field.
 * @return The value of the field, if the field is present and has
 *         type #eFTLFieldTypeString;
 *         \c nil otherwise.
 */
- (NSString *)getStringField:(NSString *)name;

/**
 * \brief Get the value of a string array field from a message.
 *
 * @param name Get this field.
 * @return The value of the field, if the field is present and has
 *         type #eFTLFieldTypeStringArray;
 *         \c nil otherwise.
 */
- (NSArray *)getStringArrayField:(NSString *)name;

/**
 * \brief Get the value of a long field from a message.
 *
 * @param name Get this field.
 * @return The value of the field, if the field is present and has
 *         type #eFTLFieldTypeLong;
 *         \c nil otherwise.
 */
- (NSNumber *)getLongField:(NSString *)name;

/**
 * \brief Get the value of a long array field from a message.
 *
 * @param name Get this field.
 * @return The value of the field, if the field is present and has
 *         type #eFTLFieldTypeLongArray;
 *         \c nil otherwise.
 */
- (NSArray *)getLongArrayField:(NSString *)name;

/**
 * \brief Get the value of a double field from a message.
 *
 * @param name Get this field.
 * @return The value of the field, if the field is present and has
 *         type #eFTLFieldTypeDouble;
 *         \c nil otherwise.
 */
- (NSNumber *)getDoubleField:(NSString *)name;

/**
 * \brief Get the value of a double array field from a message.
 *
 * @param name Get this field.
 * @return The value of the field, if the field is present and has
 *         type #eFTLFieldTypeDoubleArray;
 *         \c nil otherwise.
 */
- (NSArray *)getDoubleArrayField:(NSString *)name;

/**
 * \brief Get the value of a date field from a message.
 *
 * @param name Get this field.
 * @return The value of the field, if the field is present and has
 *         type #eFTLFieldTypeDate;
 *         \c nil otherwise.
 */
- (NSDate *)getDateField:(NSString *)name;

/**
 * \brief Get the value of a date array field from a message.
 *
 * @param name Get this field.
 * @return The value of the field, if the field is present and has
 *         type #eFTLFieldTypeDateArray;
 *         \c nil otherwise.
 */
- (NSArray *)getDateArrayField:(NSString *)name;

/**
 * \brief Get the value of a sub-message field from a message.
 *
 * @param name Get this field.
 * @return The value of the field, if the field is present and has
 *         type #eFTLFieldTypeMessage;
 *         \c nil otherwise.
 */
- (eFTLMessage *)getMessageField:(NSString *)name;

/**
 * \brief Get the value of a message array field from a message.
 *
 * @param name Get this field.
 * @return The value of the field, if the field is present and has
 *         type #eFTLFieldTypeMessageArray;
 *         \c nil otherwise.
 */
- (NSArray *)getMessageArrayField:(NSString *)name;

/**
 * \brief Get the value of an opaque field from a message.
 *
 * @param name Get this field.
 * @return The value of the field, if the field is present and has
 *         type #eFTLFieldTypeOpaque;
 *         \c nil otherwise.
 */
- (NSData *)getOpaqueField:(NSString *)name;

@end
