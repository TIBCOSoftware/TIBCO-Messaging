/*
 * Copyright (c) 2001-$Date: 2017-01-31 15:28:19 -0600 (Tue, 31 Jan 2017) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: JSONMessage.cs 91161 2017-01-31 21:28:19Z bmahurka $
 *
 */

using System;
using System.Text;

namespace TIBCO.EFTL
{
    public class JSONMessage: IMessage
    {
        public static readonly String DOUBLE_FIELD      = "_d_";
        public static readonly String MILLISECOND_FIELD = "_m_";
        public static readonly String OPAQUE_FIELD      = "_o_";

        protected JsonObject data;

        public JSONMessage(JsonObject data)
        {
            this.data   = data;
        }

        public JSONMessage()
        {
            this.data = new JsonObject();
        }

        public JsonObject _rawData()
        {
            return data;
        }

        public DateTime GetDateTime(String fieldName)
        {
            object obj = null;

            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            if (!data.TryGetValue(fieldName, out obj))
                throw new System.Exception("field " + fieldName + " does not exist");

            if (!(obj is JsonObject && ((JsonObject)obj).ContainsKey(MILLISECOND_FIELD)))
                throw new System.Exception("field " + fieldName + " is not of type DATE");

            return new DateTime(1970, 1, 1).AddMilliseconds((long)((JsonObject)obj)[MILLISECOND_FIELD]);
        }

        public DateTime[] GetDateTimeArray(String fieldName)
        {
            object obj = null;

            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            if (!data.TryGetValue(fieldName, out obj))
                throw new System.Exception("field " + fieldName + " does not exist");

            if (obj is JsonArray)
            {
                JsonArray arr = (JsonArray) obj;

                if(arr.Count == 0)
                {
                    return new DateTime[0];
                }
                else if(arr[0] is JsonObject && ((JsonObject)arr[0]).ContainsKey(MILLISECOND_FIELD))
                {
                    DateTime[] retVal = new DateTime[arr.Count];

                    for(int i=0,max=arr.Count;i<max;i++)
                    {
                        retVal[i] = new DateTime(1970, 1, 1).AddMilliseconds((long)((JsonObject)arr[i])[MILLISECOND_FIELD]);
                    }

                    return retVal;
                }
                else
                {
                    throw new System.Exception("field " + fieldName + " is not of type DATE_ARRAY");
                }
            }

            return null;
        }

        public Double GetDouble(String fieldName)
        {
            object obj = null;

            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            if (!data.TryGetValue(fieldName, out obj))
                throw new System.Exception("field " + fieldName + " does not exist");

            if (!(obj is JsonObject && ((JsonObject)obj).ContainsKey(DOUBLE_FIELD)))
                throw new System.Exception("field " + fieldName + " is not of type DOUBLE");

            if (((JsonObject)obj)[DOUBLE_FIELD] is String)
                return Double.Parse((String)((JsonObject)obj)[DOUBLE_FIELD]);
            else
                return (Double)((JsonObject)obj)[DOUBLE_FIELD];
        }

        public Double[] GetDoubleArray(String fieldName)
        {
            object obj = null;

            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            if (!data.TryGetValue(fieldName, out obj))
                throw new System.Exception("field " + fieldName + " does not exist");

            if(obj is JsonArray)
            {
                JsonArray arr = (JsonArray) obj;

                if(arr.Count == 0)
                {
                    return new Double[0];
                }
                else if (arr[0] is JsonObject && ((JsonObject)arr[0]).ContainsKey(DOUBLE_FIELD))
                {
                    Double[] retVal = new Double[arr.Count];

                    for(int i=0,max=arr.Count;i<max;i++)
                    {
                        if (((JsonObject)arr[i])[DOUBLE_FIELD] is String)
                            retVal[i] = Double.Parse((String)((JsonObject)arr[i])[DOUBLE_FIELD]);
                        else
                            retVal[i] = (Double)((JsonObject)arr[i])[DOUBLE_FIELD];
                    }

                    return retVal;
                }
                else
                {
                    throw new System.Exception("field " + fieldName + " is not of type DOUBLE_ARRAY");
                }
            }

            return null;
        }

