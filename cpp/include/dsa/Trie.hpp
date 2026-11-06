#pragma once

#include "CustomHashTable.hpp"
#include <string>
#include <vector>

namespace plagiguard::dsa {

/**
 * @brief Trie (Prefix Tree) implementation for vocabulary indexing and fast prefix queries.
 *
 * Requirements:
 * - Insert: O(L) where L is string length
 * - Search: O(L)
 * - Prefix Search (startsWith): O(L)
 * - Word Frequency tracking
 * - Subtree collection for autocomplete / prefix retrieval
 * - Manual memory deallocation (no leaks)
 */
class Trie {
private:
    struct TrieNode {
        bool isTerminal;
        size_t frequency;
        CustomHashTable<char, TrieNode*> children;

        TrieNode() : isTerminal(false), frequency(0), children(16) {}

        ~TrieNode() {
            for (char c : children.keys()) {
                TrieNode* child = nullptr;
                if (children.get(c, child) && child != nullptr) {
                    delete child;
                }
            }
        }
    };

    TrieNode* root;
    size_t totalWords;

    void collectWords(TrieNode* node, const std::string& currentPrefix, std::vector<std::string>& results) const {
        if (node == nullptr) return;
        if (node->isTerminal) {
            results.push_back(currentPrefix);
        }
        std::vector<char> childKeys = node->children.keys();
        for (char c : childKeys) {
            TrieNode* nextNode = nullptr;
            if (node->children.get(c, nextNode) && nextNode != nullptr) {
                collectWords(nextNode, currentPrefix + c, results);
            }
        }
    }

public:
    Trie() : root(new TrieNode()), totalWords(0) {}

    ~Trie() {
        delete root;
        root = nullptr;
    }

    // Disable copy for simplicity or deep copy
    Trie(const Trie&) = delete;
    Trie& operator=(const Trie&) = delete;

    void insert(const std::string& word) {
        if (word.empty()) return;
        TrieNode* curr = root;

        for (char c : word) {
            TrieNode* next = nullptr;
            if (!curr->children.get(c, next) || next == nullptr) {
                next = new TrieNode();
                curr->children.put(c, next);
            }
            curr = next;
        }

        if (!curr->isTerminal) {
            curr->isTerminal = true;
            totalWords++;
        }
        curr->frequency++;
    }

    bool search(const std::string& word) const {
        if (word.empty()) return false;
        TrieNode* curr = root;

        for (char c : word) {
            TrieNode* next = nullptr;
            if (!curr->children.get(c, next) || next == nullptr) {
                return false;
            }
            curr = next;
        }
        return curr != nullptr && curr->isTerminal;
    }

    bool startsWith(const std::string& prefix) const {
        if (prefix.empty()) return true;
        TrieNode* curr = root;

        for (char c : prefix) {
            TrieNode* next = nullptr;
            if (!curr->children.get(c, next) || next == nullptr) {
                return false;
            }
            curr = next;
        }
        return curr != nullptr;
    }

    size_t getFrequency(const std::string& word) const {
        if (word.empty()) return 0;
        TrieNode* curr = root;

        for (char c : word) {
            TrieNode* next = nullptr;
            if (!curr->children.get(c, next) || next == nullptr) {
                return 0;
            }
            curr = next;
        }
        return (curr != nullptr && curr->isTerminal) ? curr->frequency : 0;
    }

    std::vector<std::string> getWordsWithPrefix(const std::string& prefix) const {
        std::vector<std::string> results;
        TrieNode* curr = root;

        for (char c : prefix) {
            TrieNode* next = nullptr;
            if (!curr->children.get(c, next) || next == nullptr) {
                return results;
            }
            curr = next;
        }

        collectWords(curr, prefix, results);
        return results;
    }

    size_t size() const {
        return totalWords;
    }

    void clear() {
        delete root;
        root = new TrieNode();
        totalWords = 0;
    }
};

} // namespace plagiguard::dsa

