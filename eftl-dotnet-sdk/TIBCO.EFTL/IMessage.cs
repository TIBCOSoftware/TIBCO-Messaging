/*
 * Copyright (c) 2001-$Date: 2017-01-31 15:28:19 -0600 (Tue, 31 Jan 2017) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: IMessage.cs 91161 2017-01-31 21:28:19Z bmahurka $
 *
 */
using System;
using System.Collections;
using System.Linq;
using System.Text;

namespace TIBCO.EFTL
{
    /// <summary>
    /// Message objects contain typed fields that map names to values. 
    /// </summary>
    public interface IMessage
    {
        /// <summary>
        /// Determine whether a field is present in the message.
        /// </summary>
        /// 
        /// <param name="fieldName"> The method checks for this field. </param>
        ///
        /// <returns> <c>true</c> if the field is present;
        /// <c>false</c> otherwise. 
        /// </returns>
        ///
        /// <exception cref="ArgumentException"> The fieldName argument is null or empty. </exception>
        ///
        bool IsFieldSet(String fieldName);
        
        /// <summary>
        /// Get the names of all fields present in a message.
        /// </summary>
        /// 
        /// <returns> An array of field names.
        /// </returns>
        ///
        String[] GetFieldNames();
        
        /// <summary>
        /// Get the type of a message field.
        /// </summary>
        /// 
        /// <param name="fieldName"> The name of the field. </param>
        ///
        /// <returns> The type of the field, if present;
        /// <c>null</c> otherwise.
        /// </returns>
        ///
        /// <exception cref="ArgumentException"> If the field name is null or empty.  </exception>
        ///
        FieldType GetFieldType(String fieldName);
        
        /// <summary>
        /// Set a string field in a message.
        /// </summary>
        /// 
        /// <param name="fieldName"> The call sets this field. </param>
        /// <param name="value"> The call sets this value. </param>
        ///              To remove the field, supply <c>null</c>.
        ///
        /// <exception cref="ArgumentException"> The field name is null or empty.  </exception>
        ///
        void SetString(String fieldName, String value);
        
        ///  <summary>
        /// Set a long field in a message.
        /// </summary>
        /// 
        /// <param name="fieldName"> The call sets this field. </param>
        /// <param name="value"> The call sets this value. </param>
        ///              To remove the field, supply <c>null</c>.
        ///
        /// <exception cref="ArgumentException"> The field name is null or empty.  </exception>
        ///
        void SetLong(String fieldName, long value);
        
        ///  <summary>
        /// Set a double field in a message.
        /// </summary>
        /// 
        /// <param name="fieldName"> The call sets this field. </param>
        /// <param name="value"> The call sets this value.
        ///              To remove the field, supply <c>null</c>. </param>
        ///
        /// <exception cref="ArgumentException"> The field name is null or empty.  </exception>
        ///
        void SetDouble(String fieldName, Double value);
        
        /// <summary>
        /// Set a date field in a message.
        /// </summary>
        /// 
        /// <param name="fieldName"> The call sets this field. </param>
        /// <param name="value"> The call sets this value.
        ///              To remove the field, supply <c>null</c>. </param>
        ///
        /// <exception cref="ArgumentException"> The field name is null or empty.   </exception>
        ///
        void SetDateTime(String fieldName, DateTime value);
        
        /// <summary>
        /// Set a sub-message field in a message.
        /// </summary>
        /// 
        /// <p>
        /// This method makes an independent copy of the sub-message, and
        /// adds the copy to the message.  
        /// After this method returns you can safely modify the
        /// original sub-message.
        /// </p>
        /// 
        /// <param name="fieldName"> The call sets this field. </param>
        /// <param name="value"> The call sets this value.
        ///              To remove the field, supply <c>null</c>. </param>
        ///
        /// <exception cref="ArgumentException"> The field name is null or empty.   </exception>
        ///
        void SetMessage(String fieldName, IMessage value);

        /// <summary>
        /// Set an opaque field in a message.
        /// </summary> 
        ///  
        /// <param name="fieldName"> The call sets this field.  </param>
        /// <param name="value"> The call sets this value.
        ///              To remove the field, supply <c>null</c>
        ///              </param>
        /// <exception cref="ArgumentException"> The field name is null or empty.   </exception>
        ///
        void SetOpaque(String fieldName, byte[] value);
        
