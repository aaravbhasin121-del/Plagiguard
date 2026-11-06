#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>

namespace plagiguard::dsa {

/**
 * @brief Represents a scored comparison between two documents in the MaxHeap.
 */
struct HeapEntry {
    int docAId;
    std::string docAName;
    int docBId;
    std::string docBName;
    double similarityScore;

    HeapEntry()
        : docAId(0), docBId(0), similarityScore(0.0) {}

    HeapEntry(int aId, const std::string& aName, int bId, const std::string& bName, double score)
        : docAId(aId), docAName(aName), docBId(bId), docBName(bName), similarityScore(score) {}

    bool operator<(const HeapEntry& other) const {
        return similarityScore < other.similarityScore;
    }

    bool operator>(const HeapEntry& other) const {
        return similarityScore > other.similarityScore;
    }

    bool operator<=(const HeapEntry& other) const {
        return similarityScore <= other.similarityScore;
    }

    bool operator>=(const HeapEntry& other) const {
        return similarityScore >= other.similarityScore;
    }
};

/**
 * @brief MaxHeap - A custom binary maximum heap implementation for ranking document similarity.
 *
 * Requirements:
 * - Dynamic array storage
 * - Insert: O(log N)
 * - Extract Maximum: O(log N)
 * - Peek: O(1)
 * - Heapify Up / Heapify Down operations
 * - Build Heap: O(N)
 * - Get Top-K largest entries
 * - Handles equal scores stably
 */
class MaxHeap {
private:
    HeapEntry* tree;
    size_t heapSize;
    size_t capacity;

    void heapifyUp(size_t index) {
        while (index > 0) {
            size_t parent = (index - 1) / 2;
            if (tree[index].similarityScore > tree[parent].similarityScore) {
                std::swap(tree[index], tree[parent]);
                index = parent;
            } else {
                break;
            }
        }
    }

    void heapifyDown(size_t index) {
        while (true) {
            size_t left = 2 * index + 1;
            size_t right = 2 * index + 2;
            size_t largest = index;

            if (left < heapSize && tree[left].similarityScore > tree[largest].similarityScore) {
                largest = left;
            }
            if (right < heapSize && tree[right].similarityScore > tree[largest].similarityScore) {
                largest = right;
            }

            if (largest != index) {
                std::swap(tree[index], tree[largest]);
                index = largest;
            } else {
                break;
            }
        }
    }

    void resize(size_t newCapacity) {
        HeapEntry* newTree = new HeapEntry[newCapacity];
        for (size_t i = 0; i < heapSize; ++i) {
            newTree[i] = tree[i];
        }
        delete[] tree;
        tree = newTree;
        capacity = newCapacity;
    }

public:
    explicit MaxHeap(size_t initialCapacity = 32)
        : heapSize(0), capacity(initialCapacity < 4 ? 4 : initialCapacity) {
        tree = new HeapEntry[capacity];
    }

    ~MaxHeap() {
        delete[] tree;
        tree = nullptr;
    }

    // Copy constructor
    MaxHeap(const MaxHeap& other)
        : heapSize(other.heapSize), capacity(other.capacity) {
        tree = new HeapEntry[capacity];
        for (size_t i = 0; i < heapSize; ++i) {
            tree[i] = other.tree[i];
        }
    }

    // Copy assignment
    MaxHeap& operator=(const MaxHeap& other) {
        if (this != &other) {
            delete[] tree;
            heapSize = other.heapSize;
            capacity = other.capacity;
            tree = new HeapEntry[capacity];
            for (size_t i = 0; i < heapSize; ++i) {
                tree[i] = other.tree[i];
            }
        }
        return *this;
    }

    void insert(const HeapEntry& entry) {
        if (heapSize >= capacity) {
            resize(capacity * 2);
        }
        tree[heapSize] = entry;
        heapifyUp(heapSize);
        heapSize++;
    }

    HeapEntry peek() const {
        if (empty()) {
            throw std::runtime_error("Heap is empty, cannot peek");
        }
        return tree[0];
    }

    HeapEntry extractMax() {
        if (empty()) {
            throw std::runtime_error("Heap is empty, cannot extract max");
        }
        HeapEntry maxVal = tree[0];
        tree[0] = tree[heapSize - 1];
        heapSize--;
        if (heapSize > 0) {
            heapifyDown(0);
        }
        return maxVal;
    }

    void buildHeap(const std::vector<HeapEntry>& entries) {
        delete[] tree;
        capacity = std::max(entries.size() * 2, size_t(16));
        tree = new HeapEntry[capacity];
        heapSize = entries.size();

        for (size_t i = 0; i < heapSize; ++i) {
            tree[i] = entries[i];
        }

        // Bottom-up heapify in O(N)
        if (heapSize > 1) {
            for (int i = static_cast<int>((heapSize - 2) / 2); i >= 0; --i) {
                heapifyDown(static_cast<size_t>(i));
            }
        }
    }

    std::vector<HeapEntry> getTopK(size_t k) {
        MaxHeap copyHeap = *this; // work on a copy to preserve original heap
        std::vector<HeapEntry> topK;
        size_t count = std::min(k, copyHeap.size());
        topK.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            topK.push_back(copyHeap.extractMax());
        }
        return topK;
    }

    size_t size() const {
        return heapSize;
    }

    bool empty() const {
        return heapSize == 0;
    }

    void clear() {
        heapSize = 0;
    }
};

} // namespace plagiguard::dsa

