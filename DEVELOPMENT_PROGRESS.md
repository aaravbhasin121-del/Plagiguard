# PlagiGuard — Development Progress & Bug Fix Log

## 1. Project Overview
- **Application Name**: PlagiGuard — Intelligent Academic Plagiarism Detection & Document Integrity Platform
- **Core Technology**: 100% C++20 Core Engine & Embedded REST Server, SQLite3, HTML5/CSS3/JavaScript (ES6+) Canvas Frontend.
- **Academic Context**: Advanced Data Structures & Algorithms University Mini-Project.

---

## 2. Issues Discovered & Resolved

### Bug 1: 0% Similarity and Zero Matching Passages on SQLite Documents
- **Symptom**: When documents were loaded from SQLite and compared, the similarity score returned 0.0% and no matching passages were detected, even for identical or partially copied documents (`document_a.txt` vs `document_d_partial_copy.txt`).
- **Root Cause**: `DocumentRepository::findAll()` populated `originalContent` and `normalizedContent` from the database columns, but did not populate the in-memory `tokens`, `tokenCharOffsets`, and `paragraphs` vectors. When `PlagiarismDetector::compare` invoked `generator.generateShingles(docA.tokens)`, `docA.tokens` was empty, resulting in 0 shingles and 0% similarity.
- **Resolution**: Enhanced `PlagiarismDetector::compare()` to inspect whether `docA.tokens` and `docB.tokens` are empty while `originalContent` is populated. If empty, the detector dynamically triggers `TextPreprocessor::tokenizeDetailed()` and `extractParagraphs()` to populate tokens, byte offsets, and paragraph boundaries before N-gram generation.
- **Verification**: Comparing `document_a.txt` against `document_d_partial_copy.txt` now accurately computes:
  - **Jaccard Similarity**: $82.09\%$
  - **Directional Overlap**: $90.05\%$
  - **Matching Shingles**: $353$
  - **Matching Passages**: $2$ verified passages (322 words and 39 words).

---

### Bug 2: Similarity Network Page Rendered Completely Blank
- **Symptom**: Navigating to the Similarity Network tab showed an empty canvas with no nodes, edges, or traversal animations.
- **Root Cause**: `documentGraph` was an in-memory object in `AnalysisService`. Upon server boot or restart, `documentGraph` had no vertices or edges populated until a batch analysis was executed. Furthermore, `runGraphBFS()` in `app.js` was simply cycling through canvas nodes in a linear loop rather than calling the C++ engine's BFS/DFS algorithms.
- **Resolution**:
  1. Implemented `AnalysisService::syncGraphFromDatabase(threshold)` which queries all indexed documents in SQLite as vertices and all completed comparison records in `SIMILARITY_RESULTS` as weighted edges, followed by computing a 2D force-directed layout.
  2. Added dedicated C++ endpoints `/api/graph?threshold=...`, `/api/graph/bfs?start=...&threshold=...`, and `/api/graph/dfs?start=...&threshold=...`.
  3. Updated `web/app.js` to call these endpoints, rendering real nodes, edges, connected clusters, and animating the exact node sequences produced by C++ queue-based BFS and stack-based DFS.
- **Verification**: Opening the Similarity Network tab displays all documents as interactive nodes connected by weighted similarity edges with working threshold filtering and animated C++ traversals.

---

### Bug 3: Ambiguous Duplicate Warning & Single-Document Restriction
- **Symptom**: Uploading multiple files previously caused earlier items to disappear from the file list, and duplicate detection produced a cryptic warning without explaining why the file was rejected or allowing the user to view the duplicate.
- **Root Cause**: `handleFiles()` in `app.js` did not support parallel Base64 encoding cleanly, and `/api/upload` only returned a simple string for duplicates.
- **Resolution**:
  1. Updated `handleFiles()` with an `async/await` promise reader that preserves uploaded documents in a clean queue.
  2. Enhanced `/api/upload` to return detailed duplicate metadata (`duplicateType: "EXACT_TEXT_DUPLICATE"`, `existingDocId`, `existingDocName`, `fingerprint`, `wordCount`).
  3. Created an interactive **Duplicate Detection Modal** explaining that the uploaded document has an identical 128-bit text fingerprint to an existing file, and providing a 1-click button to select the existing document for analysis.
- **Verification**: Uploading identical files prompts the duplicate modal with full transparency; uploading distinct files ingests both with status `✓ Ingested (#id)`.

---

