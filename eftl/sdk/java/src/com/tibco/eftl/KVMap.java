package com.tibco.eftl;

/**
 * Key-value map objects allow setting, getting, and removing key-value pairs 
 * in an FTL map. 
 */
public interface KVMap {

    /**
     * Set a key-value pair in the map, overwriting any existing value.
     *
     * @param key Set the value for this key.
     * @param value Set this value for the key.
     * @param listener This listener defines callback methods for
     *                 successful completion and for errors.
     *
     * @throws IllegalStateException The connection is not open.
     * @throws IllegalArgumentException The message would exceed the
     *         eFTL server's maximum message size.
     */
    public void set(String key, Message value, KVMapListener listener);

    /**
     * Get the value of a key from the map, or <code>null</code> if the key is not set.
     *  
     * @param key Get the value for this key.
     * @param listener This listener defines callback methods for
     *                 successful completion and for errors.
     * 
     * @throws IllegalStateException The connection is not open.
     */
    public void get(String key, KVMapListener listener);
    
    /**
     * Remove a key-value pair from the map.
     *
     * @param key Remove the value for this key.
     * @param listener This listener defines callback methods for
     *                 successful completion and for errors.
     *
     * @throws IllegalStateException The connection is not open.
     */
    public void remove(String key, KVMapListener listener);
}
