/*
 * Copyright (c) 2001-$Date$ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id$
 *
 */
package com.tibco.eftl;

import java.util.Date;

/**
 * Message objects contain typed fields that map names to values. 
 */
public interface Message
{
    /**
     * Message field name identifying the EMS destination of a message. 
     * <p>
     * The destination message field is only required when communicating with
     * EMS.
     * <p>
     * To publish a message on a specific destination include this 
     * message field using {@link Message#setString}.
     * <pre>
     *     message.setString(Message.FIELD_NAME_DESTINATION, "MyDest");
     * </pre>
     * <p>
     * To subscribe to messages published on a specific destination,
     * use a matcher that includes this message field name. 
     * <pre>
     *     String matcher = String.Format("{\"%s\":\"%s\"}",
     *         Message.FIELD_NAME_DESTINATION, "MyDest");
     *
     *     connection.Subscribe(matcher, "MyDurable", null, new MySubscriptionListener());
     * </pre>
     * <p>
     * To distinguish between topics and queues the destination name
     * can be prefixed with either "TOPIC:" or "QUEUE:", for example
     * "TOPIC:MyDest" or "QUEUE:MyDest". A destination with no prefix is
     * a topic.
     */
    public static final String FIELD_NAME_DESTINATION = "_dest";

    /**
     * Message field types.
     * <p>
     * Enumerates the legal types for eFTL message fields.
     */
    public enum FieldType 
    {
        /** Unknown field. */
        UNKNOWN,
        
        /** String field. */
        STRING,
        
        /** Long field. */
        LONG,
        
        /** Double field. */
        DOUBLE,
        
        /** Date field. */
        DATE,
        
        /** Sub-message field. */
        MESSAGE,
        
        /** Opaque field. */
        OPAQUE,
        
        /** String array field. */
        STRING_ARRAY,
        
        /** Long array field. */
        LONG_ARRAY,
        
        /** Double array field. */
        DOUBLE_ARRAY,
        
        /** Date array field. */
        DATE_ARRAY,
        
        /** Message array field. */
        MESSAGE_ARRAY
    };
    
    /**
     * Get the message's unique store identifier assigned
     * by the persistence service.
     *
     * @return The message identifier.
     */
    public long getStoreMessageId();

    /**
     * Get the message's delivery count assigned
     * by the persistence service.
     *
     * @return The message delivery count.
     */
    public long getDeliveryCount();

    /**
     * Determine whether a field is present in the message.
     * 
     * @param fieldName The method checks for this field.
     *
     * @return {@code true} if the field is present; {@code false} otherwise.
     *
     * @throws IllegalArgumentException The fieldName argument is null or empty.
     */
    public boolean isFieldSet(String fieldName);
    
    /**
     * Get the names of all fields present in a message.
     * 
     * @return An array of field names.
     */
    public String[] getFieldNames();
    
    /**
     * Get the type of a message field.
     * 
     * @param fieldName The name of the field.
     *
     * @return The type of the field, if present in the message;
     *         {@code null} otherwise.
     *
     * @throws IllegalArgumentException If the field name is null or empty.
     */
    public FieldType getFieldType(String fieldName);

    /**
     * Set a string field in a message.
     * 
     * @param fieldName The call sets this field.
     * @param value The call sets this value.
     *              To remove the field, supply {@code null}.
     *
     * @throws IllegalArgumentException The field name is null or empty.
     */
    public void setString(String fieldName, String value);
    
    /**
     * Set a long field in a message.
     * 
     * @param fieldName The call sets this field.
     * @param value The call sets this value.
     *              To remove the field, supply {@code null}.
     *
     * @throws IllegalArgumentException The field name is null or empty.
     */
    public void setLong(String fieldName, Long value);
    
    /**
     * Set a double field in a message.
     * 
     * @param fieldName The call sets this field.
     * @param value The call sets this value.
     *              To remove the field, supply {@code null}.
     *
     * @throws IllegalArgumentException The field name is null or empty.
     */
    public void setDouble(String fieldName, Double value);
    
    /**
     * Set a date field in a message.
     * 
     * @param fieldName The call sets this field.
     * @param value The call sets this value.
     *              To remove the field, supply {@code null}.
     *
     * @throws IllegalArgumentException The field name is null or empty.
     */
    public void setDate(String fieldName, Date value);

    /**
     * Set a sub-message field in a message.
     * <p>
     * This method makes an independent copy of the sub-message, and
     * adds the copy to the message.  
     * After this method returns you can safely modify the
     * original sub-message.
     * 
     * @param fieldName The call sets this field.
     * @param value The call sets this value.
     *              To remove the field, supply {@code null}.
     *
     * @throws IllegalArgumentException The field name is null or empty.
     */
    public void setMessage(String fieldName, Message value);

    /**
     * Set an opaque field in a message.
     * 
     * @param fieldName The call sets this field.
     * @param value The call sets this value.
     *              To remove the field, supply {@code null}.
     */
    public void setOpaque(String fieldName, byte[] value);
    
    /**
     * Set a string array field in a message.
     * <p>
     * This method makes an independent copy of the array and strings,
     * and adds the copy to the message.  After this method returns
     * you can safely modify the original array and its contents.
     * 
     * @param fieldName The call sets this field.
     * @param value The call sets this value.
     *              (Null is not a legal value within this array.)
     *              To remove the field, supply {@code null}.
     *
     * @throws IllegalArgumentException The field name is null or empty.
     */
    public void setArray(String fieldName, String[] value);

