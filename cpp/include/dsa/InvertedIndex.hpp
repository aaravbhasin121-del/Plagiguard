#pragma once

#include "CustomHashTable.hpp"
#include <string>
#include <vector>

namespace plagiguard::dsa {

/**
 * @brief PostingEntry - Represents an occurrence of a term in a document.
 */
struct PostingEntry {
    int documentId;
    std::string documentName;
    std::vector<size_t> positions; // token or shingle positions

    PostingEntry() : documentId(-1) {}
    PostingEntry(int docId, const std::string& docName, size_t pos)
        : documentId(docId), documentName(docName), positions({pos}) {}
};

/**
 * @brief InvertedIndex - Maps terms or shingles to documents and positions.
 *
 * Used to retrieve potentially matching candidate documents without
 * comparing every document against every other document unnecessarily.
 *
 * Space Complexity: O(T * P) where T is terms, P is postings
 * Lookup Complexity: O(1) average lookup per query term
 */
class InvertedIndex {
private:
    CustomHashTable<std::string, std::vector<PostingEntry>> index;
    CustomHashTable<int, std::string> documentCatalog;

public:
    InvertedIndex() : index(1024) {}

    void addTerm(const std::string& term, int documentId, const std::string& documentName, size_t position) {
        documentCatalog.put(documentId, documentName);
        std::vector<PostingEntry>* postings = index.find(term);
        if (postings == nullptr) {
            std::vector<PostingEntry> list;
            list.emplace_back(documentId, documentName, position);
            index.put(term, list);
        } else {
            // Check if document already exists in postings
            bool found = false;
            for (auto& entry : *postings) {
                if (entry.documentId == documentId) {
                    entry.positions.push_back(position);
                    found = true;
                    break;
                }
            }
            if (!found) {
                postings->emplace_back(documentId, documentName, position);
            }
        }
    }

    void indexDocument(int docId, const std::string& docName, const std::vector<std::string>& shingles) {
        for (size_t i = 0; i < shingles.size(); ++i) {
            addTerm(shingles[i], docId, docName, i);
        }
    }

    const std::vector<PostingEntry>* lookup(const std::string& term) const {
        return index.find(term);
    }

    /**
     * @brief Retrieves all document IDs that share at least one shingle with the query document.
     * Candidate pruning prevents all-pairs O(N^2) quadratic explosion.
     */
    std::vector<int> findCandidateDocuments(const std::vector<std::string>& shingles, int excludeDocId = -1) const {
        CustomHashTable<int, size_t> candidateHits(64);
        for (const auto& shingle : shingles) {
            const std::vector<PostingEntry>* postings = index.find(shingle);
            if (postings != nullptr) {
                for (const auto& p : *postings) {
                    if (p.documentId == excludeDocId) continue;
                    size_t count = 0;
                    candidateHits.get(p.documentId, count);
                    candidateHits.put(p.documentId, count + 1);
                }
            }
        }
        return candidateHits.keys();
    }

    size_t uniqueTermsCount() const {
        return index.size();
    }

    size_t documentCount() const {
        return documentCatalog.size();
    }

    void clear() {
        index.clear();
        documentCatalog.clear();
    }
};

} // namespace plagiguard::dsa

