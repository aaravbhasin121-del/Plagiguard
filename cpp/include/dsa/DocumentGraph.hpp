#pragma once

#include "CustomHashTable.hpp"
#include <string>
#include <vector>
#include <queue>
#include <cmath>
#include <algorithm>

namespace plagiguard::dsa {

/**
 * @brief Represents a vertex in the document similarity graph.
 */
struct GraphVertex {
    int id;
    std::string name;
    size_t wordCount;
    double x; // 2D layout coordinate
    double y;

    GraphVertex() : id(-1), wordCount(0), x(0.0), y(0.0) {}
    GraphVertex(int vId, const std::string& vName, size_t wc = 0)
        : id(vId), name(vName), wordCount(wc), x(0.0), y(0.0) {}
};

/**
 * @brief Represents a weighted directed/undirected edge in the graph.
 */
struct GraphEdge {
    int targetId;
    std::string targetName;
    double similarity; // weight [0.0 - 100.0]

    GraphEdge() : targetId(-1), similarity(0.0) {}
    GraphEdge(int tId, const std::string& tName, double sim)
        : targetId(tId), targetName(tName), similarity(sim) {}
};

/**
 * @brief DocumentGraph - Graph modeling document relationships and clusters.
 *
 * Requirements:
 * - Adjacency list implementation
 * - BFS and DFS traversal algorithms
 * - Connected components detection (disjoint clusters)
 * - Most similar neighbors retrieval
 * - Configurable edge threshold
 * - 2D layout coordinates calculation for visualization
 *
 * Complexity:
 * - BFS / DFS: O(V + E)
 * - Connected Components: O(V + E)
 */
class DocumentGraph {
private:
    double similarityThreshold;
    CustomHashTable<int, GraphVertex> vertices;
    CustomHashTable<int, std::vector<GraphEdge>> adjacencyList;

public:
    explicit DocumentGraph(double threshold = 20.0)
        : similarityThreshold(threshold), vertices(64), adjacencyList(64) {}

    void setThreshold(double threshold) {
        similarityThreshold = threshold;
    }

    double getThreshold() const {
        return similarityThreshold;
    }

    void addVertex(int id, const std::string& name, size_t wordCount = 0) {
        vertices.put(id, GraphVertex(id, name, wordCount));
        if (!adjacencyList.contains(id)) {
            adjacencyList.put(id, std::vector<GraphEdge>());
        }
    }

    void addEdge(int fromId, int toId, const std::string& toName, double similarity) {
        if (similarity < similarityThreshold) return;

        std::vector<GraphEdge>* edges = adjacencyList.find(fromId);
        if (edges != nullptr) {
            // Check if edge already exists, update if higher similarity
            bool found = false;
            for (auto& e : *edges) {
                if (e.targetId == toId) {
                    if (similarity > e.similarity) e.similarity = similarity;
                    found = true;
                    break;
                }
            }
            if (!found) {
                edges->emplace_back(toId, toName, similarity);
            }
        }
    }

    void addUndirectedEdge(int aId, const std::string& aName,
                           int bId, const std::string& bName,
                           double similarity) {
        if (similarity < similarityThreshold) return;
        addVertex(aId, aName);
        addVertex(bId, bName);
        addEdge(aId, bId, bName, similarity);
        addEdge(bId, aId, aName, similarity);
    }

    const std::vector<GraphEdge>* getNeighbors(int vertexId) const {
        return adjacencyList.find(vertexId);
    }

    std::vector<int> getVertexIds() const {
        return vertices.keys();
    }

    bool getVertex(int id, GraphVertex& outVertex) const {
        return vertices.get(id, outVertex);
    }

    /**
     * @brief Breadth-First Search (BFS) starting from startVertex.
     * Explores reachable documents level-by-level.
     */
    std::vector<int> bfs(int startVertex) const {
        std::vector<int> traversal;
        if (!vertices.contains(startVertex)) return traversal;

        CustomHashTable<int, bool> visited(vertices.size() * 2 + 16);
        std::queue<int> q;

        visited.put(startVertex, true);
        q.push(startVertex);

        while (!q.empty()) {
            int u = q.front();
            q.pop();
            traversal.push_back(u);

            const std::vector<GraphEdge>* neighbors = getNeighbors(u);
            if (neighbors != nullptr) {
                for (const auto& edge : *neighbors) {
                    if (!visited.contains(edge.targetId)) {
                        visited.put(edge.targetId, true);
                        q.push(edge.targetId);
                    }
                }
            }
        }
        return traversal;
    }

    /**
     * @brief Depth-First Search (DFS) starting from startVertex.
     */
    std::vector<int> dfs(int startVertex) const {
        std::vector<int> traversal;
        if (!vertices.contains(startVertex)) return traversal;

        CustomHashTable<int, bool> visited(vertices.size() * 2 + 16);
        std::vector<int> stack;

        stack.push_back(startVertex);

        while (!stack.empty()) {
            int u = stack.back();
            stack.pop_back();

            if (!visited.contains(u)) {
                visited.put(u, true);
                traversal.push_back(u);

                const std::vector<GraphEdge>* neighbors = getNeighbors(u);
                if (neighbors != nullptr) {
                    for (auto it = neighbors->rbegin(); it != neighbors->rend(); ++it) {
                        if (!visited.contains(it->targetId)) {
                            stack.push_back(it->targetId);
                        }
                    }
                }
            }
        }
        return traversal;
    }

    /**
     * @brief Computes all connected components using BFS.
     * Identifies clusters of mutually similar documents.
     */
    std::vector<std::vector<int>> getConnectedComponents() const {
        std::vector<std::vector<int>> components;
        CustomHashTable<int, bool> visited(vertices.size() * 2 + 16);
        std::vector<int> allNodes = vertices.keys();

        for (int node : allNodes) {
            if (!visited.contains(node)) {
                std::vector<int> component;
                std::queue<int> q;

                visited.put(node, true);
                q.push(node);

                while (!q.empty()) {
                    int u = q.front();
                    q.pop();
                    component.push_back(u);

                    const std::vector<GraphEdge>* neighbors = getNeighbors(u);
                    if (neighbors != nullptr) {
                        for (const auto& edge : *neighbors) {
                            if (!visited.contains(edge.targetId)) {
                                visited.put(edge.targetId, true);
                                q.push(edge.targetId);
                            }
                        }
                    }
                }
                components.push_back(component);
            }
        }
        return components;
    }

    /**
     * @brief Computes 2D circular/force layout coordinates for graph visualization.
     */
    void computeLayout(double width = 800.0, double height = 600.0) {
        std::vector<int> nodes = vertices.keys();
        if (nodes.empty()) return;

        double centerX = width / 2.0;
        double centerY = height / 2.0;
        double radius = std::min(centerX, centerY) * 0.75;

        for (size_t i = 0; i < nodes.size(); ++i) {
            double angle = 2.0 * M_PI * static_cast<double>(i) / static_cast<double>(nodes.size());
            GraphVertex* v = vertices.find(nodes[i]);
            if (v != nullptr) {
                v->x = centerX + radius * std::cos(angle);
                v->y = centerY + radius * std::sin(angle);
            }
        }
    }

    size_t vertexCount() const { return vertices.size(); }

    size_t edgeCount() const {
        size_t count = 0;
        for (int vId : vertices.keys()) {
            const auto* edges = getNeighbors(vId);
            if (edges != nullptr) count += edges->size();
        }
        return count / 2; // Undirected pairs
    }

    void clear() {
        vertices.clear();
        adjacencyList.clear();
    }
};

} // namespace plagiguard::dsa

