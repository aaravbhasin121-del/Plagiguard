package com.plagiguard.dsa;

import java.util.ArrayList;
import java.util.List;

/**
 * MaxHeap - A custom max-heap data structure that ranks documents by similarity score.
 *
 * <p>A max-heap is a complete binary tree stored in an array where every parent node has a
 * value &ge; both its children. This guarantees that the root always holds the maximum element,
 * making both {@link #peek()} and {@link #extractMax()} O(1) and O(log n) respectively.
 *
 * <p>PlagiGuard uses this heap to efficiently retrieve the top-K most similar documents to a
 * query document without sorting the entire similarity list.
 *
 * <p><b>Time Complexity:</b>
 * <ul>
 *   <li>{@code insert}:     O(log n) – bubble up from leaf to root</li>
 *   <li>{@code extractMax}: O(log n) – move last element to root then heapify down</li>
 *   <li>{@code peek}:       O(1)     – read array index 0</li>
 *   <li>{@code buildHeap}:  O(n)     – Floyd's algorithm (heapify each internal node)</li>
 *   <li>{@code getTopK}:    O(k log n)</li>
 * </ul>
 *
 * <p><b>Space Complexity:</b> O(n) for the backing array
 */
public class MaxHeap {

    // -----------------------------------------------------------------------
    // Inner Class
    // -----------------------------------------------------------------------

    /**
     * Represents a single document entry to be ranked in the heap.
     *
     * <p>Ordering is determined by {@code similarityScore} (higher = higher priority).
     */
    public static class HeapEntry implements Comparable<HeapEntry> {
        /** Human-readable document name (e.g. filename). */
        public final String documentName;
        /** Similarity score in [0.0, 1.0]. Higher values indicate more similarity. */
        public final double similarityScore;
        /** Unique numeric identifier for the document. */
        public final int documentId;

        /**
         * Constructs a HeapEntry.
         *
         * @param documentName   the document name or identifier string
         * @param similarityScore similarity score in [0.0, 1.0]
         * @param documentId     unique numeric document ID
         */
        public HeapEntry(String documentName, double similarityScore, int documentId) {
            this.documentName = documentName;
            this.similarityScore = similarityScore;
            this.documentId = documentId;
        }

        /**
         * Compares two entries by similarity score (ascending, so that max-heap parent
         * comparisons use &gt; naturally).
         */
        @Override
        public int compareTo(HeapEntry other) {
            return Double.compare(this.similarityScore, other.similarityScore);
        }

        @Override
        public String toString() {
            return String.format("HeapEntry{id=%d, name='%s', score=%.4f}",
                    documentId, documentName, similarityScore);
        }
    }

    // -----------------------------------------------------------------------
    // Fields
    // -----------------------------------------------------------------------

    /** Backing array storing heap entries. Index 0 is the root (maximum). */
    private HeapEntry[] heap;

    /** Number of valid entries currently in the heap. */
    private int size;

    /** Current allocated capacity of the backing array. */
    private int capacity;

    /** Default starting capacity. */
    private static final int DEFAULT_CAPACITY = 16;

    // -----------------------------------------------------------------------
    // Constructors
    // -----------------------------------------------------------------------

    /**
     * Constructs an empty MaxHeap with the default capacity (16).
     */
    public MaxHeap() {
        this.capacity = DEFAULT_CAPACITY;
        this.heap = new HeapEntry[capacity];
        this.size = 0;
    }

    /**
     * Constructs an empty MaxHeap with the specified initial capacity.
     *
     * @param initialCapacity the desired initial capacity (must be &gt; 0)
     */
    public MaxHeap(int initialCapacity) {
        if (initialCapacity <= 0) {
            throw new IllegalArgumentException("Capacity must be positive: " + initialCapacity);
        }
        this.capacity = initialCapacity;
        this.heap = new HeapEntry[capacity];
        this.size = 0;
    }

    // -----------------------------------------------------------------------
    // Core Operations
    // -----------------------------------------------------------------------

    /**
     * Inserts a new entry into the heap and restores the heap property by bubbling up.
     *
     * <p><b>Time Complexity:</b> O(log n) amortised (O(1) amortised for dynamic resize)
     *
     * @param entry the entry to insert (must not be {@code null})
     */
    public void insert(HeapEntry entry) {
        if (entry == null) {
            throw new IllegalArgumentException("Cannot insert null entry");
        }
        // Grow the array if necessary
        if (size == capacity) {
            resize();
        }
        heap[size] = entry;
        heapifyUp(size);
        size++;
    }

    /**
     * Removes and returns the entry with the maximum similarity score (the root).
     *
     * <p>The last element is moved to the root and then sifted down to restore the
     * heap property.
     *
     * <p><b>Time Complexity:</b> O(log n)
     *
     * @return the maximum {@link HeapEntry}, or {@code null} if the heap is empty
     */
    public HeapEntry extractMax() {
        if (size == 0) {
            return null;
        }
        HeapEntry max = heap[0];
        size--;
        heap[0] = heap[size];  // Move last element to root
        heap[size] = null;     // Help GC
        if (size > 0) {
            heapifyDown(0);
        }
        return max;
    }

