#include "dsa/CustomHashTable.hpp"
#include "dsa/RollingHash.hpp"
#include "dsa/NGramGenerator.hpp"
#include "dsa/RabinKarp.hpp"
#include "dsa/Similarity.hpp"
#include "dsa/MaxHeap.hpp"
#include "dsa/Trie.hpp"
#include "dsa/InvertedIndex.hpp"
#include "dsa/DocumentGraph.hpp"
#include "dsa/EditDistance.hpp"
#include "dsa/LCS.hpp"
#include "processing/TextPreprocessor.hpp"
#include "processing/DocumentFingerprint.hpp"
#include "processing/TextExtractor.hpp"
#include "service/PdfGenerator.hpp"
#include "detector/PlagiarismDetector.hpp"

#include <iostream>
#include <cassert>
#include <string>
#include <vector>

using namespace plagiguard;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "FAILED: " << msg << " (" << #cond << ") at line " << __LINE__ << std::endl; \
            return false; \
        } \
    } while (0)

bool testCustomHashTable() {
    std::cout << "[Test 1] CustomHashTable Operations, Collisions & Resizing...";
    dsa::CustomHashTable<std::string, int> table(8);

    TEST_ASSERT(table.empty(), "Table should be initially empty");
    TEST_ASSERT(table.size() == 0, "Initial size must be 0");

    // Insert 50 items to force multiple resizings
    for (int i = 0; i < 50; ++i) {
        table.put("key_" + std::to_string(i), i * 10);
    }
    TEST_ASSERT(table.size() == 50, "Size should be 50 after 50 puts");
    TEST_ASSERT(table.capacity() >= 64, "Table must have resized");

    // Verification
    for (int i = 0; i < 50; ++i) {
        int val = 0;
        bool found = table.get("key_" + std::to_string(i), val);
        TEST_ASSERT(found && val == i * 10, "Key retrieval must match inserted value");
    }

    // Update
    table.put("key_5", 999);
    int updatedVal = 0;
    table.get("key_5", updatedVal);
    TEST_ASSERT(updatedVal == 999, "Updated value must be reflected");
    TEST_ASSERT(table.size() == 50, "Size should not increase on update");

    // Remove
    TEST_ASSERT(table.remove("key_5"), "Removing existing key should succeed");
    TEST_ASSERT(!table.contains("key_5"), "Removed key should not exist");
    TEST_ASSERT(table.size() == 49, "Size should decrement after removal");
    TEST_ASSERT(!table.remove("non_existent"), "Removing non-existent key returns false");

    // Collision robustness
    dsa::CustomHashTable<int, std::string> collisionTable(4);
    collisionTable.put(4, "four");
    collisionTable.put(8, "eight");
    collisionTable.put(12, "twelve");
    std::string s;
    TEST_ASSERT(collisionTable.get(4, s) && s == "four", "Bucket chain key 4");
    TEST_ASSERT(collisionTable.get(8, s) && s == "eight", "Bucket chain key 8");
    TEST_ASSERT(collisionTable.get(12, s) && s == "twelve", "Bucket chain key 12");

    std::cout << " PASSED\n";
    return true;
}

bool testRollingHashAndRabinKarp() {
    std::cout << "[Test 2] RollingHash Polynomial Double Hashing & Rabin-Karp...";
    dsa::RollingHash rh(4);

    std::vector<std::string> docA = {"data", "structures", "and", "algorithms", "in", "computer", "science"};
    std::vector<std::string> docB = {"advanced", "data", "structures", "and", "algorithms", "are", "essential"};

    auto idsA = rh.mapTokensToIds(docA);
    auto idsB = rh.mapTokensToIds(docB);

    auto hashesA = rh.computeAllHashes(idsA);
    auto hashesB = rh.computeAllHashes(idsB);

    TEST_ASSERT(hashesA.size() == docA.size() - 4 + 1, "Hash count must equal N - k + 1");
    // "data structures and algorithms" is at index 0 of docA, index 1 of docB
    TEST_ASSERT(hashesA[0] == hashesB[1], "Shared 4-gram must yield identical double rolling hash");

    // Rabin-Karp Search
    auto matches = dsa::RabinKarp::findMatches(docA, docB, 4, rh);
    TEST_ASSERT(matches.size() == 1, "Must find exactly 1 matching passage");
    TEST_ASSERT(matches[0].startA == 0 && matches[0].startB == 1, "Match positions must be exact");
    TEST_ASSERT(matches[0].length == 4, "Match length must be 4");
    TEST_ASSERT(matches[0].text == "data structures and algorithms", "Matched text verified");

    std::cout << " PASSED\n";
    return true;
}