    /**
     * Set a long array field in a message.
     * <p>
     * This method makes an independent copy of the array, and
     * adds the copy to the message.  
     * After this method returns you can safely modify the
     * original array.
     * 
     * @param fieldName The call sets this field.
     * @param value The call sets this value.
     *              To remove the field, supply {@code null}.
     *
     * @throws IllegalArgumentException The field name is null or empty.
     */
    public void setArray(String fieldName, Long[] value);

    /**
     * Set a double array field in a message.
     * <p>
     * This method makes an independent copy of the array, and
     * adds the copy to the message.  
     * After this method returns you can safely modify the
     * original array.
     * 
     * @param fieldName The call sets this field.
     * @param value The call sets this value.
     *              To remove the field, supply {@code null}.
     *
     * @throws IllegalArgumentException The field name is null or empty.
     */
    public void setArray(String fieldName, Double[] value);

    /**
     * Set a date array field in a message.
     * <p>
     * This method makes an independent copy of the array, and
     * adds the copy to the message.  
     * After this method returns you can safely modify the
     * original array and its contents.
     * 
     * @param fieldName The call sets this field.
     * @param value The call sets this value.
     *              To remove the field, supply {@code null}.
     *
     * @throws IllegalArgumentException The field name is null or empty.
     */
    public void setArray(String fieldName, Date[] value);
    
    /**
     * Set a message array field in a message.
     * <p>
     * This method makes an independent copy of the array and
     * messages, and adds the copy to the message.  After this method
     * returns you can safely modify the original array and its
     * contents.
     * 
     * @param fieldName The call sets this field.
     * @param value The call sets this value.
     *              To remove the field, supply {@code null}.
     *
     * @throws IllegalArgumentException The field name is null or empty.
     */
    public void setArray(String fieldName, Message[] value);

    /**
     * Get the value of a string field from a message.
     * 
     * @param fieldName Get this field.
     *
     * @return The value of the field, if the field is present and has
     *         type {@link FieldType#STRING};
     *         {@code null} otherwise.
     *
     * @throws IllegalArgumentException The field name is null or empty.
     */
    public String getString(String fieldName);

    /**
     * Get the value of a long field from a message.
     * 
     * @param fieldName Get this field.
     *
     * @return The value of the field, if the field is present and has
     *         type {@link FieldType#LONG};
     *         {@code null} otherwise.
     *
     * @throws IllegalArgumentException The field name is null or empty.
     */
    public Long getLong(String fieldName);

    /**
     * Get the value of a double field from a message.
     * 
     * @param fieldName Get this field.
     *
     * @return The value of the field, if the field is present and has
     *         type {@link FieldType#DOUBLE};
     *         {@code null} otherwise.
     *
     * @throws IllegalArgumentException The field name is null or empty.
     */
    public Double getDouble(String fieldName);

    /**
     * Get the value of a date field from a message.
     * 
     * @param fieldName Get this field.
     *
     * @return The value of the field, if the field is present and has
     *         type {@link FieldType#DATE};
     *         {@code null} otherwise.
     *
     * @throws IllegalArgumentException The field name is null or empty.
     */
    public Date getDate(String fieldName);

    /**
     * Get the value of a sub-message field from a message.
     *
     * @param fieldName Get this field.
     *
     * @return The value of the field, if the field is present and has
     *         type {@link FieldType#MESSAGE};
     *         {@code null} otherwise.
     *
     * @throws IllegalArgumentException The field name is null or empty.
     */
    public Message getMessage(String fieldName);
    
    /**
     * Get the value of an opaque field from a message.
     * 
     * @param fieldName Get this field.
     * 
     * @return The value of the field, if the field is present and has
     *         type {@link FieldType#OPAQUE};
     *         {@code null} otherwise.
     *         
     * @throws IllegalArgumentException The field name is null or empty.
     */
    public byte[] getOpaque(String fieldName);
    
    /**
     * Get the value of a string array field from a message.
     *
     * @param fieldName Get this field.
     *
     * @return The value of the field, if the field is present and has
     *         type {@link FieldType#STRING_ARRAY};
     *         {@code null} otherwise.
     *
     * @throws IllegalArgumentException The field name is null or empty.
     */
    public String[] getStringArray(String fieldName);
    
    /**
     * Get the value of a long array field from a message.
     *
     * @param fieldName Get this field.
     *
     * @return The value of the field, if the field is present and has
     *         type {@link FieldType#LONG_ARRAY};
     *         {@code null} otherwise.
     *
     * @throws IllegalArgumentException The field name is null or empty.
     */
    public Long[] getLongArray(String fieldName);
    
    /**
     * Get the value of a double array field from a message.
     *
     * @param fieldName Get this field.
     *
     * @return The value of the field, if the field is present and has
     *         type {@link FieldType#DOUBLE_ARRAY};
     *         {@code null} otherwise.
     *
     * @throws IllegalArgumentException The field name is null or empty.
     */
    public Double[] getDoubleArray(String fieldName);
    
    /**
     * Get the value of a date array field from a message.
     *
     * @param fieldName Get this field.
     *
     * @return The value of the field, if the field is present and has
     *         type {@link FieldType#DATE_ARRAY};
     *         {@code null} otherwise.
     *
     * @throws IllegalArgumentException The field name is null or empty.
     */
    public Date[] getDateArray(String fieldName);

    /**
     * Get the value of a message array field from a message.
     *
     * @param fieldName Get this field.
     *
     * @return The value of the field, if the field is present and has
     *         type {@link FieldType#MESSAGE_ARRAY};
     *         {@code null} otherwise.
     *
     * @throws IllegalArgumentException The field name is null or empty.
     */
    public Message[] getMessageArray(String fieldName);
}
