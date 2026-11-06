# PlagiGuard — Intelligent Academic Plagiarism Detection & Integrity Engine

**Tagline:** *Detect Similarity. Discover Originality.*  
**Core Implementation:** 100% Native Modern C++ (C++20) & SQLite3  
**Course:** Advanced Data Structures & Algorithms University Project  

---

## 🌟 Overview

**PlagiGuard** is an academic document comparison and similarity detection platform. Engineered with **100% manual, from-scratch implementations of core data structures and algorithms** in modern C++20, it avoids standard library container shortcuts for core algorithms and provides mathematical transparency and passage-level evidence.

The frontend is designed around modern design systems inspired by **Linear**, **Stripe Dashboard**, **Raycast**, and **Apple**, operating on a deep midnight navy palette (`#0B1120`) with emerald green accents (`#10B981`).

---

## 🚀 Key Features

### 1. Custom Data Structures & Algorithms (C++20 Native)
1. **Custom Hash Table:** Separate chaining with dynamic linked-list nodes, load factor monitoring ($\alpha \ge 0.75$), and capacity doubling.
2. **Polynomial Rolling Hash:** Double hashing with prime moduli ($10^9+7$ and $998{,}244{,}353$) and base $B=31$ providing $\mathcal{O}(1)$ sliding window updates.
3. **Rabin-Karp Pattern Matching:** Rolling hash candidate detection backed by deep token sequence verification to eliminate hash collisions.
4. **N-gram Shingle Generator:** Configurable sliding window size ($3$ to $10$ words) with precise byte and paragraph offset mapping.
5. **Similarity Metrics:** Jaccard similarity and Directional overlap with safe zero-denominator handling:
   $$J(A, B) = \frac{|A \cap B|}{|A \cup B|} \times 100\% \quad \text{and} \quad O(A \to B) = \frac{|A \cap B|}{|A|} \times 100\%$$
6. **Custom Binary Max-Heap:** Dynamic priority queue with $\mathcal{O}(\log N)$ `heapifyUp` and `heapifyDown`, providing $\mathcal{O}(K \log N)$ top-K match ranking.
7. **Document Similarity Graph:** Weighted adjacency list modeling document relationships where edge weights represent Jaccard scores exceeding configurable thresholds.
8. **Breadth-First Search (BFS):** FIFO queue-based exploration exploring document clusters level-by-level.
9. **Depth-First Search (DFS):** LIFO stack-based traversal tracing copying chains and connected components.
10. **Dynamic Programming:** Levenshtein Edit Distance with operation backtracking and Longest Common Subsequence (LCS) for version revision diffing.
11. **Trie (Prefix Tree):** Pointer-based character tree for vocabulary indexing and rapid prefix queries.
12. **Inverted Index:** Term-to-document postings list for candidate pruning without all-pairs overhead.

---

### 2. Multi-Format Document Ingestion & Apple Vision Neural OCR
* **PDF Direct Extraction & Neural OCR:** Uses Apple's native `PDFKit` framework. When scanned or image-based PDFs are uploaded, the engine automatically renders pages at $2\times$ scale and runs Apple's native `Vision` Neural OCR (`VNRecognizeTextRequest`) with offline language correction.
* **Microsoft Word (.docx):** Native macOS `/usr/bin/textutil` extraction with raw XML unpack fallback (`word/document.xml`).
* **Plain Text (.txt):** Stream-buffered UTF-8 reader.
* **128-bit Fingerprinting & Duplicate Detection:** Generates unique content digests using 64-bit dual-pass FNV-1a hashing to prevent redundant storage and offer transparent duplicate reuse.

---

### 3. Redesigned User Interface (Linear / Stripe / Raycast Aesthetic)
* **Overview Dashboard:** Hero banner, 4 key metrics, dynamic similarity distribution histogram, active DSA stack monitor, and recent activity audit table.
* **New Analysis Guided Stepper:** 4-step workflow (Select Documents $\to$ Configure $\to$ Run Detection $\to$ Review Results).
* **3 Comparison Modes:**
  * **Mode 1 (Two-Document Pair):** Document A $\leftrightarrow$ Document B with quick-swap button ($\rightleftarrows$).
  * **Mode 2 (Batch Collection Analysis):** Cross-compares all library documents ($N \times (N-1)/2$ pairs) and ranks with Max-Heap.
  * **Mode 3 (Version Comparison):** LCS sequence alignment for document revisions.
* **Analysis Execution Stage View:** 7-stage vertical timeline explaining each C++ algorithmic operation in real-time.
* **Results Dashboard:**
  * Animated SVG radial similarity score gauge.
  * Mathematical Transparency accordion with formal LaTeX definitions.
  * Paragraph-level similarity heatmap.
  * Side-by-side verified passage cards with token indices and "Inspect in Side-by-Side" jump action.