### Bug 4: Neural OCR & Multi-Format Parsing
- **Symptom**: Previous attempts to ingest `.docx` and scanned `.pdf` files failed because external utilities (`pdftotext`, Python libraries) were missing.
- **Root Cause**: The system lacked native extraction for macOS binary documents.
- **Resolution**:
  1. Created `AppleOcrExtractor.hpp` linking directly against macOS native frameworks:
     - `-framework Vision`: Executes Apple Neural OCR (`VNRecognizeTextRequest`) on scanned PDFs and images at $2\times$ scale with language correction.
     - `-framework PDFKit`: Direct extraction of text-layer PDFs.
     - `/usr/bin/textutil`: Native macOS extraction of `.docx` files with raw XML fallback (`word/document.xml`).
  2. Linked `-framework Foundation -framework PDFKit -framework Vision -framework AppKit -framework CoreGraphics` in Apple `clang++`.
- **Verification**: `CHAPTER 2.1.1.docx` (18 words) and `25BCS10396 Assignment.pdf` (2,380 words) both ingest cleanly with 0 errors.

---

## 3. Test Suites Executed & Passed

The comprehensive C++ automated test runner (`cpp/tests/test_runner.cpp`) validates all 11 core data structures and engine components:

```text
=======================================================
  PLAGIGUARD C++20 COMPREHENSIVE AUTOMATED TEST SUITE
=======================================================

[Test 1] CustomHashTable Operations, Collisions & Resizing... PASSED
[Test 2] RollingHash Polynomial Double Hashing & Rabin-Karp... PASSED
[Test 3] N-gram Generation & Boundary Conditions (Short & Empty)... PASSED
[Test 4] Jaccard & Directional Overlap Mathematical Accuracy... PASSED
[Test 5] Custom MaxHeap Ranking, Heapify & Top-K Extraction... PASSED
[Test 6] Trie (Prefix Search) & Inverted Index Postings... PASSED
[Test 7] DocumentGraph Adjacency List, BFS, DFS & Connected Components... PASSED
[Test 8] Dynamic Programming: Levenshtein Distance & LCS Diff... PASSED
[Test 9] Text Normalization, Positions, Citations & Unicode... PASSED
[Test 10] Complete End-to-End Plagiarism Detection Engine... PASSED
[Test 11] Pure C++ PDF 1.4 Generation & File Verification... PASSED

-------------------------------------------------------
TEST RESULTS: 11 / 11 test suites passed.
-------------------------------------------------------
```

---

## 4. Summary of API Endpoints

| Endpoint | Method | Description |
| :--- | :--- | :--- |
| `/api/status` | `GET` | Returns C++ engine health, uptime, version, and active algorithms list |
| `/api/stats` | `GET` | Returns total documents, analyses, average similarity, and histogram data |
| `/api/documents` | `GET` | Returns all documents stored in SQLite database |
| `/api/document?id=...` | `GET` | Returns complete metadata and content preview for a single document |
| `/api/delete-doc` | `POST` | Deletes a document from SQLite and rebuilds the inverted index |
| `/api/upload` | `POST` | Accepts Base64 data, runs OCR/extraction, checks 128-bit duplicates, and indexes |
| `/api/analyze-two` | `POST` | Runs two-document comparison with configurable N-gram window |
| `/api/analyze-batch` | `POST` | Cross-compares all library documents ($N(N-1)/2$) and ranks with Max-Heap |
| `/api/analysis-detail?id=...` | `GET` | Returns complete analysis record with matching passages and heatmap |
| `/api/graph?threshold=...` | `GET` | Returns graph layout, nodes, edges, and clusters from SQLite |
| `/api/graph/bfs?start=...` | `GET` | Executes C++ queue-based BFS traversal starting at specified node |
| `/api/graph/dfs?start=...` | `GET` | Executes C++ stack-based DFS traversal starting at specified node |
| `/api/version-diff` | `POST` | Computes dynamic programming LCS token diff between two revisions |
| `/api/report?id=...&format=...`| `GET` | Generates stand-alone HTML or binary PDF 1.4 audit report |

---

## 5. Current System State
- **Server Status**: Online and active at `http://127.0.0.1:8080`.
- **Database**: SQLite3 (`plagiguard.db`) initialized and verified.
- **Binary Binaries**: `bin/plagiguard`, `bin/plagiguard-cli`, `bin/plagiguard-tests`.
- **Known Limitations**: Scanned document OCR is optimized for Latin alphabet manuscripts via Apple Vision framework. Non-Latin OCR requires downloading additional macOS language packs.
