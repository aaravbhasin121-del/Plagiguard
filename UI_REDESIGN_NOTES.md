# PlagiGuard — UI/UX Master Redesign & Architecture Notes

## 1. Executive Summary & Design Philosophy

**PlagiGuard** has been transformed from a basic utility interface into a **production-grade academic document-integrity and data structures analysis platform**. 

Inspired by modern developer-first and design-forward systems such as **Linear**, **Stripe Dashboard**, **Raycast**, and **Apple**, the redesign emphasizes:
- **Clarity & Evidence**: Every similarity percentage is backed by mathematical explanations ($J(A, B)$ and $O(A \to B)$) and verbatim token-by-token passage evidence.
- **Deep Midnight Aesthetic**: A layered, dark palette designed for extended research and grading sessions without eye fatigue.
- **Microinteractions**: Fluid state transitions, real-time N-gram slider explanations, radial SVG score gauges, and live graph traversal animations.
- **100% C++ Computational Primacy**: The graphical user interface acts as a transparent window directly into the C++20 DSA engine, making data structures demonstrable and understandable for academic evaluation.

---

## 2. Visual Identity & Color System

The user interface follows a strict, disciplined color hierarchy avoiding garish gradients or distracting animations:

| Element | Hex Code | Purpose |
| :--- | :--- | :--- |
| **Main Background** | `#0B1120` | Deep midnight navy providing maximum contrast and depth |
| **Sidebar Surface** | `#111B2E` | Distinct architectural partition for navigation and status |
| **Card Surface** | `#17243A` | Primary container surface with $1\text{px}$ subtle borders |
| **Elevated Surface** | `#1E2D45` | Hover states, modals, code blocks, and nested panels |
| **Primary Accent** | `#10B981` | Emerald green representing verified original text and clean states |
| **Secondary Accent** | `#38BDF8` | Sky blue for BFS queue traversals and informational tags |
| **Highlight Accent** | `#818CF8` | Indigo/violet for DFS stack traversals and cluster boundaries |
| **Warning Accent** | `#F59E0B` | Amber for duplicate detection warnings and partial overlap |
| **Error Accent** | `#EF4444` | Coral/red for severe plagiarism thresholds and file parse failures |
| **Primary Typography**| `#F1F5F9` | Inter font for crisp, legible UI elements |
| **Monospace / Code** | `#FEF08A` on `#17243A` | JetBrains Mono for token passages, offsets, and Big-O formulas |

---

## 3. Global Application Shell

### Sidebar Navigation
- **Branding**: Shield icon with primary product name and subtitle: *"Document Integrity Engine"*.
- **Navigation Tabs**:
  1. **Overview**: Executive control centre and system stats.
  2. **New Analysis**: The central 4-step guided workflow.
  3. **Document Library**: Comprehensive repository management with instant search, inspection modal, and 1-click fingerprint copy.
  4. **Analysis Results**: Dynamic radial score gauge, mathematical breakdowns, paragraph heatmaps, and passage evidence cards.
  5. **Side-by-Side**: High-contrast, synchronized dual-pane passage comparison.
  6. **Similarity Network**: Force-directed interactive canvas graph with live C++ BFS & DFS traversal visualizers.
  7. **Analysis History**: SQLite audit records with instant 1-click reload into results.
  8. **Version Comparison**: Dynamic Programming LCS sequence alignment diff.
  9. **DSA Engine Explorer**: Interactive technical showcase of all 11 manually implemented data structures.
  10. **Settings**: Configurable thresholds, N-gram defaults, and theme toggle.
- **Engine Heartbeat Status**: Real-time polling `/api/status` displaying a pulsating green indicator `● C++ Engine Active (v2.4 - Native)` when online, or shifting to red if the backend is unreachable.

### Top Navigation Bar
- Glassmorphic backdrop blur (`backdrop-filter: blur(12px)`).
- Dynamic breadcrumb navigation (`PlagiGuard / SECTION`).
- Theme toggle (Light/Dark mode) and Quick Analysis action button.

---

## 4. Re-Engineered Core Workflows

### A. New Analysis — Guided 4-Step Stepper
1. **Step 1: Select Documents**:
   - Universal drag-and-drop zone supporting `.pdf`, `.docx`, and `.txt`.
   - Apple Vision Framework Neural OCR automatically parses scanned or image-based PDFs at $2\times$ scale with zero cloud dependencies.
   - Dedicated **File Queue** displaying filename, format badge, byte size, word count, and real-time ingestion status.
   - **Duplicate Detection Modal**: Rather than an ambiguous warning, the system identifies exact-content duplicates via 128-bit FNV-1a digests, shows the existing matching document name, and offers to reuse it for comparison.
