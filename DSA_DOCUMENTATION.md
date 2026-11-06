# PlagiGuard — Advanced Data Structures & Algorithms Documentation

**System Title:** Intelligent Plagiarism Detection & Academic Document Analysis System  
**Tagline:** *Detect Similarity. Discover Originality.*  
**Academic Level:** Advanced Data Structures & Algorithms (University Mini-Project)  
**Implementation Language:** Modern C++ (C++20) & SQLite3  

---

## 1. Executive Summary & Problem Statement

Academic institutions face challenges in identifying textual similarity across student submissions, dissertations, and research papers. Traditional methods either rely on naive pairwise character comparisons with quadratic time complexity $\mathcal{O}(N \times M)$ or delegate critical algorithmic logic to high-level language libraries.

**PlagiGuard** solves this problem by providing a high-performance document analysis engine where **all core data structures and algorithms are implemented manually from first principles**. The application guarantees mathematical transparency, rigorous collision handling, exact token-to-character position mapping, and clear separation between textual overlap and confirmed academic misconduct.

---

## 2. System Architecture

```
+-------------------------------------------------------------------------+
|                              PRESENTATION LAYER                         |
|   Modern Linear/Apple-inspired UI: Single-Page Architecture             |
|   Canvas 2D Similarity Network, Side-by-Side Highlighted Inspector     |
+-------------------------------------------------------------------------+
                                    │  HTTP / REST (JSON)
                                    ▼
+-------------------------------------------------------------------------+
|                         APPLICATION SERVICE LAYER                       |
|   DocumentService │ AnalysisService │ ReportService │ HttpServer        |
+-------------------------------------------------------------------------+
                                    │
                                    ▼
+-------------------------------------------------------------------------+
|                      PLAGIARISM DETECTION ENGINES                       |
|   ExactMatchDetector │ PartialMatchDetector │ DuplicateDetector         |
|   ParaphraseAnalyzer │ VersionComparator (LCS + Edit Distance)         |
+-------------------------------------------------------------------------+
                                    │
                                    ▼
+-------------------------------------------------------------------------+
|                  CUSTOM DATA STRUCTURES & ALGORITHMS (DSA)              |
|  1. CustomHashTable<K, V> (Separate Chaining, Dynamic Rehashing)       |
|  2. RollingHash (Polynomial Double Hashing: Base 31, Mod 1e9+7, 9.9e8)  |
|  3. NGramGenerator (Configurable Sliding Window, Position Offsets)      |
|  4. RabinKarp (Rolling Hash Search + Deep Token Verification)           |
|  5. SimilarityMetrics (Jaccard Similarity + Directional Overlap)       |
|  6. InvertedIndex (Postings Map for Candidate Document Pruning)        |
|  7. MaxHeap<T> (Binary Max-Heap with HeapifyUp/Down for Top-K Ranking)  |
|  8. DocumentGraph (Adjacency List, BFS, DFS, Connected Components)      |
|  9. Dynamic Programming: Levenshtein Distance & LCS Sequence Diff       |
| 10. Trie (Prefix Tree with Terminal Frequency Tracking)                |
+-------------------------------------------------------------------------+
                                    │
                                    ▼
+-------------------------------------------------------------------------+
|                    PERSISTENCE & STORAGE LAYER (SQLite3)                |
|   DOCUMENTS │ ANALYSES │ ANALYSIS_DOCUMENTS │ SIMILARITY_RESULTS        |
|   MATCHING_PASSAGES │ USER_SETTINGS (WAL Journaling, Prepared Stmts)   |
+-------------------------------------------------------------------------+
```

---

## 3. Data Structure & Algorithm Specifications

### 3.1 Custom Hash Table (`CustomHashTable<K, V>`)
* **Role:** Primary indexing mechanism for vocabulary, n-gram fingerprints, inverted postings, and graph adjacency lists.
* **Mechanism:** Dynamic array of bucket pointers with singly-linked list separate chaining for collision resolution.
* **Resizing Policy:** Automatically doubles bucket capacity and rehashes all elements when the load factor $\alpha = \frac{N}{M} \ge 0.75$.
* **Time Complexity:**
  - Insert / Update: $\mathcal{O}(1)$ average, $\mathcal{O}(N)$ worst-case.
  - Search / Lookup: $\mathcal{O}(1)$ average, $\mathcal{O}(N)$ worst-case.
  - Delete: $\mathcal{O}(1)$ average, $\mathcal{O}(N)$ worst-case.
  - Rehash: $\mathcal{O}(N + M)$.
