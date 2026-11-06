#pragma once

#include <string>
#include <vector>
#include <chrono>

namespace plagiguard::model {

/**
 * @brief Represents an academic document loaded into PlagiGuard.
 * Preserves original text for display and normalized text for accurate comparison.
 */
struct Document {
    int id;
    std::string name;
    std::string filePath;
    std::string fileType; // TXT, PDF, DOCX
    size_t fileSize;
    std::string originalContent;
    std::string normalizedContent;
    std::vector<std::string> tokens;
    std::vector<std::pair<size_t, size_t>> tokenCharOffsets;
    std::vector<int> tokenParagraphs;
    std::vector<std::string> paragraphs;
    std::string fingerprint;
    size_t wordCount;
    std::string uploadDate;

    Document()
        : id(0), fileSize(0), wordCount(0) {}

    Document(int docId, const std::string& docName, const std::string& path,
             const std::string& type, size_t size)
        : id(docId), name(docName), filePath(path), fileType(type),
          fileSize(size), wordCount(0) {}
};

} // namespace plagiguard::model

