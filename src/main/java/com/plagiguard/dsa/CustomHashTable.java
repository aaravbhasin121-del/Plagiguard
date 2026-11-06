package com.plagiguard.dsa;

import java.util.ArrayList;
import java.util.List;

/**
 * CustomHashTable - A generic hash table implementation using separate chaining
 * for collision resolution.
 *
 * <p>This implementation does NOT use Java's built-in HashMap. It manually implements:
 * <ul>
 *   <li>A hash function based on key.hashCode() &amp; (capacity - 1)</li>
 *   <li>A bucket array with linked-list chaining via an internal {@link Entry} node class</li>
 *   <li>Dynamic resizing when the load factor exceeds the threshold (0.75)</li>
 * </ul>
 *
 * <p><b>Time Complexity:</b>
 * <ul>
 *   <li>{@code put}:    O(1) average, O(n) worst case (all keys in one bucket)</li>
 *   <li>{@code get}:    O(1) average, O(n) worst case</li>
 *   <li>{@code remove}: O(1) average, O(n) worst case</li>
 *   <li>{@code resize}: O(n) – rehashes all entries</li>
 * </ul>
 *
 * <p><b>Space Complexity:</b> O(n + m) where n = number of entries, m = bucket count
 *
 * <p><b>Used in PlagiGuard for:</b>
 * <ul>
 *   <li>N-gram fingerprint storage and lookup</li>
 *   <li>Document indexing by fingerprint hash</li>
 *   <li>Matching segment retrieval</li>
 *   <li>Duplicate document detection</li>
 * </ul>
 *
 * @param <K> the type of keys maintained by this table
 * @param <V> the type of mapped values
 */
public class CustomHashTable<K, V> {

    // -----------------------------------------------------------------------
    // Constants
    // -----------------------------------------------------------------------

    /** Default number of buckets when no initial capacity is provided. */
    private static final int DEFAULT_CAPACITY = 16;

    /**
     * Load-factor threshold above which the table is resized.
     * A value of 0.75 balances memory usage and lookup performance.
     */
    private static final double LOAD_FACTOR_THRESHOLD = 0.75;

    // -----------------------------------------------------------------------
    // Internal linked-list node
    // -----------------------------------------------------------------------

    /**
     * Internal singly-linked node used for separate chaining within a bucket.
     *
     * @param <K> key type
     * @param <V> value type
     */
    private static class Entry<K, V> {
        /** The key stored in this entry. */
        final K key;
        /** The value associated with the key (mutable for updates). */
        V value;
        /** Pointer to the next entry in the same bucket chain. */
        Entry<K, V> next;

        Entry(K key, V value) {
            this.key = key;
            this.value = value;
            this.next = null;
        }
    }

    // -----------------------------------------------------------------------
    // Fields
    // -----------------------------------------------------------------------

    /** The bucket array; each slot is the head of a singly-linked chain. */
    @SuppressWarnings("unchecked")
    private Entry<K, V>[] buckets;

    /** Current number of key-value mappings stored in the table. */
    private int size;

    /** Current number of buckets (always a power of two for fast hashing). */
    private int capacity;

    // -----------------------------------------------------------------------
    // Constructors
    // -----------------------------------------------------------------------

    /**
     * Constructs a CustomHashTable with the default initial capacity (16).
     */
    @SuppressWarnings("unchecked")
    public CustomHashTable() {
        this.capacity = DEFAULT_CAPACITY;
        this.buckets = new Entry[capacity];
        this.size = 0;
    }

    /**
     * Constructs a CustomHashTable with the specified initial capacity.
     * The capacity is rounded up to the next power of two to enable the
     * fast bitwise hash index computation.
     *
     * @param initialCapacity the desired initial capacity (must be &gt; 0)
     */
    @SuppressWarnings("unchecked")
    public CustomHashTable(int initialCapacity) {
        if (initialCapacity <= 0) {
            throw new IllegalArgumentException("Initial capacity must be positive: " + initialCapacity);
        }
        // Round up to next power of two
        this.capacity = nextPowerOfTwo(initialCapacity);
        this.buckets = new Entry[capacity];
        this.size = 0;
    }