        /// <summary>
        /// Set a string array field in a message.
        /// </summary>
        /// 
        /// <p>
        /// This method makes an independent copy of the array and strings,
        /// and adds the copy to the message.  After this method returns
        /// you can safely modify the original array and its contents.
        /// </p>
        /// 
        /// <param name="fieldName"> The call sets this field. </param>
        /// <param name="value"> The call sets this value.
        ///              (Null is not a legal value within this array.)
        ///              To remove the field, supply <c>null</c>. </param>
        ///
        /// <exception cref="ArgumentException"> The field name is null or empty.   </exception>
        ///
        void SetArray(String fieldName, String[] value);
        
        ///
        /// Set a long array field in a message.
        /// <p>
        /// This method makes an independent copy of the array, and
        /// adds the copy to the message.  
        /// After this method returns you can safely modify the
        /// original array.
        /// </p>
        /// 
        /// <param name="fieldName"> The call sets this field. </param>
        /// <param name="value"> The call sets this value.
        ///              To remove the field, supply <c>null</c>. </param>
        ///
        /// <exception cref="ArgumentException"> The field name is null or empty.   </exception>
        ///
        void SetArray(String fieldName, long[] value);
        
        ///<summary>
        /// Set a double array field in a message.
        /// </summary>
        /// 
        /// <p>
        /// This method makes an independent copy of the array, and
        /// adds the copy to the message.  
        /// After this method returns you can safely modify the
        /// original array.
        /// </p>
        /// 
        /// <param name="fieldName"> The call sets this field. </param>
        /// <param name="value"> The call sets this value.
        ///              To remove the field, supply <c>null</c>. </param>
        ///
        /// <exception cref="ArgumentException"> The field name is null or empty.   </exception>
        ///
        void SetArray(String fieldName, Double[] value);
        
        /// <summary>
        /// Set a date array field in a message.
        /// </summary>
        /// 
        /// <p>
        /// This method makes an independent copy of the array, and
        /// adds the copy to the message.  
        /// After this method returns you can safely modify the
        /// original array and its contents.
        /// </p>
        /// 
        /// <param name="fieldName"> The call sets this field. </param>
        /// <param name="value"> The call sets this value.
        ///              To remove the field, supply <c>null</c>. </param>
        ///
        /// <exception cref="ArgumentException"> The field name is null or empty.   </exception>
        ///
        void SetArray(String fieldName, DateTime[] value);
        
        /// <summary>
        /// Set a message array field in a message.
        /// </summary>
        /// 
        /// <p>
        /// This method makes an independent copy of the array and
        /// messages, and adds the copy to the message.  After this method
        /// returns you can safely modify the original array and its
        /// contents.
        /// </p>
        /// <param name="fieldName"> The call sets this field. </param>
        /// <param name="value"> The call sets this value.
        ///              To remove the field, supply <c>null</c>. </param>
        ///
        /// <exception cref="ArgumentException"> The field name is null or empty.   </exception>
        ///
        void SetArray(String fieldName, IMessage[] value);
        
        /// <summary>
        /// Get the value of a string field from a message.
        /// </summary>
        /// 
        /// <param name="fieldName"> Get this field. </param>
        ///
        /// <returns> The value of the field, if the field is present and has
        ///         type <see cref="FieldType.STRING"/>;
        ///         <c>null</c> otherwise. </returns>
        ///
        /// <exception cref="ArgumentException"> The field name is null or empty.   </exception>
        ///
        String GetString(String fieldName);
        
        /// <summary>
        /// Get the value of a long field from a message.
        /// </summary>
        /// 
        /// <param name="fieldName"> Get this field. </param>
        ///
        /// <returns> The value of the field, if the field is present and has
        ///         type <see cref="FieldType.LONG"/>;
        ///         <c>null</c> otherwise. </returns>
        ///
        /// <exception cref="ArgumentException"> The field name is null or empty.   </exception>
        ///
        long GetLong(String fieldName);
        
        /// <summary>
        /// Get the value of a double field from a message.
        /// </summary>
        /// 
        /// <param name="fieldName"> Get this field. </param>
        ///
        /// <returns> The value of the field, if the field is present and has
        ///         type <see cref="FieldType.DOUBLE"/>;
        ///         <c>null</c> otherwise. </returns>
        ///
        /// <exception cref="ArgumentException"> The field name is null or empty.   </exception>
        ///
        Double GetDouble(String fieldName);
        