bool testNGramsAndEdgeCases() {
    std::cout << "[Test 3] N-gram Generation & Boundary Conditions (Short & Empty)...";
    dsa::NGramGenerator gen(5);

    // Empty tokens
    std::vector<std::string> emptyTokens;
    auto emptyNgrams = gen.generate(emptyTokens);
    TEST_ASSERT(emptyNgrams.empty(), "Empty input produces empty ngrams");

    // Document shorter than window size N=5
    std::vector<std::string> shortTokens = {"tree", "graph"};
    auto shortNgrams = gen.generate(shortTokens);
    TEST_ASSERT(shortNgrams.size() == 1, "Short document produces 1 partial ngram");
    TEST_ASSERT(shortNgrams[0].tokens.size() == 2, "Partial ngram retains all tokens");

    // Exact window size N=5
    std::vector<std::string> exactTokens = {"a", "b", "c", "d", "e"};
    auto exactNgrams = gen.generate(exactTokens);
    TEST_ASSERT(exactNgrams.size() == 1, "Exact window produces 1 ngram");

    // Multiple windows
    std::vector<std::string> sixTokens = {"a", "b", "c", "d", "e", "f"};
    auto sixNgrams = gen.generate(sixTokens);
    TEST_ASSERT(sixNgrams.size() == 2, "6 tokens produce 2 ngrams with N=5");

    std::cout << " PASSED\n";
    return true;
}

bool testSimilarityFormulas() {
    std::cout << "[Test 4] Jaccard & Directional Overlap Mathematical Accuracy...";
    std::vector<std::string> setA = {"w1", "w2", "w3", "w4"};
    std::vector<std::string> setB = {"w3", "w4", "w5", "w6"};

    // Intersection = {"w3", "w4"} (size 2)
    // Union = {"w1", "w2", "w3", "w4", "w5", "w6"} (size 6)
    // Jaccard = 2 / 6 * 100 = 33.333%
    // Directional A->B = 2 / 4 * 100 = 50.0%
    auto score = dsa::SimilarityMetrics::compute(setA, setB);

    TEST_ASSERT(std::abs(score.jaccard - (2.0 / 6.0 * 100.0)) < 1e-4, "Jaccard similarity formula check");
    TEST_ASSERT(std::abs(score.directionalAtoB - 50.0) < 1e-4, "Directional overlap A->B formula check");
    TEST_ASSERT(std::abs(score.directionalBtoA - 50.0) < 1e-4, "Directional overlap B->A formula check");

    // Completely disjoint
    std::vector<std::string> setC = {"x1", "x2"};
    auto zeroScore = dsa::SimilarityMetrics::compute(setA, setC);
    TEST_ASSERT(zeroScore.jaccard == 0.0, "Disjoint sets must have 0% Jaccard");

    // Identical sets
    auto fullScore = dsa::SimilarityMetrics::compute(setA, setA);
    TEST_ASSERT(std::abs(fullScore.jaccard - 100.0) < 1e-4, "Identical sets must have 100% Jaccard");
    TEST_ASSERT(std::abs(fullScore.directionalAtoB - 100.0) < 1e-4, "Identical sets must have 100% Directional");

    // Empty sets handled safely
    std::vector<std::string> emptyList;
    auto emptyScore = dsa::SimilarityMetrics::compute(emptyList, emptyList);
    TEST_ASSERT(emptyScore.jaccard == 0.0, "Empty sets yield 0% similarity without division-by-zero");

    std::cout << " PASSED\n";
    return true;
}