    // -----------------------------------------------------------------------
    // Hashing
    // -----------------------------------------------------------------------

    /**
     * Computes the bucket index for the given key.
     *
     * <p>Uses {@code key.hashCode()} mixed with a Wang/Jenkins-style spread
     * and then masked to the current capacity. Using {@code (capacity - 1)}
     * as the mask is valid because the capacity is always a power of two.
     *
     * <p><b>Time Complexity:</b> O(1)
     *
     * @param key the key to hash (must not be {@code null})
     * @return the bucket index in the range [0, capacity)
     */
    private int hashCode(K key) {
        if (key == null) {
            return 0; // null keys map to bucket 0
        }
        int h = key.hashCode();
        // Spread high bits to low to reduce clustering
        h ^= (h >>> 16);
        return h & (capacity - 1);
    }

    // -----------------------------------------------------------------------
    // Core Operations
    // -----------------------------------------------------------------------

    /**
     * Associates the specified value with the specified key in this table.
     * If the table previously contained a mapping for the key, the old value
     * is replaced by the new value.
     *
     * <p><b>Time Complexity:</b> O(1) amortised average; O(n) worst case when all
     * keys collide into the same bucket. Resizing is O(n) but amortised O(1).
     *
     * @param key   the key (may be {@code null}, stored in bucket 0)
     * @param value the value to associate
     */
    public void put(K key, V value) {
        // Resize before inserting if the load factor would be exceeded
        if ((double) (size + 1) / capacity > LOAD_FACTOR_THRESHOLD) {
            resize();
        }

        int index = hashCode(key);
        Entry<K, V> head = buckets[index];

        // Walk the chain – update if key already exists
        for (Entry<K, V> current = head; current != null; current = current.next) {
            if (keysEqual(current.key, key)) {
                current.value = value;
                return;
            }
        }

        // Key not found – prepend new entry (O(1) insertion at head)
        Entry<K, V> newEntry = new Entry<>(key, value);
        newEntry.next = head;
        buckets[index] = newEntry;
        size++;
    }

    /**
     * Returns the value to which the specified key is mapped, or {@code null}
     * if this table contains no mapping for the key.
     *
     * <p><b>Time Complexity:</b> O(1) average, O(n) worst case.
     *
     * @param key the key whose associated value is to be returned
     * @return the value, or {@code null} if not found
     */
    public V get(K key) {
        int index = hashCode(key);
        for (Entry<K, V> current = buckets[index]; current != null; current = current.next) {
            if (keysEqual(current.key, key)) {
                return current.value;
            }
        }
        return null;
    }

    /**
     * Removes the mapping for the specified key from this table if present.
     *
     * <p><b>Time Complexity:</b> O(1) average, O(n) worst case.
     *
     * @param key the key whose mapping is to be removed
     * @return {@code true} if a mapping was found and removed, {@code false} otherwise
     */
    public boolean remove(K key) {
        int index = hashCode(key);
        Entry<K, V> current = buckets[index];
        Entry<K, V> prev = null;

        while (current != null) {
            if (keysEqual(current.key, key)) {
                if (prev == null) {
                    // Removing the head of the chain
                    buckets[index] = current.next;
                } else {
                    prev.next = current.next;
                }
                size--;
                return true;
            }
            prev = current;
            current = current.next;
        }
        return false; // Key not present
    }

    /**
     * Returns {@code true} if this table contains a mapping for the specified key.
     *
     * <p><b>Time Complexity:</b> O(1) average, O(n) worst case.
     *
     * @param key the key to test
     * @return {@code true} if the key is present
     */
    public boolean containsKey(K key) {
        int index = hashCode(key);
        for (Entry<K, V> current = buckets[index]; current != null; current = current.next) {
            if (keysEqual(current.key, key)) {
                return true;
            }
        }
        return false;
    }

    // -----------------------------------------------------------------------
    // Metrics
    // -----------------------------------------------------------------------

    /**
     * Returns the number of key-value mappings in this table.
     *
     * <p><b>Time Complexity:</b> O(1)
     *
     * @return the number of entries
     */
    public int size() {
        return size;
    }

