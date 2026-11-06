#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <stdexcept>
#include <sstream>

namespace plagiguard::dsa {

/**
 * @brief CustomHashTable - A generic hash table implementation using separate chaining
 * for collision resolution.
 *
 * This implementation manually manages:
 * - A dynamic array of bucket head pointers
 * - Singly-linked list chaining for collisions
 * - Dynamic capacity doubling and rehashing when load factor exceeds 0.75
 * - Deep copying and memory deallocation without memory leaks
 *
 * Time Complexity:
 * - put:        O(1) average, O(N) worst case (extreme hash collision)
 * - get:        O(1) average, O(N) worst case
 * - remove:     O(1) average, O(N) worst case
 * - resize:     O(N + M) where N = entries, M = bucket capacity
 *
 * Space Complexity: O(N + M)
 */
template <typename K, typename V>
class CustomHashTable {
private:
    struct HashNode {
        K key;
        V value;
        HashNode* next;

        HashNode(const K& k, const V& v, HashNode* n = nullptr)
            : key(k), value(v), next(n) {}
    };

    HashNode** buckets;
    size_t bucketCapacity;
    size_t elementCount;
    double maxLoadFactor;

    size_t hashKey(const K& key) const {
        return std::hash<K>{}(key) % bucketCapacity;
    }

    void rehash(size_t newCapacity) {
        HashNode** oldBuckets = buckets;
        size_t oldCapacity = bucketCapacity;

        bucketCapacity = newCapacity;
        buckets = new HashNode*[bucketCapacity]();
        elementCount = 0;

        for (size_t i = 0; i < oldCapacity; ++i) {
            HashNode* curr = oldBuckets[i];
            while (curr != nullptr) {
                HashNode* next = curr->next;
                put(curr->key, curr->value);
                delete curr;
                curr = next;
            }
        }
        delete[] oldBuckets;
    }

    void clearInternal() {
        if (buckets != nullptr) {
            for (size_t i = 0; i < bucketCapacity; ++i) {
                HashNode* curr = buckets[i];
                while (curr != nullptr) {
                    HashNode* next = curr->next;
                    delete curr;
                    curr = next;
                }
                buckets[i] = nullptr;
            }
        }
        elementCount = 0;
    }

public:
    explicit CustomHashTable(size_t initialCapacity = 16, double loadFactor = 0.75)
        : bucketCapacity(initialCapacity < 8 ? 8 : initialCapacity),
          elementCount(0),
          maxLoadFactor(loadFactor) {
        buckets = new HashNode*[bucketCapacity]();
    }

    ~CustomHashTable() {
        clearInternal();
        delete[] buckets;
        buckets = nullptr;
    }

    // Copy Constructor (Deep copy)
    CustomHashTable(const CustomHashTable& other)
        : bucketCapacity(other.bucketCapacity),
          elementCount(0),
          maxLoadFactor(other.maxLoadFactor) {
        buckets = new HashNode*[bucketCapacity]();
        for (size_t i = 0; i < other.bucketCapacity; ++i) {
            HashNode* curr = other.buckets[i];
            while (curr != nullptr) {
                put(curr->key, curr->value);
                curr = curr->next;
            }
        }
    }

    // Copy Assignment Operator
    CustomHashTable& operator=(const CustomHashTable& other) {
        if (this != &other) {
            clearInternal();
            delete[] buckets;

            bucketCapacity = other.bucketCapacity;
            elementCount = 0;
            maxLoadFactor = other.maxLoadFactor;
            buckets = new HashNode*[bucketCapacity]();

            for (size_t i = 0; i < other.bucketCapacity; ++i) {
                HashNode* curr = other.buckets[i];
                while (curr != nullptr) {
                    put(curr->key, curr->value);
                    curr = curr->next;
                }
            }
        }
        return *this;
    }

    // Move Constructor
    CustomHashTable(CustomHashTable&& other) noexcept
        : buckets(other.buckets),
          bucketCapacity(other.bucketCapacity),
          elementCount(other.elementCount),
          maxLoadFactor(other.maxLoadFactor) {
        other.buckets = nullptr;
        other.bucketCapacity = 0;
        other.elementCount = 0;
    }