        public String[] GetFieldNames()
        {
            String[] names = new String[data.Count];

            data.Keys.CopyTo(names, 0);

            return names;
        }

        protected FieldType getFieldType(Object test)
        {
            FieldType type = 0;

            if(test is string)
            {
                type = FieldType.STRING;
            }
            else if(test is long)
            {
                type = FieldType.LONG;
            }
            else if(test is JsonObject)
            {
                JsonObject obj = (JsonObject)test;

                if (obj.ContainsKey(DOUBLE_FIELD))
                {
                    type = FieldType.DOUBLE;
                }
                else if(obj.ContainsKey(MILLISECOND_FIELD))
                {
                    type = FieldType.DATE;
                }
                else
                {
                    type = FieldType.MESSAGE;
                }
            }

            return type;
        }

        public FieldType GetFieldType(String fieldName)
        {
            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            Object test = data[fieldName];
            FieldType type  = 0;

            if(test is JsonArray)
            {
                JsonArray arr = (JsonArray) test;

                if(arr.Count > 0)
                {
                    type = getFieldType(arr[0]);

                    if(type == FieldType.STRING) type = FieldType.STRING_ARRAY;
                    else if(type == FieldType.LONG) type = FieldType.LONG_ARRAY;
                    else if(type == FieldType.DOUBLE) type = FieldType.DOUBLE_ARRAY;
                    else if(type == FieldType.MESSAGE) type = FieldType.MESSAGE_ARRAY;
                    else if(type == FieldType.DATE) type = FieldType.DATE_ARRAY;
                }
            }
            else
            {
                type = getFieldType(test);
            }

            return type;
        }

        public long GetLong(String fieldName)
        {
            object obj = null;

            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            if (!data.TryGetValue(fieldName, out obj))
                throw new System.Exception("field " + fieldName + " does not exist");

            if (!(obj is long))
                throw new System.Exception("field " + fieldName + " is not of type LONG");

            return (long)obj;
        }

        public long[] GetLongArray(String fieldName)
        {
            object obj = null;

            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            if (!data.TryGetValue(fieldName, out obj))
                throw new System.Exception("field " + fieldName + " does not exist");

            if(obj is JsonArray)
            {
                JsonArray arr = (JsonArray) obj;

                if(arr.Count == 0)
                {
                    return new long[0];
                }
                else if(arr[0] is long)
                {
                    long[] retVal = new long[arr.Count];

                    for(int i=0,max=arr.Count;i<max;i++)
                    {
                        retVal[i] = (long)arr[i];
                    }

                    return retVal;
                }
                else
                {
                    throw new System.Exception("field " + fieldName + " is not of type LONG_ARRAY");
                }
            }

            return null;
        }

        public IMessage GetMessage(String fieldName)
        {
            object obj = null;

            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            if (!data.TryGetValue(fieldName, out obj))
                throw new System.Exception("field " + fieldName + " does not exist");

            if (!(obj is JsonObject))
                throw new System.Exception("field " + fieldName + " is not of type MESSAGE");

            return new JSONMessage((JsonObject)obj);
        }

        public byte[] GetOpaque(String fieldName)
        {
            object obj = null;

            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            if (!data.TryGetValue(fieldName, out obj))
                throw new System.Exception("field " + fieldName + " does not exist");

            if (!(obj is JsonObject && ((JsonObject)obj).ContainsKey(OPAQUE_FIELD)))
                throw new System.Exception("field " + fieldName + " is not of type OPAQUE");

            if (((JsonObject)obj)[OPAQUE_FIELD] is String)
            {
                String s = (String)(((JsonObject)obj)[OPAQUE_FIELD]);
                return Convert.FromBase64String(s);
            }
            return new byte[0];
        }

        public IMessage[] GetMessageArray(String fieldName)
        {
            object obj = null;

            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            if (!data.TryGetValue(fieldName, out obj))
                throw new System.Exception("field " + fieldName + " does not exist");

            if(obj is JsonArray)
            {
                JsonArray arr = (JsonArray) obj;

                if(arr.Count == 0)
                {
                    return new IMessage[0];
                }
                else if(arr[0] is JsonObject)
                {
                    IMessage[] retVal = new IMessage[arr.Count];

                    for(int i=0,max=arr.Count;i<max;i++)
                    {
                        retVal[i] = new JSONMessage((JsonObject)arr[i]);
                    }

                    return retVal;
                }
                else
                {
                    throw new System.Exception("field " + fieldName + " is not of type MESSAGE_ARRAY");
                }
            }

            return null;
        }

