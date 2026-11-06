#pragma once

#include "dsa/CustomHashTable.hpp"
#include "model/Document.hpp"
#include <string>
#include <vector>

namespace plagiguard::detector {

struct DuplicateOccurrence {
    int documentId;
    std::string documentName;
    size_t startToken;
    size_t endToken;
    size_t paragraphIndex;
};

struct DuplicateGroup {
    std::string text;
    size_t occurrenceCount;
    std::vector<DuplicateOccurrence> occurrences;
};

/**
 * @brief DuplicateDetector - Detects internally repeated sentences or paragraphs,
 * and duplicate blocks across multiple documents.
 */
class DuplicateDetector {
public:
    static std::vector<DuplicateGroup> findInternalDuplicates(const model::Document& doc) {
        std::vector<DuplicateGroup> duplicates;
        if (doc.paragraphs.empty()) return duplicates;

        dsa::CustomHashTable<std::string, std::vector<size_t>> paragraphMap(doc.paragraphs.size() * 2 + 16);

        for (size_t i = 0; i < doc.paragraphs.size(); ++i) {
            std::string p = doc.paragraphs[i];
            if (p.size() < 20) continue; // Skip very short paragraphs
            std::vector<size_t>* indices = paragraphMap.find(p);
            if (indices != nullptr) {
                indices->push_back(i);
            } else {
                std::vector<size_t> list = {i};
                paragraphMap.put(p, list);
            }
        }

        for (const auto& key : paragraphMap.keys()) {
            std::vector<size_t> indices;
            paragraphMap.get(key, indices);
            if (indices.size() > 1) {
                DuplicateGroup group;
                group.text = key;
                group.occurrenceCount = indices.size();
                for (size_t idx : indices) {
                    group.occurrences.push_back({doc.id, doc.name, 0, 0, idx});
                }
                duplicates.push_back(group);
            }
        }

        return duplicates;
    }
};

} // namespace plagiguard::detector