    // Move Assignment
    CustomHashTable& operator=(CustomHashTable&& other) noexcept {
        if (this != &other) {
            clearInternal();
            delete[] buckets;

            buckets = other.buckets;
            bucketCapacity = other.bucketCapacity;
            elementCount = other.elementCount;
            maxLoadFactor = other.maxLoadFactor;

            other.buckets = nullptr;
            other.bucketCapacity = 0;
            other.elementCount = 0;
        }
        return *this;
    }

    void put(const K& key, const V& value) {
        if (getLoadFactor() >= maxLoadFactor) {
            rehash(bucketCapacity * 2);
        }

        size_t idx = hashKey(key);
        HashNode* curr = buckets[idx];

        while (curr != nullptr) {
            if (curr->key == key) {
                curr->value = value; // Update existing
                return;
            }
            curr = curr->next;
        }

        // Insert at bucket head
        HashNode* newNode = new HashNode(key, value, buckets[idx]);
        buckets[idx] = newNode;
        elementCount++;
    }

    bool get(const K& key, V& outValue) const {
        if (bucketCapacity == 0 || buckets == nullptr) return false;
        size_t idx = hashKey(key);
        HashNode* curr = buckets[idx];

        while (curr != nullptr) {
            if (curr->key == key) {
                outValue = curr->value;
                return true;
            }
            curr = curr->next;
        }
        return false;
    }

    V* find(const K& key) {
        if (bucketCapacity == 0 || buckets == nullptr) return nullptr;
        size_t idx = hashKey(key);
        HashNode* curr = buckets[idx];

        while (curr != nullptr) {
            if (curr->key == key) {
                return &(curr->value);
            }
            curr = curr->next;
        }
        return nullptr;
    }

    const V* find(const K& key) const {
        if (bucketCapacity == 0 || buckets == nullptr) return nullptr;
        size_t idx = hashKey(key);
        HashNode* curr = buckets[idx];

        while (curr != nullptr) {
            if (curr->key == key) {
                return &(curr->value);
            }
            curr = curr->next;
        }
        return nullptr;
    }

    bool contains(const K& key) const {
        V dummy;
        return get(key, dummy);
    }

    bool remove(const K& key) {
        if (bucketCapacity == 0 || buckets == nullptr) return false;
        size_t idx = hashKey(key);
        HashNode* curr = buckets[idx];
        HashNode* prev = nullptr;

        while (curr != nullptr) {
            if (curr->key == key) {
                if (prev == nullptr) {
                    buckets[idx] = curr->next;
                } else {
                    prev->next = curr->next;
                }
                delete curr;
                elementCount--;
                return true;
            }
            prev = curr;
            curr = curr->next;
        }
        return false;
    }

    size_t size() const {
        return elementCount;
    }

    bool empty() const {
        return elementCount == 0;
    }

    size_t capacity() const {
        return bucketCapacity;
    }

    double getLoadFactor() const {
        if (bucketCapacity == 0) return 0.0;
        return static_cast<double>(elementCount) / static_cast<double>(bucketCapacity);
    }

    void clear() {
        clearInternal();
    }

    std::vector<K> keys() const {
        std::vector<K> result;
        result.reserve(elementCount);
        if (buckets == nullptr) return result;

        for (size_t i = 0; i < bucketCapacity; ++i) {
            HashNode* curr = buckets[i];
            while (curr != nullptr) {
                result.push_back(curr->key);
                curr = curr->next;
            }
        }
        return result;
    }

    std::vector<V> values() const {
        std::vector<V> result;
        result.reserve(elementCount);
        if (buckets == nullptr) return result;

        for (size_t i = 0; i < bucketCapacity; ++i) {
            HashNode* curr = buckets[i];
            while (curr != nullptr) {
                result.push_back(curr->value);
                curr = curr->next;
            }
        }
        return result;
    }

    std::string getStats() const {
        std::ostringstream oss;
        oss << "CustomHashTable [Size: " << elementCount
            << ", Capacity: " << bucketCapacity
            << ", LoadFactor: " << getLoadFactor() << "]";
        return oss.str();
    }
};

} // namespace plagiguard::dsa

