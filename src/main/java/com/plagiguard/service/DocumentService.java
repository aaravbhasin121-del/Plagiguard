package com.plagiguard.service;

import com.plagiguard.model.Document;
import com.plagiguard.processing.TextExtractor;
import com.plagiguard.processing.TextPreprocessor;
import com.plagiguard.processing.DocumentFingerprint;
import com.plagiguard.database.DatabaseManager;
import com.plagiguard.database.DocumentRepository;
import com.plagiguard.dsa.InvertedIndex;
import com.plagiguard.dsa.Trie;
import com.plagiguard.util.FileValidator;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.sql.SQLException;
import java.time.LocalDateTime;
import java.util.*;

/**
 * Service layer for document management.
 * Coordinates extraction, preprocessing, fingerprinting, and persistence.
 *
 * Document processing pipeline:
 * 1. Validate file
 * 2. Extract text
 * 3. Preprocess and normalize
 * 4. Tokenize
 * 5. Generate fingerprint
 * 6. Check for duplicates
 * 7. Persist to database
 * 8. Update inverted index
 */
public class DocumentService {
    private static final Logger logger = LoggerFactory.getLogger(DocumentService.class);

    private final TextExtractor extractor;
    private final TextPreprocessor preprocessor;
    private final DocumentFingerprint fingerprinter;
    private final FileValidator validator;
    private final DocumentRepository docRepo;
    private final InvertedIndex invertedIndex; // in-memory index
    private final Trie vocabulary;            // vocabulary trie

    public DocumentService(DatabaseManager dbManager) {
        this.extractor     = new TextExtractor();
        this.preprocessor  = new TextPreprocessor();
        this.fingerprinter = new DocumentFingerprint();
        this.validator     = new FileValidator();
        this.docRepo       = new DocumentRepository(dbManager);
        this.invertedIndex = new InvertedIndex();
        this.vocabulary    = new Trie();
    }

    /**
     * Process and add a document to the system.
     *
     * @return ProcessResult with document or error info
     */
    public ProcessResult processDocument(File file) {
        try {
            // 1. Validate file
            FileValidator.ValidationResult validation = validator.validate(file);
            if (!validation.isValid()) {
                logger.warn("File validation failed for {}: {}", file.getName(), validation.getMessage());
                return ProcessResult.error(validation.getMessage());
            }

            // 2. Extract text
            String originalContent = extractor.extract(file);
            if (originalContent == null || originalContent.isBlank()) {
                return ProcessResult.error("Could not extract text from file — it may be empty or corrupt.");
            }

            // 3. Preprocess and normalize
            String normalizedContent = preprocessor.normalize(originalContent);

            // 4. Tokenize
            String[] tokens = preprocessor.tokenize(normalizedContent);

            // 5. Generate fingerprint
            String fingerprint = fingerprinter.generate(normalizedContent);

            // 6. Check for duplicates
            Optional<Document> existing = docRepo.findByFingerprint(fingerprint);
            if (existing.isPresent()) {
                logger.info("Duplicate document detected: {} matches existing document id={}",
                        file.getName(), existing.get().getId());
                return ProcessResult.duplicate(existing.get());
            }

            // 7. Build Document object
            Document doc = new Document();
            doc.setName(file.getName());
            doc.setFilePath(file.getAbsolutePath());
            doc.setFileType(getExtension(file.getName()).toUpperCase());
            doc.setFileSize(file.length());
            doc.setOriginalContent(originalContent);
            doc.setNormalizedContent(normalizedContent);
            doc.setTokens(tokens);
            doc.setFingerprint(fingerprint);
            doc.setWordCount(tokens.length);
            doc.setUploadDate(LocalDateTime.now());
            doc.setParagraphBoundaries(preprocessor.detectParagraphBoundaries(originalContent, tokens));

            // 8. Persist to database
            int id = docRepo.save(doc);
            doc.setId(id);
            logger.info("Document saved: id={}, name={}, words={}", id, doc.getName(), doc.getWordCount());

            // 9. Update in-memory index
            indexDocument(doc);

            return ProcessResult.success(doc);

        } catch (Exception e) {
            logger.error("Error processing document: {}", file.getName(), e);
            return ProcessResult.error("Processing error: " + e.getMessage());
        }
    }

    /** Index the document's tokens into the inverted index and vocabulary trie. */
    private void indexDocument(Document doc) {
        String[] tokens = doc.getTokens();
        for (int i = 0; i < tokens.length; i++) {
            String token = tokens[i];
            if (token != null && !token.isBlank()) {
                invertedIndex.add(token, doc.getId(), i);
                vocabulary.insert(token);
            }
        }
    }

    /**
     * Get all documents from database.
     */
    public List<Document> getAllDocuments() throws SQLException {
        return docRepo.findAll();
    }

    /**
     * Delete a document by ID.
     */
    public boolean deleteDocument(int id) throws SQLException {
        boolean deleted = docRepo.delete(id);
        if (deleted) {
            // Rebuild index after deletion to keep it consistent
            try {
                rebuildIndex();
            } catch (SQLException e) {
                logger.warn("Could not rebuild index after deleting document id={}", id, e);
            }
        }
        return deleted;
    }

    /**
     * Find document by ID.
     */
    public Optional<Document> getDocumentById(int id) throws SQLException {
        return docRepo.findById(id);
    }

    /**
     * Get total document count.
     */
    public int getDocumentCount() throws SQLException {
        return docRepo.count();
    }

    /**
     * Search documents by name (partial match).
     */
    public List<Document> searchDocuments(String query) throws SQLException {
        if (query == null || query.isBlank()) {
            return docRepo.findAll();
        }
        return docRepo.findByName(query.trim());
    }

    /**
     * Rebuild in-memory index from all database documents.
     * Called on application startup.
     */
    public void rebuildIndex() throws SQLException {
        invertedIndex.clear();
        vocabulary.clear();
        List<Document> all = docRepo.findAll();
        for (Document doc : all) {
            indexDocument(doc);
        }
        logger.info("In-memory index rebuilt with {} documents.", all.size());
    }

    /**
     * Get the in-memory inverted index.
     */
    public InvertedIndex getInvertedIndex() {
        return invertedIndex;
    }

    /**
     * Get the vocabulary trie.
     */
    public Trie getVocabularyTrie() {
        return vocabulary;
    }

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    private String getExtension(String filename) {
        int dotIndex = filename.lastIndexOf('.');
        if (dotIndex >= 0 && dotIndex < filename.length() - 1) {
            return filename.substring(dotIndex + 1);
        }
        return "TXT";
    }

    // -------------------------------------------------------------------------
    // Inner result type
    // -------------------------------------------------------------------------

    public static class ProcessResult {
        public final Document document;
        public final boolean success;
        public final String message;
        public final boolean isDuplicate;
        public final Document duplicateOf; // if duplicate, the existing document

        public ProcessResult(Document document, boolean success, String message,
                             boolean isDuplicate, Document duplicateOf) {
            this.document    = document;
            this.success     = success;
            this.message     = message;
            this.isDuplicate = isDuplicate;
            this.duplicateOf = duplicateOf;
        }

        public static ProcessResult success(Document doc) {
            return new ProcessResult(doc, true, "Document processed successfully.", false, null);
        }

        public static ProcessResult duplicate(Document existing) {
            return new ProcessResult(existing, false,
                    "A document with identical content already exists: " + existing.getName(),
                    true, existing);
        }

        public static ProcessResult error(String message) {
            return new ProcessResult(null, false, message, false, null);
        }
    }
}