* **Synchronized Side-by-Side Viewer:** Dual-pane document viewer with high-contrast highlighted matching spans.
* **Similarity Network Visualizer:** Interactive Canvas graph with force-directed layout and animated C++ BFS/DFS traversal logs.
* **Document Library:** Instant search, type filtering, 1-click fingerprint copy, and inspection modal.
* **Analysis History:** SQLite audit records with instant 1-click reload into the Results dashboard.
* **DSA Engine Explorer:** Interactive technical showcase detailing inputs, outputs, and Big-O complexities for all 11 DSA components.

---

## 🛠️ Quick Start & Build Instructions

### Prerequisites
* macOS (with Apple Clang supporting C++20)
* `make`
* SQLite3 development libraries (`-lsqlite3`)

### 1. Build All Binaries
```bash
make all
```
Compiles three native executables in `bin/`:
* `bin/plagiguard`: Embedded HTTP application server and web interface.
* `bin/plagiguard-cli`: Terminal inspection and comparison tool.
* `bin/plagiguard-tests`: Automated unit test runner.

---

### 2. Run Automated Unit Test Suite
```bash
make test
```
Executes all 11 test suites verifying custom hash table resizing, rolling hash collision rates, Rabin-Karp verification, heap ranking, graph traversals, and PDF generation.

---

### 3. Launch Desktop Server & UI
```bash
make run
```
Or directly:
```bash
./bin/plagiguard --no-browser
```
Access the application at:
**[http://127.0.0.1:8080](http://127.0.0.1:8080)**

---

### 4. Command-Line Interface (`plagiguard-cli`)

#### Compare two documents:
```bash
./bin/plagiguard-cli compare test_documents/document_a.txt test_documents/document_d_partial_copy.txt 5
```

#### Run Dynamic Programming Version Diff (LCS):
```bash
./bin/plagiguard-cli version test_documents/document_a.txt test_documents/document_d_partial_copy.txt
```

#### Run Algorithmic Benchmark:
```bash
./bin/plagiguard-cli benchmark
```

---

## 📂 Project Structure

```
PlagiGuard/
├── Makefile                          # Apple Clang C++20 build configuration
├── README.md                         # Quick start and documentation
├── UI_REDESIGN_NOTES.md              # Detailed UI/UX architecture notes
├── DEVELOPMENT_PROGRESS.md           # Bug fixes and test logs
├── DSA_DOCUMENTATION.md              # Comprehensive academic DSA report
├── ACADEMIC_EVALUATION.md            # University evaluation rubric notes
├── cpp/
│   ├── main.cpp                      # Server daemon entry point
│   ├── cli_main.cpp                  # CLI entry point
│   ├── include/
│   │   ├── dsa/                      # Custom data structures & algorithms (100% manual)
│   │   │   ├── CustomHashTable.hpp   # Separate chaining hash table with resizing
│   │   │   ├── RollingHash.hpp       # Polynomial double hash (10^9+7, 998244353)
│   │   │   ├── RabinKarp.hpp         # String pattern matcher & collision verifier
│   │   │   ├── NGramGenerator.hpp    # Word sliding window shingle generator
│   │   │   ├── Similarity.hpp        # Jaccard & Directional overlap calculations
│   │   │   ├── MaxHeap.hpp           # Binary max-heap priority queue
│   │   │   ├── DocumentGraph.hpp     # Adjacency list, BFS, DFS, and layout engine
│   │   │   ├── EditDistance.hpp      # Levenshtein distance 2D dynamic programming
│   │   │   ├── LCS.hpp               # Longest Common Subsequence DP diff
│   │   │   ├── Trie.hpp              # Prefix tree for vocabulary indexing
│   │   │   └── InvertedIndex.hpp     # Postings list for document candidate pruning
│   │   ├── processing/               # Text extraction & normalization
│   │   │   ├── AppleOcrExtractor.hpp # Apple Vision Neural OCR & textutil extraction
│   │   │   ├── TextExtractor.hpp     # Unified multi-format parser
│   │   │   ├── TextPreprocessor.hpp  # Tokenization, boundaries, offsets
│   │   │   └── DocumentFingerprint.hpp# 128-bit dual-pass FNV-1a fingerprint
│   │   ├── detector/                 # Plagiarism detection orchestrators
│   │   ├── database/                 # SQLite3 repositories
│   │   ├── service/                  # Business services & PDF generator
│   │   ├── ui/                       # Embedded HTTP & REST server
│   │   └── util/                     # JSON & string utilities
│   └── tests/
│       └── test_runner.cpp           # 11 automated test suites
├── web/
│   ├── index.html                    # 11 application panels & modals
│   ├── style.css                     # Linear/Raycast design system
│   └── app.js                        # Frontend controller & canvas graph visualizer
└── test_documents/                   # Benchmark academic evaluation manuscripts
```

---

## ⚖️ Academic Integrity Notice

Similarity percentages quantify textual and shingle overlap. They do not constitute proof of academic misconduct. Legitimate overlap includes standard bibliographies, technical terminology, and properly cited quotations. Academic evaluation requires faculty review.