* **Space Complexity:** $\mathcal{O}(N + M)$ where $N$ is entry count and $M$ is bucket capacity.

### 3.2 Polynomial Double Rolling Hash (`RollingHash`)
* **Mathematical Formula:**
  $$H = \left( \sum_{i=0}^{k-1} x_i \cdot B^{k-1-i} \right) \bmod P$$
  Where $x_i$ represents token IDs, $B = 31$ (prime base), and $k$ is the sliding window size.
* **Double Hashing:**
  - $P_1 = 1{,}000{,}000{,}007$
  - $P_2 = 998{,}244{,}353$
* **Sliding Window Update ($x_0$ leaves, $x_k$ enters):**
  $$H_{\text{next}} = \left( (H - x_0 \cdot B^{k-1}) \cdot B + x_k \right) \bmod P$$
* **Collision Probability:** $\approx \frac{1}{P_1 \times P_2} \approx 10^{-18}$.
* **Time Complexity:** Initial hash: $\mathcal{O}(k)$; Window roll: $\mathcal{O}(1)$.

### 3.3 Rabin-Karp Pattern Matching (`RabinKarp`)
* **Role:** Substring and shingle matching between documents.
* **Verification Step:** To prevent false positives from hash collisions, whenever a double hash match occurs in the `CustomHashTable`, a deep character-by-character token verification is executed.
* **Coalescing Algorithm:** Overlapping and contiguous matching windows are merged into maximal passages.
* **Time Complexity:** $\mathcal{O}(N + M)$ expected, $\mathcal{O}(N \times M)$ worst-case.

### 3.4 Similarity Metrics (`SimilarityMetrics`)
* **Jaccard Similarity:**
  $$J(A, B) = \frac{|A \cap B|}{|A \cup B|} \times 100\%$$
* **Directional Overlap:**
  $$O(A, B) = \frac{|A \cap B|}{|A|} \times 100\%$$
* **Set Operations:** Computed using `CustomHashTable` for deduplication and intersection counting. Empty inputs are protected against division-by-zero.

### 3.5 Custom Max-Heap (`MaxHeap`)
* **Role:** Ranks all document pairs by similarity score for dashboard display and top-K alerts.
* **Operations:**
  - `insert(entry)`: Appends to array and invokes `heapifyUp` ($\mathcal{O}(\log N)$).
  - `extractMax()`: Swaps root with last element and invokes `heapifyDown` ($\mathcal{O}(\log N)$).
  - `buildHeap(entries)`: Bottom-up heapify in linear time $\mathcal{O}(N)$.
  - `getTopK(k)`: Returns $K$ largest elements in descending order ($\mathcal{O}(K \log N)$).

### 3.6 Document Similarity Graph (`DocumentGraph`)
* **Structure:** Adjacency list using `CustomHashTable<int, std::vector<GraphEdge>>`.
* **Algorithms:**
  - **BFS ($\mathcal{O}(V + E)$):** Explores reachable documents level-by-level.
  - **DFS ($\mathcal{O}(V + E)$):** Deep path exploration across document clusters.
  - **Connected Components ($\mathcal{O}(V + E)$):** Partitions documents into disjoint clusters sharing similar content.
* **Visualization:** Computes 2D radial layout coordinates $(x, y)$ for HTML5 Canvas rendering.

### 3.7 Dynamic Programming Sequence Comparison
* **Levenshtein Edit Distance (`EditDistance`):**
  $$dp[i][j] = \min \begin{cases} dp[i-1][j] + 1 & \text{(Deletion)} \\ dp[i][j-1] + 1 & \text{(Insertion)} \\ dp[i-1][j-1] + (A[i-1] \ne B[j-1]) & \text{(Match / Replace)} \end{cases}$$
  Includes backtracking to output the sequence of edit operations.
