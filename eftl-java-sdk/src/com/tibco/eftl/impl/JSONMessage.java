/*
 * Copyright (c) 2001-$Date: 2020-09-24 12:20:18 -0700 (Thu, 24 Sep 2020) $ TIBCO Software Inc.
 * All rights reserved.
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: JSONMessage.java 128796 2020-09-24 19:20:18Z bpeterse $
 *
 */
package com.tibco.eftl.impl;

import java.util.Arrays;
import java.util.Date;

import com.tibco.eftl.Message;
import com.tibco.eftl.json.JsonArray;
import com.tibco.eftl.json.JsonObject;
import com.tibco.eftl.json.JsonValue;

public class JSONMessage implements Message
{
    public static final String DOUBLE_FIELD = "_d_";
    public static final String MILLISECOND_FIELD = "_m_";
    public static final String OPAQUE_FIELD = "_o_";
    
    protected JsonObject json;
    protected long seqNum;
    protected long reqId;
    protected long msgId;
    protected long deliveryCount;
    protected String subId;
    protected String replyTo;
    
    public JSONMessage(JsonObject jsonObject)
    {
        this.json = jsonObject;
    }

    public JSONMessage()
    {
        this(new JsonObject());
    }
    
    protected void setReceipt(long seqNum, String subId)
    {
        this.seqNum = seqNum;
        this.subId = subId;
    }

    protected void setReplyTo(String replyTo, long reqId)
    {
        this.replyTo = replyTo;
        this.reqId = reqId;
    }

    protected void setStoreMessageId(long msgId)
    {
        this.msgId = msgId;
    }

    protected void setDeliveryCount(long deliveryCount)
    {
        this.deliveryCount = deliveryCount;
    }

    protected JsonObject toJsonObject()
    {
        return json;
    }
    
    @Override
    public long getStoreMessageId()
    {
        return msgId;
    }

    @Override
    public long getDeliveryCount()
    {
        return deliveryCount;
    }

    @Override
    public boolean isFieldSet(String fieldName)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");
        
