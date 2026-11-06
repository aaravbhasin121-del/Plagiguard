#pragma once

#include "model/Analysis.hpp"
#include "PdfGenerator.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace plagiguard::service {

/**
 * @brief ReportService - Generates publication-ready academic plagiarism analysis reports.
 * Supports both standalone responsive HTML format and binary PDF 1.4 documents.
 */
class ReportService {
public:
    static bool generatePdfReport(const model::Analysis& analysis, const std::string& outputPath) {
        return PdfGenerator::generatePdf(analysis, outputPath);
    }
    static std::string generateHtmlReport(const model::Analysis& analysis) {
        std::ostringstream html;

        html << R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>PlagiGuard Analysis Report - #)" << analysis.id << R"(</title>
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; background: #0f172a; color: #f8fafc; margin: 0; padding: 40px; }
        .container { max-width: 960px; margin: 0 auto; background: #1e293b; border-radius: 12px; border: 1px solid #334155; padding: 40px; box-shadow: 0 10px 25px rgba(0,0,0,0.5); }
        .header { display: flex; justify-content: space-between; align-items: center; border-bottom: 2px solid #334155; padding-bottom: 24px; margin-bottom: 32px; }
        .brand { font-size: 28px; font-weight: 800; color: #10b981; letter-spacing: -0.5px; }
        .tagline { font-size: 14px; color: #94a3b8; margin-top: 4px; }
        .badge { background: rgba(16, 185, 129, 0.15); color: #10b981; padding: 6px 14px; border-radius: 999px; font-weight: 600; font-size: 13px; border: 1px solid rgba(16, 185, 129, 0.3); }
        .grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 16px; margin-bottom: 32px; }
        .card { background: #0f172a; border: 1px solid #334155; border-radius: 8px; padding: 20px; }
        .card-label { font-size: 12px; text-transform: uppercase; color: #94a3b8; font-weight: 600; letter-spacing: 0.5px; }
        .card-value { font-size: 32px; font-weight: 700; margin-top: 8px; color: #f8fafc; }
        .card-value.emerald { color: #10b981; }
        .card-value.amber { color: #f59e0b; }
        .card-value.rose { color: #f43f5e; }
        .section-title { font-size: 20px; font-weight: 700; margin-bottom: 16px; border-left: 4px solid #10b981; padding-left: 12px; }
        .methodology { background: #0f172a; border: 1px solid #334155; border-radius: 8px; padding: 20px; margin-bottom: 32px; font-size: 14px; line-height: 1.6; color: #cbd5e1; }
        .match-card { background: #0f172a; border-left: 4px solid #f59e0b; border-radius: 4px; padding: 16px; margin-bottom: 16px; }
        .match-header { display: flex; justify-content: space-between; font-size: 13px; color: #94a3b8; margin-bottom: 8px; }
        .match-text { background: rgba(245, 158, 11, 0.1); color: #fef08a; padding: 12px; border-radius: 6px; font-family: monospace; font-size: 13px; line-height: 1.5; }
        .disclaimer { border-top: 1px solid #334155; margin-top: 40px; padding-top: 20px; font-size: 12px; color: #64748b; line-height: 1.6; }
        @media print {
            body { background: #ffffff; color: #000000; padding: 0; }
            .container { background: #ffffff; border: none; box-shadow: none; color: #000; }
            .card, .methodology, .match-card { background: #f8fafc; border: 1px solid #cbd5e1; color: #000; }
            .match-text { background: #fef9c3; color: #854d0e; }
        }
    </style>
</head>
<body>
<div class="container">
    <div class="header">
        <div>
            <div class="brand">PlagiGuard</div>
            <div class="tagline">Detect Similarity. Discover Originality.</div>
        </div>
        <div>
            <span class="badge">Official Academic Report</span>
        </div>
    </div>

    <div class="grid">)";

        double avgSim = analysis.getAverageSimilarity();
        std::string simColor = avgSim >= 70.0 ? "rose" : (avgSim >= 30.0 ? "amber" : "emerald");

        html << R"(
        <div class="card">
            <div class="card-label">Average Similarity</div>
            <div class="card-value )" << simColor << R"(">)" << std::fixed << std::setprecision(1) << avgSim << R"(%</div>
        </div>
        <div class="card">
            <div class="card-label">Documents Analyzed</div>
            <div class="card-value">)" << analysis.documentCount << R"(</div>
        </div>
        <div class="card">
            <div class="card-label">Matching Passages</div>
            <div class="card-value">)" << analysis.getTotalMatchingPassages() << R"(</div>
        </div>
        <div class="card">
            <div class="card-label">Execution Time</div>
            <div class="card-value">)" << analysis.processingTimeMs << R"(ms</div>
        </div>
    </div>

    <div class="section-title">Detection Methodology</div>
    <div class="methodology">
        This audit was performed using the <strong>PlagiGuard Advanced DSA Engine</strong>.
        Analysis employed a manual <strong>Custom Hash Table</strong> with separate chaining,
        <strong>Polynomial Double Rolling Hash</strong> (Base 31, Moduli 10^9+7 and 998,244,353),
        <strong>Rabin-Karp pattern matching</strong> with collision verification, and
        <strong>Jaccard Similarity / Directional Overlap</strong> shingle metrics.
    </div>

    <div class="section-title">Identified Similarities & Matching Passages</div>
)";

        for (const auto& res : analysis.results) {
            html << R"(
            <div style="margin-bottom: 24px;">
                <h3 style="color: #38bdf8; margin-bottom: 8px;">)"
                 << res.docAName << " vs " << res.docBName
                 << " &mdash; Jaccard: " << std::fixed << std::setprecision(1) << res.jaccardScore << "%"
                 << ", Overlap: " << res.directionalScoreAtoB << "%</h3>";

            if (res.matchingPassages.empty()) {
                html << "<p style='color: #94a3b8; font-style: italic;'>No matching passages exceeded the verification threshold.</p>";
            } else {
                for (size_t m = 0; m < res.matchingPassages.size() && m < 15; ++m) {
                    const auto& match = res.matchingPassages[m];
                    html << R"(
                    <div class="match-card">
                        <div class="match-header">
                            <span>Match #)" << (m + 1) << " (" << match.matchLength << R"( tokens)</span>
                            <span>Tokens [)" << match.sourceStartToken << " - " << match.sourceEndToken << R"(]</span>
                        </div>
                        <div class="match-text">")" << match.matchingText << R"("</div>
                    </div>)";
                }
            }
            html << "</div>";
        }

        html << R"(
    <div class="disclaimer">
        <strong>Academic Ethics & Misconduct Notice:</strong>
        This report reflects algorithmic textual similarity and shingle overlap.
        A positive similarity coefficient does <em>not</em> constitute confirmed academic plagiarism.
        Permissible overlap includes standard bibliographies, technical terminology, standardized prompts,
        and legitimately cited academic quotations. Human evaluation by faculty is strictly required.
    </div>
</div>
</body>
</html>)";

        return html.str();
    }

    static bool saveReport(const model::Analysis& analysis, const std::string& outputPath) {
        std::string content = generateHtmlReport(analysis);
        std::ofstream out(outputPath);
        if (!out.is_open()) return false;
        out << content;
        return true;
    }
};

} // namespace plagiguard::service

