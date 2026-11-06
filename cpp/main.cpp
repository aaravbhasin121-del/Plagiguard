#include "database/DatabaseManager.hpp"
#include "service/DocumentService.hpp"
#include "service/AnalysisService.hpp"
#include "service/ReportService.hpp"
#include "ui/HttpServer.hpp"

#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>

using namespace plagiguard;

int main(int argc, char* argv[]) {
    std::cout << "========================================================\n";
    std::cout << "  PlagiGuard — Intelligent Academic Plagiarism Detection\n";
    std::cout << "  Tagline: Detect Similarity. Discover Originality.\n";
    std::cout << "  Advanced Data Structures University Project (C++20 Core)\n";
    std::cout << "========================================================\n\n";

    // 1. Initialize SQLite Database
    std::cout << "[Init] Initializing SQLite database 'plagiguard.db'...\n";
    try {
        database::DatabaseManager::getInstance().initialize("plagiguard.db");
        std::cout << "[Init] Database schema verified successfully.\n";
    } catch (const std::exception& e) {
        std::cerr << "[Error] Database initialization failed: " << e.what() << "\n";
        return 1;
    }

    // 2. Initialize Services
    std::cout << "[Init] Bootstrapping Core Services and Inverted Index...\n";
    service::DocumentService docService;
    service::AnalysisService analysisService;

    // 3. Preload Sample Test Documents if database is empty
    std::string testDocDir = "test_documents";
    if (std::filesystem::exists(testDocDir)) {
        std::cout << "[Init] Ingesting benchmark academic test documents from " << testDocDir << "...\n";
        for (const auto& entry : std::filesystem::directory_iterator(testDocDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                auto res = docService.processDocument(entry.path().string());
                if (res.success) {
                    std::cout << "  + Ingested: " << entry.path().filename() << " (" << res.document.wordCount << " words)\n";
                }
            }
        }
    }

    // 4. Run baseline batch analysis if history is empty
    if (analysisService.getHistory().empty()) {
        auto docs = docService.getAllDocuments();
        if (docs.size() >= 2) {
            std::cout << "[Init] Running baseline batch comparison for " << docs.size() << " documents...\n";
            analysisService.analyzeBatch(docs, 5, [](int pct, const std::string& status) {
                std::cout << "  [" << pct << "%] " << status << "\r" << std::flush;
            });
            std::cout << "\n[Init] Baseline analysis completed and graph initialized.\n";
        }
    }

    // 5. Start Embedded Web Application Server
    int port = 8080;
    std::string webDir = "web";
    ui::HttpServer server(port, webDir, docService, analysisService);

    if (!server.start()) {
        std::cerr << "[Error] Failed to bind HTTP server to port " << port << "\n";
        return 1;
    }

    std::cout << "\n>>> PlagiGuard UI is running at: http://127.0.0.1:" << port << " <<<\n";
    std::cout << ">>> Open your browser to explore the interactive dashboard and DSA network.\n\n";

    // Open browser on macOS automatically if not running in headless mode
#ifdef __APPLE__
    if (argc > 1 && std::string(argv[1]) == "--no-browser") {
        // Skip opening
    } else {
        system("open http://127.0.0.1:8080 2>/dev/null &");
    }
#endif

    std::cout << "[PlagiGuard] Press Ctrl+C in terminal to stop server.\n";

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}