        return json.containsKey(fieldName);
    }
    
    @Override
    public String[] getFieldNames()
    {
        return (String[]) json.keySet().toArray(new String[0]);
    }

    @Override
    public FieldType getFieldType(String fieldName)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");
        
        Object test = json.get(fieldName);
        FieldType type = null;
        
        if(test instanceof JsonArray)
        {
            JsonArray arr = (JsonArray) test;
            
            if(arr.size()>0)
            {
                type = getFieldType(arr.get(0));

                if(type == FieldType.STRING) type = FieldType.STRING_ARRAY;
                else if(type == FieldType.LONG) type = FieldType.LONG_ARRAY;
                else if(type == FieldType.DOUBLE) type = FieldType.DOUBLE_ARRAY;
                else if(type == FieldType.MESSAGE) type = FieldType.MESSAGE_ARRAY;
                else if(type == FieldType.DATE) type = FieldType.DATE_ARRAY;
            }
            else
            {
                type = FieldType.UNKNOWN;
            }
        }
        else
        {
            type = getFieldType(test);
        }
        
        return type;
    }

    protected FieldType getFieldType(Object test)
    {
        FieldType type = null;
        
        if(test instanceof String)
        {
            type = FieldType.STRING;
        }
        else if(test instanceof Number)
        {
            type = FieldType.LONG;
        }
        else if(test instanceof JsonObject)
        {
            JsonObject obj = (JsonObject) test;
            
            if(obj.containsKey(DOUBLE_FIELD))
            {
                type = FieldType.DOUBLE;
            }
            else if(obj.containsKey(MILLISECOND_FIELD))
            {
                type = FieldType.DATE;
            }
            else if(obj.containsKey(OPAQUE_FIELD))
            {
                type = FieldType.OPAQUE;
            }
            else
            {
                type = FieldType.MESSAGE;
            }
        }
        
        return type;
    }

    @Override
    public void setString(String fieldName, String value)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");
        
        if(value == null) json.remove(fieldName);
        else json.put(fieldName, value);
    }

    @Override
    public String getString(String fieldName)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");
        
        Object o = json.get(fieldName);
        return (String) (o instanceof String ? o : null);
    }

    @Override
    public void setLong(String fieldName, Long value)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");
        
        if(value == null) json.remove(fieldName);
        else json.put(fieldName, value);
    }

    @Override
    public Long getLong(String fieldName)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");

        Object o = json.get(fieldName);
        if (o instanceof Number)
            return new Long(((Number) o).longValue());
        return null;
    }
    
    @Override
    public void setDouble(String fieldName, Double value)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");
        
        if (value == null)
        {
            json.remove(fieldName);
        }
        else 
        {
            JsonObject obj = new JsonObject();
            if (value.isNaN() || value.isInfinite())
                obj.put(DOUBLE_FIELD, value.toString());
            else
                obj.put(DOUBLE_FIELD,  value);
            
            json.put(fieldName, obj);
        }
    }

    @Override
    public Double getDouble(String fieldName)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");
        
        Object o = json.get(fieldName);
        if (o instanceof JsonObject && ((JsonObject) o).containsKey(DOUBLE_FIELD))
        {
            Object value = ((JsonObject) o).get(DOUBLE_FIELD);
            if (value instanceof String)
                return Double.parseDouble((String) value);
            else if (value instanceof Number)
                return new Double(((Number) value).doubleValue());
        }
        return null;
    }
    
    @Override
    public void setDate(String fieldName, Date value)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");
        
        if (value==null)
        {
            json.remove(fieldName);
        }
        else
        {
            JsonObject obj = new JsonObject();
            obj.put(MILLISECOND_FIELD, value.getTime());
            
            json.put(fieldName, obj);
        }
    }
    
    @Override
    public Date getDate(String fieldName)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");
        
        Object o = json.get(fieldName);
        return (Date) (o instanceof JsonObject && ((JsonObject) o).containsKey(MILLISECOND_FIELD) ? new Date((Long) ((JsonObject) o).get(MILLISECOND_FIELD)): null);
    }
    
    @Override
    public void setMessage(String fieldName, Message value)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");
        
        if(value == null) json.remove(fieldName);
        else json.put(fieldName, JsonValue.parse(((JSONMessage) value).json.toString()));
    }

    @Override
    public Message getMessage(String fieldName)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");
        
        Object o = json.get(fieldName);
        return (Message) (o instanceof JsonObject ? new JSONMessage((JsonObject) o) : null);
    }
    
    @Override
    public void setOpaque(String fieldName, byte[] value)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");

        if (value == null)
        {
            json.remove(fieldName);
        }
        else
        {
            JsonObject obj = new JsonObject();
            obj.put(OPAQUE_FIELD, Base64.encode(value));
            
            json.put(fieldName, obj);
        }
    }
    
    @Override
    public byte[] getOpaque(String fieldName)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");

        Object o = json.get(fieldName);
        if (o instanceof JsonObject && ((JsonObject) o).containsKey(OPAQUE_FIELD))
        {
            Object value = ((JsonObject) o).get(OPAQUE_FIELD);
            if (value instanceof String)
                return Base64.decode(((String) value).getBytes());
        } 
        return new byte[0];
    }
    
    @Override
    public void setArray(String fieldName, String[] values)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");
        
        if(values == null)
        {
            json.remove(fieldName);
            return;
        }
        
        JsonArray array = new JsonArray();
        array.addAll(Arrays.asList(values));
        
        json.put(fieldName, array);
    }

    @Override
    public void setArray(String fieldName, Long[] values)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");
        
        if(values == null)
        {
            json.remove(fieldName);
            return;
        }
        
        JsonArray array = new JsonArray();
        array.addAll(Arrays.asList(values));
        
        json.put(fieldName, array);
    }

    @Override
    public void setArray(String fieldName, Double[] values)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");
        
        if(values == null)
        {
            json.remove(fieldName);
            return;
        }
        
        JsonArray arr = new JsonArray();
        
        for(int i=0,max=values.length;i<max;i++)
        {
            JsonObject obj = new JsonObject();
            
            if (values[i].isNaN() || values[i].isInfinite())
                obj.put(DOUBLE_FIELD, values[i].toString());
            else
                obj.put(DOUBLE_FIELD, values[i]);
            
            arr.add(obj);
        }
        
        json.put(fieldName, arr);
    }

    @Override
    public void setArray(String fieldName, Date[] values)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");
        
        if(values == null)
        {
            json.remove(fieldName);
            return;
        }
        
        JsonArray arr = new JsonArray();
        
        for(int i=0,max=values.length;i<max;i++)
        {
            JsonObject obj = new JsonObject();
            obj.put(MILLISECOND_FIELD, values[i].getTime());
            
            arr.add(obj);
        }
        
        json.put(fieldName, arr);
    }
    
    @Override
    public void setArray(String fieldName, Message[] values)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");
        
        if(values == null)
        {
            json.remove(fieldName);
            return;
        }
        
        JsonArray arr = new JsonArray();
        
        for(int i=0,max=values.length;i<max;i++)
        {
            arr.add(JsonValue.parse(((JSONMessage) values[i]).json.toString()));
        }
        
        json.put(fieldName, arr);
    }

    @Override
    public String[] getStringArray(String fieldName)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");
        
        Object test = json.get(fieldName);
        
        if(test instanceof JsonArray)
        {
            JsonArray arr = (JsonArray) test;
            
            if(arr.size()==0)
            {
                return new String[0];
            }
            else if (arr.get(0) instanceof String)
            {
                String[] retVal = new String[arr.size()];
                
                for(int i=0,max=arr.size();i<max;i++)
                {
                    retVal[i] = (String) arr.get(i);
                }
                
                return retVal;
            }
        }
        
        return null;
    }

    @Override
    public Long[] getLongArray(String fieldName)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");
        
        Object test = json.get(fieldName);
        
        if(test instanceof JsonArray)
        {
            JsonArray arr = (JsonArray) test;
            
            if(arr.size()==0)
            {
                return new Long[0];
            }
            else if (arr.get(0) instanceof Number)
            {
                Long[] retVal = new Long[arr.size()];
                
                for (int i = 0, max=arr.size(); i < max; i++)
                {
                    if (arr.get(i) instanceof Number)
                        retVal[i] = new Long(((Number) arr.get(i)).longValue());
                }
                
                return retVal;
            }
        }
        
        return null;
    }

    @Override
    public Double[] getDoubleArray(String fieldName)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");
        
        Object test = json.get(fieldName);
        
        if(test instanceof JsonArray)
        {
            JsonArray arr = (JsonArray) test;
            
            if(arr.size()==0)
            {
                return new Double[0];
            }
            else if(arr.get(0) instanceof JsonObject && ((JsonObject) arr.get(0)).containsKey(DOUBLE_FIELD))
            {
                Double[] retVal = new Double[arr.size()];
                
                for(int i=0,max=arr.size();i<max;i++)
                {
                    Object value = ((JsonObject) arr.get(i)).get(DOUBLE_FIELD);
                    
                    if (value instanceof String)
                        retVal[i] = Double.parseDouble((String) value);
                    else if (value instanceof Number)
                        retVal[i] = new Double(((Number) value).doubleValue());
                }
                
                return retVal;
            }
        }
        
        return null;
    }

    public Date[] getDateArray(String fieldName)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");
        
        Object test = json.get(fieldName);
        
        if(test instanceof JsonArray)
        {
            JsonArray arr = (JsonArray) test;
            
            if(arr.size()==0)
            {
                return new Date[0];
            }
            else if(arr.get(0) instanceof JsonObject && ((JsonObject) arr.get(0)).containsKey(MILLISECOND_FIELD))
            {
                Date[] retVal = new Date[arr.size()];
                    
                for(int i=0,max=arr.size();i<max;i++)
                {
                    retVal[i] = new Date((Long) ((JsonObject) arr.get(i)).get(MILLISECOND_FIELD));
                }
                    
                return retVal;
            }
        }
        
        return null;
    }

    @Override
    public Message[] getMessageArray(String fieldName)
    {
        if (fieldName == null || fieldName.isEmpty())
            throw new IllegalArgumentException("field name is null or empty");
        
        Object test = json.get(fieldName);
        
        if(test instanceof JsonArray)
        {
            JsonArray arr = (JsonArray) test;
            
            if(arr.size()==0)
            {
                return new Message[0];
            }
            else if(arr.get(0) instanceof JsonObject)
            {
                Message[] retVal = new Message[arr.size()];
                
                for(int i=0,max=arr.size();i<max;i++)
                {
                    retVal[i] = new JSONMessage((JsonObject) arr.get(i));
                }
                
                return retVal;
            }
        }
        
        return null;
    }
    
    @Override
    public int hashCode()
    {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((json == null) ? 0 : json.hashCode());
        return result;
    }

    @Override
    public boolean equals(Object obj)
    {
        if(this == obj)
            return true;
        if(obj == null)
            return false;
        if(getClass() != obj.getClass())
            return false;
        JSONMessage other = (JSONMessage) obj;
        if(json == null)
        {
            if(other.json != null)
                return false;
        }
        else if(!json.equals(other.json))
            return false;
        return true;
    }

    @Override
    public String toString() 
    {
        StringBuilder sb = new StringBuilder();
        sb.append('{');
        for (String name : getFieldNames())
        {
            sb.append(name);
            sb.append(':');
            switch (getFieldType(name))
            {
            case STRING:
                sb.append("string=");
                sb.append('"');
                sb.append(getString(name));
                sb.append('"');
                break;
            case LONG:
                sb.append("long=");
                sb.append(getLong(name));
                break;
            case DOUBLE:
                sb.append("double=");
                sb.append(getDouble(name));
                break;
            case DATE:
                sb.append("date=");
                sb.append(getDate(name));
                break;
            case MESSAGE:
                sb.append("message=");
                sb.append(getMessage(name));
                break;
            case OPAQUE:
                sb.append("opaque=");
                sb.append(Arrays.toString(getOpaque(name)));
                break;
            case STRING_ARRAY:
                sb.append("string_array=");
                sb.append('[');
                for (String elem : getStringArray(name))
                {
                    sb.append('"');
                    sb.append(elem);
                    sb.append('"');
                    sb.append(", ");
                }
                sb.setLength(sb.length() - 2);
                sb.append(']');
                break;
            case LONG_ARRAY:
                sb.append("long_array=");
                sb.append('[');
                for (Long elem : getLongArray(name))
                {
                    sb.append(elem);
                    sb.append(", ");
                }
                sb.setLength(sb.length() - 2);
                sb.append(']');
                break;
            case DOUBLE_ARRAY:
                sb.append("double_array=");
                sb.append('[');
                for (Double elem : getDoubleArray(name))
                {
                    sb.append(elem);
                    sb.append(", ");
                }
                sb.setLength(sb.length() - 2);
                sb.append(']');
                break;
            case DATE_ARRAY:
                sb.append("date_array=");
                sb.append('[');
                for (Date elem : getDateArray(name))
                {
                    sb.append(elem);
                    sb.append(", ");
                }
                sb.setLength(sb.length() - 2);
                sb.append(']');
                break;
            case MESSAGE_ARRAY:
                sb.append("message_array=");
                sb.append('[');
                for (Message elem : getMessageArray(name))
                {
                    sb.append(elem);
                    sb.append(", ");
                }
                sb.setLength(sb.length() - 2);
                sb.append(']');
                break;
            default:
                sb.append("unknown=");
                sb.append(json.get(name));
            }
            sb.append(", ");
        }
        sb.setLength(Math.max(sb.length() - 2, 1));
        sb.append('}');
        return sb.toString();
    }
}