    /**
     * Returns {@code true} if this table contains no mappings.
     *
     * <p><b>Time Complexity:</b> O(1)
     *
     * @return {@code true} if empty
     */
    public boolean isEmpty() {
        return size == 0;
    }

    /**
     * Returns the current load factor, defined as {@code size / capacity}.
     * When this exceeds 0.75 the table is automatically resized on the next put.
     *
     * <p><b>Time Complexity:</b> O(1)
     *
     * @return the load factor as a double in [0.0, 1.0+)
     */
    public double getLoadFactor() {
        return (double) size / capacity;
    }

    /**
     * Returns the current bucket capacity of this table.
     *
     * @return the number of buckets
     */
    public int getCapacity() {
        return capacity;
    }

    // -----------------------------------------------------------------------
    // Bulk Accessors
    // -----------------------------------------------------------------------

    /**
     * Returns a {@link List} containing all keys in this table.
     * The order is unspecified and may change after resizing.
     *
     * <p><b>Time Complexity:</b> O(n + m) where n = entries, m = bucket count
     *
     * @return list of all keys
     */
    public List<K> keys() {
        List<K> result = new ArrayList<>(size);
        for (int i = 0; i < capacity; i++) {
            for (Entry<K, V> current = buckets[i]; current != null; current = current.next) {
                result.add(current.key);
            }
        }
        return result;
    }

    /**
     * Returns a {@link List} containing all values in this table.
     * The order corresponds to the order returned by {@link #keys()}.
     *
     * <p><b>Time Complexity:</b> O(n + m) where n = entries, m = bucket count
     *
     * @return list of all values
     */
    public List<V> values() {
        List<V> result = new ArrayList<>(size);
        for (int i = 0; i < capacity; i++) {
            for (Entry<K, V> current = buckets[i]; current != null; current = current.next) {
                result.add(current.value);
            }
        }
        return result;
    }

    /**
     * Removes all mappings from this table. The capacity is NOT reduced.
     *
     * <p><b>Time Complexity:</b> O(m) where m = bucket count
     */
    @SuppressWarnings("unchecked")
    public void clear() {
        buckets = new Entry[capacity];
        size = 0;
    }

    // -----------------------------------------------------------------------
    // Internal Helpers
    // -----------------------------------------------------------------------

    /**
     * Doubles the capacity and rehashes every existing entry into the new bucket array.
     *
     * <p>This is called automatically by {@link #put} when the load factor exceeds the
     * threshold. After resizing, all bucket indices are recomputed because the capacity
     * (and therefore the bitmask) has changed.
     *
     * <p><b>Time Complexity:</b> O(n + m) – visits every entry and every bucket
     */
    @SuppressWarnings("unchecked")
    private void resize() {
        int oldCapacity = capacity;
        Entry<K, V>[] oldBuckets = buckets;

        capacity = oldCapacity * 2;
        buckets = new Entry[capacity];
        size = 0; // will be re-incremented by put()

        // Rehash all existing entries into the new bucket array
        for (int i = 0; i < oldCapacity; i++) {
            for (Entry<K, V> current = oldBuckets[i]; current != null; current = current.next) {
                put(current.key, current.value);
            }
        }
    }

    /**
     * Null-safe key equality check.
     *
     * @param a first key
     * @param b second key
     * @return {@code true} if both are null or {@code a.equals(b)}
     */
    private boolean keysEqual(K a, K b) {
        if (a == null) return b == null;
        return a.equals(b);
    }

    /**
     * Returns the smallest power of two that is &gt;= {@code n}.
     *
     * @param n the input (must be &gt; 0)
     * @return the next power of two
     */
    private int nextPowerOfTwo(int n) {
        int p = 1;
        while (p < n) {
            p <<= 1;
        }
        return p;
    }

    // -----------------------------------------------------------------------
    // Object overrides
    // -----------------------------------------------------------------------

    /**
     * Returns a concise string representation showing the number of entries
     * and the current load factor.
     *
     * @return descriptive string, e.g. {@code "CustomHashTable{size=42, loadFactor=0.66}"}
     */
    @Override
    public String toString() {
        return String.format("CustomHashTable{size=%d, capacity=%d, loadFactor=%.2f}",
                size, capacity, getLoadFactor());
    }
}