2. **Step 2: Configure Comparison**:
   - **Mode 1 (Two-Document Pair)**: Subject Document A $\leftrightarrow$ Comparison Document B, complete with a quick-swap button ($\rightleftarrows$) and validation preventing duplicate selection.
   - **Mode 2 (Batch Collection Analysis)**: Calculates and displays $N \times (N-1) / 2$ unique comparison pairs before running, utilizing the custom Max-Heap to rank the most similar pairs.
   - **Mode 3 (Version Comparison)**: Analyzes older revision vs newer revision via dynamic programming LCS.
   - **Matching Sensitivity Slider**: Range 3 to 10 with live contextual guidance (e.g., 3-4: High Sensitivity, 5-6: Balanced Academic, 7-10: Verbatim Matching).
3. **Step 3: Run Detection (Dedicated Execution Modal)**:
   - Replaces generic spinners with a vertical 7-stage C++ engine timeline showing exact algorithmic progress:
     - Stage 1: Text Extraction & Validation
     - Stage 2: Text Normalization & Tokenization
     - Stage 3: Word N-gram Generation
     - Stage 4: Polynomial Rolling Hash Calculation
     - Stage 5: Rabin-Karp Matching & Deep Collision Verification
     - Stage 6: Jaccard & Directional Similarity Calculation
     - Stage 7: Paragraph Heatmap & Report Finalization
4. **Step 4: Review Results**: Automatically routes to the Results Dashboard with radial gauge and full evidence.

---

### B. Results Dashboard & Mathematical Transparency
- **Radial Score Gauge**: Animated SVG circular progress indicator color-coded by severity (Green $<25\%$, Amber $25-60\%$, Red $>60\%$).
- **Mathematical Formula Accordion**: Expands to show KaTeX-formatted formulas for:
  $$J(A, B) = \frac{|A \cap B|}{|A \cup B|} \times 100\% \quad \text{and} \quad O(A \to B) = \frac{|A \cap B|}{|A|} \times 100\%$$
- **Paragraph-Level Similarity Heatmap**: Interactive grid cells ($P_1, P_2, \dots$) displaying local density of matching shingles across Document A.
- **Verified Matching Passage Evidence**: Side-by-side excerpt cards showing word length, token indices, and a 1-click "Inspect in Side-by-Side" button that scrolls smoothly to the highlighted match.
- **DSA Execution Report**: Collapsible technical breakdown detailing which algorithms ran, their input/output, and Big-O complexities.

---

### C. Similarity Network & C++ Graph Visualizer
- **Fixed Blank Screen**: Populates graph nodes from all database documents and edges from all historical similarity comparisons.
- **Interactive Canvas**: Renders nodes with radii proportional to document length, connected by weighted edges reflecting similarity percentages.
- **Threshold Filtering**: Slider adjusts the minimum similarity required to render an edge (5% to 80%) dynamically.
- **C++ Graph Traversal Animations**:
  - **Animate BFS**: Calls `/api/graph/bfs` to execute queue-based breadth-first exploration in C++, animating nodes in level-order with a glowing sky-blue highlight.
  - **Animate DFS**: Calls `/api/graph/dfs` to execute stack-based depth-first exploration in C++, animating nodes with a glowing indigo highlight.
  - **Live Traversal Log**: Displays the exact traversal sequence in real-time (e.g., `Doc 4 ➔ Doc 7 (Level 1) ➔ Doc 5 (Level 2)`).

---

### D. Document Library & Inspection
- Instant live search and extension filtering (`All`, `PDF`, `DOCX`, `TXT`).
- Shortened 128-bit hex fingerprints (`8fa3b7c29e1d...`) with 1-click clipboard copy.
- **Document Inspection Modal**: Inspects document metadata, character count, word count, upload date, and extracted text preview.
- 1-click **Compare** action that pre-selects the document in the New Analysis panel.

---

### E. Inside the C++ Detection Engine (Algorithm Explorer)
A dedicated interactive educational module showcasing all 11 manually implemented data structures:
1. **Custom Hash Table** (Separate Chaining with linked-list nodes & load factor resizing)
2. **Polynomial Rolling Hash** (Double Moduli $10^9+7$, $998244353$ with $O(1)$ sliding updates)
3. **Rabin-Karp Algorithm** (Rolling hash search & deep token collision verification)
4. **N-gram Sliding Window** (Contiguous word sequence generator)
5. **Jaccard & Directional Metrics** (Mathematical set intersection over union)
6. **Binary Max-Heap** (Dynamic priority queue with $O(\log N)$ heapify)
7. **Document Similarity Graph** (Weighted adjacency list & 2D layout)
8. **Breadth-First Search** (FIFO queue-based level exploration)
9. **Depth-First Search** (LIFO stack-based branch traversal)
10. **Longest Common Subsequence (LCS)** (2D Dynamic Programming matrix diff)
11. **Vocabulary Prefix Trie** (Pointer-based character tree for vocabulary indexing)