        public String GetString(String fieldName)
        {
            object obj = null;

            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            if (!data.TryGetValue(fieldName, out obj))
                throw new System.Exception("field " + fieldName + " does not exist");

            if (!(obj is String))
                throw new System.Exception("field " + fieldName + " is not of type STRING");

            return (String)obj;
        }

        public String[] GetStringArray(String fieldName)
        {
            object obj = null;

            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            if (!data.TryGetValue(fieldName, out obj))
                throw new System.Exception("field " + fieldName + " does not exist");

            if(obj is JsonArray)
            {
                JsonArray arr = (JsonArray) obj;

                if(arr.Count == 0)
                {
                    return new String[0];
                }

                else if(arr[0] is String)
                {
                    String[] retVal = new String[arr.Count];

                    for(int i=0,max=arr.Count;i<max;i++)
                    {
                        retVal[i] = (String)arr[i];
                    }

                    return retVal;
                }
                else
                {
                    throw new System.Exception("field " + fieldName + " is not of type STRING_ARRAY");
                }
            }

            return null;
        }

        public bool IsFieldSet(String fieldName)
        {
            object obj = null;

            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            return data.TryGetValue(fieldName, out obj);
        }

        public void SetArray(String fieldName, IMessage[] value)
        {
            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            data.Remove(fieldName);

            if (value != null)
            {
                JsonArray arr = new JsonArray();

                for (int i = 0, max = value.Length; i < max; i++)
                {
                    arr.Add(JsonValue.Parse(((JSONMessage)value[i]).data.ToString()));
                }

                data.Add(fieldName, arr);
            }
        }

        public void SetArray(String fieldName, String[] value)
        {
            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            data.Remove(fieldName);

            if (value != null)
            {
                JsonArray arr = new JsonArray();

                for (int i = 0, max = value.Length; i < max; i++)
                {
                    arr.Add(value[i]);
                }

                data.Add(fieldName, arr);
            }
        }

        public void SetArray(String fieldName, long[] value)
        {
            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            data.Remove(fieldName);

            if (value != null)
            {
                JsonArray arr = new JsonArray();

                for (int i = 0, max = value.Length; i < max; i++)
                {
                    arr.Add(value[i]);
                }

                data.Add(fieldName, arr);
            }
        }

        public void SetArray(String fieldName, Double[] value)
        {
            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            data.Remove(fieldName);

            if (value != null)
            {
                JsonArray arr = new JsonArray();

                for (int i = 0, max = value.Length; i < max; i++)
                {
                    JsonObject obj = new JsonObject();

                    if (Double.IsNaN(value[i]) || Double.IsInfinity(value[i]))
                        obj.Add(DOUBLE_FIELD, value[i].ToString());
                    else
                        obj.Add(DOUBLE_FIELD, value[i]);

                    arr.Add(obj);
                }

                data.Add(fieldName, arr);
            }
        }

        public void SetArray(String fieldName, DateTime[] value)
        {
            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            data.Remove(fieldName);

            if (value != null)
            {
                JsonArray arr = new JsonArray();

                for (int i = 0, max = value.Length; i < max; i++)
                {
                    JsonObject obj = new JsonObject();
                    obj.Add(MILLISECOND_FIELD, ((long)((value[i] - new DateTime(1970, 1, 1)).TotalMilliseconds)));

                    arr.Add(obj);
                }

                data.Add(fieldName, arr);
            }
        }

        public void SetDateTime(String fieldName, DateTime value)
        {
            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            data.Remove(fieldName);

            JsonObject obj = new JsonObject();
            obj.Add(MILLISECOND_FIELD, (long)((value - new DateTime(1970, 1, 1)).TotalMilliseconds));

            data.Add(fieldName, obj);
        }

        public void SetDouble(String fieldName, Double value)
        {
            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            data.Remove(fieldName);

            JsonObject obj = new JsonObject();

            if (Double.IsNaN(value) || Double.IsInfinity(value))
                obj.Add(DOUBLE_FIELD, value.ToString());
            else
                obj.Add(DOUBLE_FIELD, value);

            data.Add(fieldName, obj);
        }