bool testMaxHeap() {
    std::cout << "[Test 5] Custom MaxHeap Ranking, Heapify & Top-K Extraction...";
    dsa::MaxHeap heap(8);

    heap.insert(dsa::HeapEntry(1, "A", 2, "B", 45.5));
    heap.insert(dsa::HeapEntry(1, "A", 3, "C", 88.2));
    heap.insert(dsa::HeapEntry(2, "B", 3, "C", 12.0));
    heap.insert(dsa::HeapEntry(1, "A", 4, "D", 88.2)); // Equal score test
    heap.insert(dsa::HeapEntry(3, "C", 4, "D", 67.4));

    TEST_ASSERT(heap.size() == 5, "Heap size must be 5");
    TEST_ASSERT(heap.peek().similarityScore == 88.2, "Peek must return maximum score");

    // Extract max in descending order
    auto max1 = heap.extractMax();
    TEST_ASSERT(max1.similarityScore == 88.2, "First extract must be 88.2");
    auto max2 = heap.extractMax();
    TEST_ASSERT(max2.similarityScore == 88.2, "Second extract must be equal score 88.2");
    auto max3 = heap.extractMax();
    TEST_ASSERT(max3.similarityScore == 67.4, "Third extract must be 67.4");

    // Get Top-K
    dsa::MaxHeap heap2;
    for (int i = 0; i < 20; ++i) {
        heap2.insert(dsa::HeapEntry(i, "doc", i + 1, "doc", static_cast<double>(i * 5)));
    }
    auto top3 = heap2.getTopK(3);
    TEST_ASSERT(top3.size() == 3, "Top-K should return 3 items");
    TEST_ASSERT(top3[0].similarityScore == 95.0, "Top 1 must be 95.0");
    TEST_ASSERT(top3[1].similarityScore == 90.0, "Top 2 must be 90.0");
    TEST_ASSERT(top3[2].similarityScore == 85.0, "Top 3 must be 85.0");

    std::cout << " PASSED\n";
    return true;
}

bool testTrieAndInvertedIndex() {
    std::cout << "[Test 6] Trie (Prefix Search) & Inverted Index Postings...";
    // Trie
    dsa::Trie trie;
    trie.insert("algorithm");
    trie.insert("algorithmic");
    trie.insert("algebra");
    trie.insert("data");

    TEST_ASSERT(trie.search("algorithm"), "Search existing word");
    TEST_ASSERT(!trie.search("algo"), "Prefix alone is not a terminal word");
    TEST_ASSERT(trie.startsWith("alg"), "Prefix match should be true for 'alg'");

    auto prefixWords = trie.getWordsWithPrefix("alg");
    TEST_ASSERT(prefixWords.size() == 3, "Prefix 'alg' should match 3 words");

    // Inverted Index
    dsa::InvertedIndex index;
    index.indexDocument(1, "Doc1", {"data structures", "algorithms design", "graph theory"});
    index.indexDocument(2, "Doc2", {"machine learning", "data structures", "neural networks"});

    const auto* postings = index.lookup("data structures");
    TEST_ASSERT(postings != nullptr, "Lookup should find postings for 'data structures'");
    TEST_ASSERT(postings->size() == 2, "'data structures' appears in 2 documents");

    auto candidates = index.findCandidateDocuments({"data structures", "quantum computing"}, 1);
    TEST_ASSERT(candidates.size() == 1 && candidates[0] == 2, "Candidate pruning finds Doc2");

    std::cout << " PASSED\n";
    return true;
}

bool testDocumentGraph() {
    std::cout << "[Test 7] DocumentGraph Adjacency List, BFS, DFS & Connected Components...";
    dsa::DocumentGraph graph(25.0); // 25% similarity threshold

    // Component 1: {1, 2, 3}
    graph.addUndirectedEdge(1, "DocA", 2, "DocB", 60.0);
    graph.addUndirectedEdge(2, "DocB", 3, "DocC", 50.0);

    // Below threshold edge -> should be rejected
    graph.addUndirectedEdge(1, "DocA", 4, "DocD", 15.0);

    // Component 2: {5, 6}
    graph.addUndirectedEdge(5, "DocE", 6, "DocF", 70.0);

    TEST_ASSERT(graph.vertexCount() >= 5, "Graph should have at least 5 vertices");

    // BFS Traversal
    auto bfsOrder = graph.bfs(1);
    TEST_ASSERT(bfsOrder.size() == 3, "BFS from node 1 should visit only component 1 (3 nodes)");

    // DFS Traversal
    auto dfsOrder = graph.dfs(1);
    TEST_ASSERT(dfsOrder.size() == 3, "DFS from node 1 should visit 3 nodes");

    // Connected Components
    auto components = graph.getConnectedComponents();
    TEST_ASSERT(components.size() >= 2, "Should have at least 2 disjoint connected components");

    std::cout << " PASSED\n";
    return true;
}