    /**
     * Returns the entry with the maximum similarity score without removing it.
     *
     * <p><b>Time Complexity:</b> O(1)
     *
     * @return the maximum {@link HeapEntry}, or {@code null} if the heap is empty
     */
    public HeapEntry peek() {
        return (size == 0) ? null : heap[0];
    }

    /**
     * Builds a max-heap from an existing list of entries using Floyd's bottom-up algorithm,
     * which is more efficient than inserting elements one by one.
     *
     * <p>Floyd's algorithm starts from the last internal node (index {@code size/2 - 1})
     * and calls {@code heapifyDown} on each node up to the root. This achieves O(n) total
     * work because most nodes are near the leaves and require few swaps.
     *
     * <p><b>Time Complexity:</b> O(n) (vs O(n log n) for n successive inserts)
     *
     * @param entries the list of entries to heapify (must not be {@code null})
     */
    public void buildHeap(List<HeapEntry> entries) {
        if (entries == null) {
            throw new IllegalArgumentException("Entries list must not be null");
        }
        size = entries.size();
        capacity = Math.max(size, DEFAULT_CAPACITY);
        heap = new HeapEntry[capacity];

        // Copy entries into the backing array
        for (int i = 0; i < size; i++) {
            heap[i] = entries.get(i);
        }

        // Floyd's bottom-up heapify: process all internal nodes from last to first
        for (int i = size / 2 - 1; i >= 0; i--) {
            heapifyDown(i);
        }
    }

    /**
     * Extracts and returns the top-K entries sorted by similarity score (highest first).
     *
     * <p>The entries are destructively removed from the heap. If the heap has fewer than
     * {@code k} entries, all entries are returned.
     *
     * <p><b>Time Complexity:</b> O(k log n)
     *
     * @param k the number of top entries to retrieve
     * @return a list of up to {@code k} entries in descending order of similarity
     */
    public List<HeapEntry> getTopK(int k) {
        if (k <= 0) {
            return new ArrayList<>();
        }
        List<HeapEntry> result = new ArrayList<>(Math.min(k, size));
        int extractCount = Math.min(k, size);
        for (int i = 0; i < extractCount; i++) {
            result.add(extractMax());
        }
        return result;
    }

    // -----------------------------------------------------------------------
    // Heap Maintenance (Private)
    // -----------------------------------------------------------------------

    /**
     * Restores the max-heap property upward from the given index by repeatedly
     * swapping the element with its parent while it is greater than its parent.
     *
     * <p><b>Time Complexity:</b> O(log n) – at most tree-height swaps
     *
     * @param index the starting index (typically the newly inserted element)
     */
    private void heapifyUp(int index) {
        while (index > 0) {
            int parentIndex = (index - 1) / 2;
            if (heap[index].similarityScore > heap[parentIndex].similarityScore) {
                swap(index, parentIndex);
                index = parentIndex;
            } else {
                break; // Heap property restored
            }
        }
    }

    /**
     * Restores the max-heap property downward from the given index by repeatedly
     * swapping the element with its largest child while it is smaller than that child.
     *
     * <p><b>Time Complexity:</b> O(log n) – at most tree-height swaps
     *
     * @param index the starting index (typically the root after extractMax)
     */
    private void heapifyDown(int index) {
        while (true) {
            int leftChild  = 2 * index + 1;
            int rightChild = 2 * index + 2;
            int largest    = index;

            if (leftChild < size && heap[leftChild].similarityScore > heap[largest].similarityScore) {
                largest = leftChild;
            }
            if (rightChild < size && heap[rightChild].similarityScore > heap[largest].similarityScore) {
                largest = rightChild;
            }

            if (largest == index) {
                break; // Heap property satisfied
            }

            swap(index, largest);
            index = largest;
        }
    }

    /**
     * Swaps two elements in the backing array.
     *
     * @param i index of first element
     * @param j index of second element
     */
    private void swap(int i, int j) {
        HeapEntry tmp = heap[i];
        heap[i] = heap[j];
        heap[j] = tmp;
    }

    /**
     * Doubles the backing array capacity when the heap is full.
     *
     * <p><b>Time Complexity:</b> O(n) – array copy
     */
    private void resize() {
        capacity *= 2;
        HeapEntry[] newHeap = new HeapEntry[capacity];
        System.arraycopy(heap, 0, newHeap, 0, size);
        heap = newHeap;
    }

    // -----------------------------------------------------------------------
    // Query Methods
    // -----------------------------------------------------------------------

    /**
     * Returns {@code true} if the heap contains no entries.
     *
     * @return {@code true} if empty
     */
    public boolean isEmpty() {
        return size == 0;
    }

    /**
     * Returns the number of entries currently in the heap.
     *
     * @return the heap size
     */
    public int size() {
        return size;
    }

    /**
     * Removes all entries from the heap. The capacity is retained.
     */
    public void clear() {
        for (int i = 0; i < size; i++) {
            heap[i] = null;
        }
        size = 0;
    }

    @Override
    public String toString() {
        if (size == 0) return "MaxHeap{empty}";
        return String.format("MaxHeap{size=%d, max=%s}", size, heap[0]);
    }
}