        public void SetLong(String fieldName, long value)
        {
            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            data.Remove(fieldName);
            data.Add(fieldName, value);
        }

        public void SetMessage(String fieldName, IMessage value)
        {
            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            data.Remove(fieldName);

            if (value != null)
            {
                data.Add(fieldName, JsonValue.Parse(((JSONMessage)value).data.ToString()));
            }
        }

        public void SetOpaque(String fieldName, byte[] value)
        {
            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            data.Remove(fieldName);

            if (value != null)
            {
                JsonObject obj = new JsonObject();
                obj.Add(OPAQUE_FIELD, Convert.ToBase64String(value));
                data.Add(fieldName, obj);
            }
        }

        public void SetString(String fieldName, String value)
        {
            if (String.IsNullOrEmpty(fieldName))
                throw new System.ArgumentException("field name is null or empty");

            data.Remove(fieldName);

            if (value != null)
            {
                data.Add(fieldName, value);
            }
        }

        public override String ToString() 
        {
            StringBuilder sb = new StringBuilder();
            sb.Append('{');
            foreach (String name in GetFieldNames())
            {
                sb.Append(name);
                sb.Append(':');
                switch (GetFieldType(name))
                {
                case FieldType.STRING:
                    sb.Append("string=");
                    sb.Append('"');
                    sb.Append(GetString(name));
                    sb.Append('"');
                    break;
                case FieldType.LONG:
                    sb.Append("long=");
                    sb.Append(GetLong(name));
                    break;
                case FieldType.DOUBLE:
                    sb.Append("double=");
                    sb.Append(GetDouble(name));
                    break;
                case FieldType.DATE:
                    sb.Append("date=");
                    sb.Append(GetDateTime(name));
                    break;
                case FieldType.MESSAGE:
                    sb.Append("message=");
                    sb.Append(GetMessage(name));
                    break;
                case FieldType.STRING_ARRAY:
                    sb.Append("string_array=");
                    sb.Append('[');
                    foreach (String elem in GetStringArray(name))
                    {
                        sb.Append('"');
                        sb.Append(elem);
                        sb.Append('"');
                        sb.Append(", ");
                    }
                    sb.Length = sb.Length - 2;
                    sb.Append(']');
                    break;
                case FieldType.LONG_ARRAY:
                    sb.Append("long_array=");
                    sb.Append('[');
                    foreach (long elem in GetLongArray(name))
                    {
                        sb.Append(elem);
                        sb.Append(", ");
                    }
                    sb.Length = sb.Length - 2;
                    sb.Append(']');
                    break;
                case FieldType.DOUBLE_ARRAY:
                    sb.Append("double_array=");
                    sb.Append('[');
                    foreach (Double elem in GetDoubleArray(name))
                    {
                        sb.Append(elem);
                        sb.Append(", ");
                    }
                    sb.Length = sb.Length - 2;
                    sb.Append(']');
                    break;
                case FieldType.DATE_ARRAY:
                    sb.Append("date_array=");
                    sb.Append('[');
                    foreach (DateTime elem in GetDateTimeArray(name))
                    {
                        sb.Append(elem);
                        sb.Append(", ");
                    }
                    sb.Length = sb.Length - 2;
                    sb.Append(']');
                    break;
                case FieldType.MESSAGE_ARRAY:
                    sb.Append("message_array=");
                    sb.Append('[');
                    foreach (IMessage elem in GetMessageArray(name))
                    {
                        sb.Append(elem);
                        sb.Append(", ");
                    }
                    sb.Length = sb.Length - 2;
                    sb.Append(']');
                    break;
                default:
                    sb.Append("unknown=");
                    sb.Append(data[name]);
                    break;
                }
                sb.Append(", ");
            }
            sb.Length = System.Math.Max((sb.Length - 2), 1);
            sb.Append('}');
            return sb.ToString();
        }
  
        public void ClearField(String name)
        {
            object obj = null;

            if (!data.TryGetValue(name, out obj))
                throw new Exception("field " + name + " does not exist");

            // remove the field by name.
            data.Remove(name);
        }
    }
}