bool testDynamicProgramming() {
    std::cout << "[Test 8] Dynamic Programming: Levenshtein Distance & LCS Diff...";
    // Edit Distance
    std::vector<std::string> seqA = {"the", "quick", "brown", "fox"};
    std::vector<std::string> seqB = {"the", "fast", "brown", "fox"};

    size_t dist = dsa::EditDistance::computeDistance(seqA, seqB);
    TEST_ASSERT(dist == 1, "Edit distance between 'quick' and 'fast' substitution is 1");

    auto alignment = dsa::EditDistance::align(seqA, seqB);
    TEST_ASSERT(alignment.distance == 1, "Alignment distance matches DP value");

    // Longest Common Subsequence (LCS)
    std::vector<std::string> vOld = {"intro", "methods", "results", "discussion"};
    std::vector<std::string> vNew = {"intro", "methodology", "results", "conclusion"};

    size_t lcsLen = dsa::LCS::computeLength(vOld, vNew);
    TEST_ASSERT(lcsLen == 2, "LCS length ('intro', 'results') should be 2");

    auto diff = dsa::LCS::computeDiff(vOld, vNew);
    TEST_ASSERT(diff.lcsLength == 2, "Diff LCS matches");
    TEST_ASSERT(diff.additions == 2, "Two additions ('methodology', 'conclusion')");
    TEST_ASSERT(diff.deletions == 2, "Two deletions ('methods', 'discussion')");
    TEST_ASSERT(diff.unchanged == 2, "Two unchanged ('intro', 'results')");

    std::cout << " PASSED\n";
    return true;
}

bool testTextPreprocessorAndCitations() {
    std::cout << "[Test 9] Text Normalization, Positions, Citations & Unicode...";
    std::string raw = "  Data Structures & Algorithms!! \n\n According to [1], trees are graphs (Smith, 2020).  ";

    std::string norm = processing::TextPreprocessor::normalize(raw);
    TEST_ASSERT(norm.find("data structures") != std::string::npos, "Lowercase and whitespace normalized");

    auto tokens = processing::TextPreprocessor::tokenizeDetailed(raw);
    TEST_ASSERT(!tokens.empty(), "Tokenization should yield tokens");

    // Citation detection
    auto citations = processing::TextPreprocessor::detectCitations(raw);
    TEST_ASSERT(citations.size() >= 2, "Should detect bracketed citation [1] and (Smith, 2020)");

    // Unicode handling
    std::string unicodeSample = "Étude des structures de données en français avec caractères spéciaux: café, résumé, naïve.";
    auto unicodeTokens = processing::TextPreprocessor::tokenizeDetailed(unicodeSample);
    TEST_ASSERT(unicodeTokens.size() >= 10, "Unicode tokens safely parsed without crash");

    std::cout << " PASSED\n";
    return true;
}

