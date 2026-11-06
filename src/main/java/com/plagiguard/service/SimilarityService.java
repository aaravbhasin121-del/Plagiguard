package com.plagiguard.service;

import com.plagiguard.dsa.CustomHashTable;
import com.plagiguard.model.Document;
import com.plagiguard.model.MatchResult;
import com.plagiguard.model.SimilarityResult;

import java.util.*;

/**
 * Computes similarity scores between documents.
 *
 * Implements:
 * 1. Jaccard Similarity: |A∩B| / |A∪B| * 100
 * 2. Directional Overlap: |A∩B| / |A| * 100
 *
 * Uses custom hash table for set operations (intersection, union).
 * DOES NOT use Java's built-in HashSet/HashMap for core computations.
 */
public class SimilarityService {

    /**
     * Compute Jaccard similarity between two n-gram sets.
     * J(A,B) = |A∩B| / |A∪B| * 100
     *
     * @param ngramsA n-gram strings from document A
     * @param ngramsB n-gram strings from document B
     * @return Jaccard score 0-100
     */
    public double computeJaccard(List<String> ngramsA, List<String> ngramsB) {
        if (ngramsA == null || ngramsB == null || ngramsA.isEmpty() || ngramsB.isEmpty()) {
            return 0.0;
        }
        List<String> inter = intersection(ngramsA, ngramsB);
        List<String> uni   = union(ngramsA, ngramsB);
        if (uni.isEmpty()) return 0.0;
        return (double) inter.size() / uni.size() * 100.0;
    }

    /**
     * Compute directional overlap (how much of A is in B).
     * O(A,B) = |A∩B| / |A| * 100
     *
     * @param ngramsA n-gram strings from document A (reference)
     * @param ngramsB n-gram strings from document B
     * @return directional overlap score 0-100
     */
    public double computeDirectionalOverlap(List<String> ngramsA, List<String> ngramsB) {
        if (ngramsA == null || ngramsA.isEmpty()) return 0.0;
        if (ngramsB == null || ngramsB.isEmpty()) return 0.0;
        List<String> inter = intersection(ngramsA, ngramsB);
        // Count unique n-grams in A for the denominator
        CustomHashTable<String, Integer> uniqueA = new CustomHashTable<>();
        for (String ng : ngramsA) {
            uniqueA.put(ng, 1);
        }
        int sizeA = uniqueA.size();
        if (sizeA == 0) return 0.0;
        return (double) inter.size() / sizeA * 100.0;
    }

    /**
     * Find intersection of two n-gram lists.
     * Uses custom hash table, not Java HashSet.
     *
     * @return list of n-grams in both A and B (deduplicated)
     */
    public List<String> intersection(List<String> ngramsA, List<String> ngramsB) {
        // Load all n-grams from A into custom hash table
        CustomHashTable<String, Integer> tableA = new CustomHashTable<>();
        for (String ng : ngramsA) {
            tableA.put(ng, 1);
        }

        // Walk B and collect those that appear in A (track what we've already added)
        CustomHashTable<String, Integer> seen = new CustomHashTable<>();
        List<String> result = new ArrayList<>();
        for (String ng : ngramsB) {
            if (tableA.containsKey(ng) && !seen.containsKey(ng)) {
                result.add(ng);
                seen.put(ng, 1);
            }
        }
        return result;
    }

    /**
     * Find union of two n-gram lists (deduplicated).
     */
    public List<String> union(List<String> ngramsA, List<String> ngramsB) {
        CustomHashTable<String, Integer> table = new CustomHashTable<>();
        for (String ng : ngramsA) {
            table.put(ng, 1);
        }
        for (String ng : ngramsB) {
            table.put(ng, 1);
        }
        return table.keys();
    }

    /**
     * Compute cosine similarity between TF-IDF vectors.
     * For paraphrase analysis.
     */
    public double cosineSimiliarity(Map<String, Double> vecA, Map<String, Double> vecB) {
        if (vecA == null || vecB == null || vecA.isEmpty() || vecB.isEmpty()) return 0.0;

        double dotProduct = 0.0;
        double normA      = 0.0;
        double normB      = 0.0;

        // Dot product (only iterate over A's keys and look up in B)
        for (Map.Entry<String, Double> entry : vecA.entrySet()) {
            double valA = entry.getValue();
            normA += valA * valA;
            Double valB = vecB.get(entry.getKey());
            if (valB != null) {
                dotProduct += valA * valB;
            }
        }
        for (double valB : vecB.values()) {
            normB += valB * valB;
        }

        if (normA == 0.0 || normB == 0.0) return 0.0;
        return dotProduct / (Math.sqrt(normA) * Math.sqrt(normB));
    }

    /**
     * Build complete SimilarityResult from two documents and their match results.
     */
    public SimilarityResult buildResult(Document docA, Document docB,
                                        List<String> ngramsA, List<String> ngramsB,
                                        List<MatchResult> matches, int analysisId,
                                        long processingTimeMs) {
        double jaccard     = computeJaccard(ngramsA, ngramsB);
        double directional = computeDirectionalOverlap(ngramsA, ngramsB);

        List<String> inter = intersection(ngramsA, ngramsB);

        // Count unique n-grams in each document
        CustomHashTable<String, Integer> uniqueA = new CustomHashTable<>();
        CustomHashTable<String, Integer> uniqueB = new CustomHashTable<>();
        for (String ng : ngramsA) uniqueA.put(ng, 1);
        for (String ng : ngramsB) uniqueB.put(ng, 1);

        SimilarityResult result = new SimilarityResult();
        result.setAnalysisId(analysisId);
        result.setDocumentAId(docA.getId());
        result.setDocumentBId(docB.getId());
        result.setDocumentAName(docA.getName());
        result.setDocumentBName(docB.getName());
        result.setJaccardScore(jaccard);
        result.setDirectionalScore(directional);
        result.setMatchingCount(inter.size());
        result.setMatchingPassageCount(matches != null ? matches.size() : 0);
        result.setUniqueNgramsA(uniqueA.size());
        result.setUniqueNgramsB(uniqueB.size());
        result.setProcessingTimeMs(processingTimeMs);
        result.setMatchingPassages(matches != null ? matches : new ArrayList<>());
        return result;
    }

    /**
     * Get human-readable interpretation of a Jaccard score.
     * Important: Does NOT claim to be a plagiarism percentage.
     */
    public String interpretScore(double jaccardScore) {
        if (jaccardScore >= 80.0) {
            return "Very High Similarity — substantial textual overlap detected. "
                 + "Content may be near-identical or extensively copied.";
        } else if (jaccardScore >= 60.0) {
            return "High Similarity — significant shared n-gram sequences. "
                 + "May indicate copied passages or common source material.";
        } else if (jaccardScore >= 40.0) {
            return "Moderate Similarity — notable overlap. "
                 + "Could reflect shared terminology, paraphrasing, or topic similarity.";
        } else if (jaccardScore >= 20.0) {
            return "Low-Moderate Similarity — some shared phrases or vocabulary. "
                 + "Common in documents addressing the same subject area.";
        } else if (jaccardScore >= 5.0) {
            return "Low Similarity — minimal shared content. "
                 + "Likely independent documents with incidental overlap.";
        } else {
            return "Negligible Similarity — documents appear substantially different.";
        }
    }
}
