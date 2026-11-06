#pragma once

#include "database/DocumentRepository.hpp"
#include "dsa/InvertedIndex.hpp"
#include "dsa/Trie.hpp"
#include "dsa/NGramGenerator.hpp"
#include "processing/TextExtractor.hpp"
#include "processing/TextPreprocessor.hpp"
#include "processing/DocumentFingerprint.hpp"
#include "model/Document.hpp"
#include <iostream>
#include <filesystem>

namespace plagiguard::service {

struct ProcessResult {
    bool success;
    bool isDuplicate;
    model::Document document;
    model::Document duplicateOf;
    std::string message;
};

/**
 * @brief DocumentService - Manages ingestion, indexing, and querying of documents.
 */
class DocumentService {
private:
    database::DocumentRepository docRepo;
    dsa::InvertedIndex invertedIndex;
    dsa::Trie vocabularyTrie;

public:
    DocumentService() {
        rebuildIndex();
    }

    void rebuildIndex() {
        invertedIndex.clear();
        vocabularyTrie.clear();
        std::vector<model::Document> allDocs = docRepo.findAll();

        dsa::NGramGenerator gen(5);
        for (auto& doc : allDocs) {
            auto detailedTokens = processing::TextPreprocessor::tokenizeDetailed(doc.originalContent);
            std::vector<std::string> tokens;
            tokens.reserve(detailedTokens.size());
            for (const auto& t : detailedTokens) {
                tokens.push_back(t.text);
                vocabularyTrie.insert(t.text);
            }
            std::vector<std::string> shingles = gen.generateShingles(tokens);
            invertedIndex.indexDocument(doc.id, doc.name, shingles);
        }
    }

    ProcessResult processDocument(const std::string& filePath) {
        ProcessResult res;
        res.success = false;
        res.isDuplicate = false;

        // 1. Text Extraction
        auto extResult = processing::TextExtractor::extract(filePath);
        if (!extResult.success) {
            res.message = extResult.errorMessage;
            return res;
        }

        std::filesystem::path p(filePath);
        std::string fileName = p.filename().string();

        // 2. Preprocessing & Normalization
        std::string normalized = processing::TextPreprocessor::normalize(extResult.text);
        auto tokenDetails = processing::TextPreprocessor::tokenizeDetailed(extResult.text);
        auto paragraphs = processing::TextPreprocessor::extractParagraphs(extResult.text);

        // 3. Document Fingerprinting
        std::string fingerprint = processing::DocumentFingerprint::generateFingerprint(normalized);

        // 4. Duplicate Check
        auto existing = docRepo.findByFingerprint(fingerprint);
        if (existing.has_value()) {
            res.isDuplicate = true;
            res.duplicateOf = existing.value();
            res.message = "Document is identical to previously uploaded: " + existing.value().name;
            return res;
        }

        // 5. Construct Document Model
        model::Document doc;
        doc.name = fileName;
        doc.filePath = filePath;
        doc.fileType = extResult.fileType;
        doc.fileSize = extResult.fileSize;
        doc.originalContent = extResult.text;
        doc.normalizedContent = normalized;
        doc.fingerprint = fingerprint;
        doc.paragraphs = paragraphs;

        doc.tokens.reserve(tokenDetails.size());
        doc.tokenCharOffsets.reserve(tokenDetails.size());
        doc.tokenParagraphs.reserve(tokenDetails.size());

        for (const auto& t : tokenDetails) {
            doc.tokens.push_back(t.text);
            doc.tokenCharOffsets.push_back({t.charStart, t.charEnd});
            doc.tokenParagraphs.push_back(t.paragraphIndex);
            vocabularyTrie.insert(t.text);
        }
        doc.wordCount = doc.tokens.size();

        // 6. Save to Database
        try {
            docRepo.save(doc);
        } catch (const std::exception& e) {
            res.message = "Database error: " + std::string(e.what());
            return res;
        }

        // 7. Update Inverted Index
        dsa::NGramGenerator gen(5);
        std::vector<std::string> shingles = gen.generateShingles(doc.tokens);
        invertedIndex.indexDocument(doc.id, doc.name, shingles);

        res.success = true;
        res.document = doc;
        res.message = "Successfully processed and indexed document.";
        return res;
    }

    std::vector<model::Document> getAllDocuments() {
        return docRepo.findAll();
    }

    std::optional<model::Document> getDocumentById(int id) {
        return docRepo.findById(id);
    }

    bool deleteDocument(int id) {
        bool ok = docRepo.deleteById(id);
        if (ok) rebuildIndex();
        return ok;
    }

    dsa::InvertedIndex& getInvertedIndex() { return invertedIndex; }
    dsa::Trie& getVocabularyTrie() { return vocabularyTrie; }
};

} // namespace plagiguard::service