        /// <summary>
        /// Get the value of a date field from a message.
        /// </summary>
        /// 
        /// <param name="fieldName"> Get this field. </param>
        ///
        /// <returns> The value of the field, if the field is present and has
        ///         type <see cref="FieldType.DATE"/>;
        ///         <c>null</c> otherwise.
        /// </returns>
        /// <exception cref="ArgumentException"> The field name is null or empty.
        /// </exception>
        ///
        DateTime GetDateTime(String fieldName);
        
        /// <summary>
        /// Get the value of a sub-message field from a message.
        /// </summary>
        /// 
        /// <param name="fieldName"> Get this field. </param>
        ///
        /// <returns> The value of the field, if the field is present and has
        ///         type <see cref="FieldType.MESSAGE"/>;
        ///         <c>null</c> otherwise.
        ///
        /// </returns>
        /// <exception cref="ArgumentException"> The field name is null or empty.   </exception>
        ///
        IMessage GetMessage(String fieldName);
        
        /// <summary>
        /// Get the value of an opaque field from a message.
        /// </summary>
        /// 
        /// <param name="fieldName"> Get this field. </param>
        /// 
        /// <returns> The value of the field, if the field is present
        ///         and has type <see cref="FieldType.OPAQUE"/>;
        ///         <c>null</c> otherwise.
        /// </returns>
        /// <exception cref="ArgumentException"> The field name is null or empty.   </exception>
        ///
        byte[] GetOpaque(String fieldName);

        /// <summary>
        /// Get the value of a string array field from a message.
        /// </summary>
        /// 
        /// <param name="fieldName"> Get this field. </param>
        ///
        /// <returns> The value of the field, if the field is present and has
        ///         type <see cref="FieldType.STRING_ARRAY"/>;
        ///         <c>null</c> otherwise.
        ///  </returns>
        ///  
        ///  
        /// <exception cref="ArgumentException"> The field name is null or empty.   </exception>
        ///
        String[] GetStringArray(String fieldName);
        
        /// <summary>
        /// Get the value of a long array field from a message.
        /// </summary>
        /// 
        /// <param name="fieldName"> Get this field. </param>
        ///
        /// 
        /// <returns> The value of the field, if the field is present and has
        ///         type <see cref="FieldType.LONG_ARRAY"/>;
        ///          <c>null</c> otherwise.
        /// </returns>
        /// 
        /// <exception cref="ArgumentException"> The field name is null or empty.  </exception>
        ///
        long[] GetLongArray(String fieldName);
        
        /// <summary>
        /// Get the value of a double array field from a message.
        /// </summary>
        /// 
        /// <param name="fieldName"> Get this field. </param>
        ///
        /// <returns> The value of the field, if the field is present and has
        ///         type <see cref="FieldType.DOUBLE_ARRAY"/>;
        ///         <c>null</c> otherwise.
        /// </returns>
        /// <exception cref="ArgumentException"> The field name is null or empty.   </exception>
        ///
        Double[] GetDoubleArray(String fieldName);
        
        /// <summary>
        /// Get the value of a date array field from a message.
        /// </summary>
        /// <param name="fieldName"> Get this field. </param>
        ///
        /// <returns> The value of the field, if the field is present and has
        ///         type <see cref="FieldType.DATE_ARRAY"/>;
        ///         <c>null</c> otherwise.
        /// </returns>
        /// <exception cref="ArgumentException"> The field name is null or empty.  </exception>
        ///
        DateTime[] GetDateTimeArray(String fieldName);
        
        /// <summary>
        /// Get the value of a message array field from a message.
        ///</summary>
        ///
        /// <param name="fieldName"> Get this field.</param>
        ///
        /// <returns> The value of the field, if the field is present and has
        /// </returns>
        /// <exception cref="ArgumentException"> The field name is null or empty.  </exception>
        ///
        IMessage[] GetMessageArray(String fieldName);

        /// <summary>
        ///  Clear a field in a message.
        /// </summary>
        /// <remarks>
        /// Clearing a field clears the data from a field in the message
        /// object.
        /// </remarks>
        /// <param name="name">
        /// The method clears the field with this name.
        /// </param>
        /// <exception cref="Exception">
        /// The field does not exist.
        /// </exception>
        void ClearField(String name);
    }
}