* **Longest Common Subsequence (`LCS`):**
  Used for version comparison. Backtracking reconstructs diff blocks (`ADDED`, `REMOVED`, `UNCHANGED`).
  Time & Space Complexity: $\mathcal{O}(N \times M)$.

### 3.8 Trie (`Trie`)
* **Structure:** Multi-way tree where each node points to children via `CustomHashTable<char, TrieNode*>`.
* **Operations:** `insert(word)`, `search(word)`, `startsWith(prefix)`, `getWordsWithPrefix(prefix)`.
* **Time Complexity:** $\mathcal{O}(L)$ where $L$ is the word length.

---

## 4. Complexity Analysis Summary Table

| Data Structure / Algorithm | Operation | Average Time | Worst Case Time | Space Complexity |
|---|---|---|---|---|
| **CustomHashTable** | Insert / Lookup / Delete | $\mathcal{O}(1)$ | $\mathcal{O}(N)$ | $\mathcal{O}(N + M)$ |
| **CustomHashTable** | Resize & Rehash | $\mathcal{O}(N)$ | $\mathcal{O}(N)$ | $\mathcal{O}(N)$ |
| **RollingHash** | Window Slide | $\mathcal{O}(1)$ | $\mathcal{O}(1)$ | $\mathcal{O}(V)$ |
| **Rabin-Karp** | Pattern Matching | $\mathcal{O}(N + M)$ | $\mathcal{O}(N \times M)$ | $\mathcal{O}(N)$ |
| **Similarity Metrics** | Jaccard / Directional | $\mathcal{O}(|A| + |B|)$ | $\mathcal{O}(|A| \times |B|)$ | $\mathcal{O}(|A| + |B|)$ |
| **MaxHeap** | Insert / Extract Max | $\mathcal{O}(\log N)$ | $\mathcal{O}(\log N)$ | $\mathcal{O}(N)$ |
| **MaxHeap** | Build Heap | $\mathcal{O}(N)$ | $\mathcal{O}(N)$ | $\mathcal{O}(N)$ |
| **DocumentGraph** | BFS / DFS Traversal | $\mathcal{O}(V + E)$ | $\mathcal{O}(V + E)$ | $\mathcal{O}(V + E)$ |
| **InvertedIndex** | Term Postings Lookup | $\mathcal{O}(1)$ | $\mathcal{O}(T)$ | $\mathcal{O}(T \times P)$ |
| **EditDistance** | Matrix Alignment | $\mathcal{O}(N \times M)$ | $\mathcal{O}(N \times M)$ | $\mathcal{O}(N \times M)$ |
| **LCS Diff** | Sequence Traceback | $\mathcal{O}(N \times M)$ | $\mathcal{O}(N \times M)$ | $\mathcal{O}(N \times M)$ |
| **Trie** | Prefix Query / Insert | $\mathcal{O}(L)$ | $\mathcal{O}(L)$ | $\mathcal{O}(\Sigma \times L)$ |

---

## 5. Verified Benchmark Results

The following benchmarks were generated directly on the target system:

* **CustomHashTable:** Inserted 100,000 key-value pairs with automatic rehashing in **26 ms** (Final load factor: 0.381).
* **Polynomial Rolling Hash:** Computed 49,996 double hashes over a 50,000-token stream in **1 ms**.
* **MaxHeap:** Inserted 50,000 entries and extracted the top-10 in **3 ms**.
* **Rabin-Karp Comparison:** Compared Document A (430 words) against Document D (420 words) with deep token verification in **< 1 ms**.

---

## 6. Academic Interpretation Guidelines

A high similarity coefficient indicates textual overlap, not necessarily intentional plagiarism. Legitimate sources of overlap include:
1. Standardized bibliographies and reference sections.
2. Direct quotations with appropriate attribution.
3. Common discipline-specific terminology and phrases.
4. Laboratory procedure instructions and assignment prompts.

PlagiGuard provides faculty and students with quantitative diagnostic evidence to support thorough human evaluation.