bool testPlagiarismPipeline() {
    std::cout << "[Test 10] Complete End-to-End Plagiarism Detection Engine...";
    model::Document doc1;
    doc1.id = 1;
    doc1.name = "Original.txt";
    doc1.originalContent = "Hash tables provide average constant time lookup insertion and deletion operations. They use a hash function to map keys to bucket indices.";
    auto toks1 = processing::TextPreprocessor::tokenizeDetailed(doc1.originalContent);
    for (const auto& t : toks1) {
        doc1.tokens.push_back(t.text);
        doc1.tokenCharOffsets.push_back({t.charStart, t.charEnd});
        doc1.tokenParagraphs.push_back(t.paragraphIndex);
    }
    doc1.paragraphs = processing::TextPreprocessor::extractParagraphs(doc1.originalContent);

    // Exact copy
    model::Document doc2 = doc1;
    doc2.id = 2;
    doc2.name = "Copy.txt";

    detector::PlagiarismDetector detector(4, 5);
    auto resultExact = detector.compare(doc1, doc2);

    TEST_ASSERT(std::abs(resultExact.jaccardScore - 100.0) < 1e-4, "Exact copy yields 100% Jaccard");
    TEST_ASSERT(!resultExact.matchingPassages.empty(), "Exact copy has matching passages");

    // Partial modified copy
    model::Document doc3;
    doc3.id = 3;
    doc3.name = "Partial.txt";
    doc3.originalContent = "Hash tables provide average constant time lookup insertion and deletion operations. But binary search trees operate in logarithmic time.";
    auto toks3 = processing::TextPreprocessor::tokenizeDetailed(doc3.originalContent);
    for (const auto& t : toks3) {
        doc3.tokens.push_back(t.text);
        doc3.tokenCharOffsets.push_back({t.charStart, t.charEnd});
        doc3.tokenParagraphs.push_back(t.paragraphIndex);
    }
    doc3.paragraphs = processing::TextPreprocessor::extractParagraphs(doc3.originalContent);

    auto resultPartial = detector.compare(doc1, doc3);
    TEST_ASSERT(resultPartial.jaccardScore > 20.0 && resultPartial.jaccardScore < 90.0, "Partial overlap yields intermediate similarity");
    TEST_ASSERT(!resultPartial.matchingPassages.empty(), "Partial copy detects shared passage");

    std::cout << " PASSED\n";
    return true;
}

bool testPdfGeneration() {
    std::cout << "[Test 11] Pure C++ PDF 1.4 Generation & File Verification...";
    model::Analysis analysis;
    analysis.id = 101;
    analysis.typeString = "TWO_DOCUMENT";
    analysis.analysisDate = "2026-11-01 12:00:00";
    analysis.documentCount = 2;
    analysis.processingTimeMs = 45;

    model::SimilarityResult sim;
    sim.docAName = "TestA.txt";
    sim.docBName = "TestB.txt";
    sim.jaccardScore = 75.5;
    sim.directionalScoreAtoB = 82.0;

    model::MatchResult match;
    match.sourceStartToken = 0;
    match.sourceEndToken = 10;
    match.matchLength = 10;
    match.matchingText = "Advanced data structures and algorithms in computer science";
    sim.matchingPassages.push_back(match);
    analysis.results.push_back(sim);

    std::string testPdfPath = "reports/test_audit.pdf";
    bool success = service::PdfGenerator::generatePdf(analysis, testPdfPath);
    TEST_ASSERT(success, "PDF generation returned true");

    // Check that file exists and starts with %PDF-1.4
    std::ifstream check(testPdfPath, std::ios::binary);
    TEST_ASSERT(check.is_open(), "Generated PDF file could be opened");

    std::string header(8, '\0');
    check.read(&header[0], 8);
    TEST_ASSERT(header.find("%PDF-1.4") != std::string::npos, "PDF starts with %PDF-1.4 magic bytes");

    std::cout << " PASSED\n";
    return true;
}

int main() {
    std::cout << "\n=======================================================\n";
    std::cout << "  PLAGIGUARD C++20 COMPREHENSIVE AUTOMATED TEST SUITE\n";
    std::cout << "=======================================================\n\n";

    int passed = 0;
    int total = 11;

    if (testCustomHashTable()) passed++;
    if (testRollingHashAndRabinKarp()) passed++;
    if (testNGramsAndEdgeCases()) passed++;
    if (testSimilarityFormulas()) passed++;
    if (testMaxHeap()) passed++;
    if (testTrieAndInvertedIndex()) passed++;
    if (testDocumentGraph()) passed++;
    if (testDynamicProgramming()) passed++;
    if (testTextPreprocessorAndCitations()) passed++;
    if (testPlagiarismPipeline()) passed++;
    if (testPdfGeneration()) passed++;

    std::cout << "\n-------------------------------------------------------\n";
    std::cout << "TEST RESULTS: " << passed << " / " << total << " test suites passed.\n";
    std::cout << "-------------------------------------------------------\n";

    return (passed == total) ? 0 : 1;
}
