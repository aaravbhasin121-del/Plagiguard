package com.plagiguard;

import com.formdev.flatlaf.FlatDarkLaf;
import com.formdev.flatlaf.FlatLightLaf;
import com.plagiguard.database.DatabaseManager;
import com.plagiguard.service.AnalysisService;
import com.plagiguard.service.DocumentService;
import com.plagiguard.service.ReportService;
import com.plagiguard.ui.MainWindow;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.swing.*;
import java.awt.*;
import java.sql.SQLException;

/**
 * PlagiGuard - Intelligent Plagiarism Detection & Academic Document Analysis System
 *
 * Entry point for the application.
 *
 * Startup sequence:
 * 1. Apply FlatLaf theme
 * 2. Initialize database
 * 3. Bootstrap services
 * 4. Launch main window on EDT
 *
 * @version 1.0.0
 */
public class Main {

    private static final Logger logger = LoggerFactory.getLogger(Main.class);

    public static void main(String[] args) {
        // Configure system properties for better rendering on macOS/HiDPI
        System.setProperty("apple.laf.useScreenMenuBar", "true");
        System.setProperty("apple.awt.application.name", "PlagiGuard");
        System.setProperty("flatlaf.animation", "true");

        // Determine theme from args or default to dark
        boolean useDarkTheme = true;
        for (String arg : args) {
            if ("--light".equalsIgnoreCase(arg)) {
                useDarkTheme = false;
            }
        }

        final boolean darkTheme = useDarkTheme;

        // Launch on the Event Dispatch Thread
        SwingUtilities.invokeLater(() -> {
            try {
                // Apply FlatLaf theme
                if (darkTheme) {
                    FlatDarkLaf.setup();
                } else {
                    FlatLightLaf.setup();
                }

                // Customize FlatLaf properties
                UIManager.put("Component.arc", 8);
                UIManager.put("Button.arc", 8);
                UIManager.put("TextComponent.arc", 6);
                UIManager.put("ScrollBar.width", 8);
                UIManager.put("ScrollBar.thumbArc", 999);
                UIManager.put("ScrollBar.thumbInsets", new Insets(2, 2, 2, 2));
                UIManager.put("TabbedPane.showTabSeparators", true);
                UIManager.put("Table.showHorizontalLines", true);
                UIManager.put("Table.showVerticalLines", false);

                // Set accent color to emerald green
                UIManager.put("Component.focusColor", new Color(16, 185, 129));
                UIManager.put("Button.default.background", new Color(16, 185, 129));
                UIManager.put("Button.default.foreground", Color.WHITE);
                UIManager.put("CheckBox.icon.selectedBackground", new Color(16, 185, 129));
                UIManager.put("Slider.thumbColor", new Color(16, 185, 129));

                logger.info("PlagiGuard starting up...");
                logger.info("Theme: {}", darkTheme ? "Dark" : "Light");

                // Initialize database
                DatabaseManager dbManager = DatabaseManager.getInstance();
                try {
                    dbManager.initialize();
                    logger.info("Database initialized successfully.");
                } catch (SQLException e) {
                    logger.error("Failed to initialize database", e);
                    JOptionPane.showMessageDialog(
                            null,
                            "Failed to initialize database:\n" + e.getMessage() +
                            "\n\nThe application will attempt to continue with limited functionality.",
                            "Database Error",
                            JOptionPane.ERROR_MESSAGE
                    );
                }

                // Bootstrap services
                DocumentService documentService = new DocumentService(dbManager);
                AnalysisService analysisService = new AnalysisService(dbManager);
                ReportService reportService = new ReportService();

                // Rebuild index from existing database documents
                try {
                    documentService.rebuildIndex();
                    logger.info("Document index rebuilt.");
                } catch (Exception e) {
                    logger.warn("Could not rebuild document index: {}", e.getMessage());
                }

                // Create and show main window
                MainWindow mainWindow = new MainWindow(documentService, analysisService, reportService);
                mainWindow.setVisible(true);

                logger.info("PlagiGuard launched successfully.");

                // Register shutdown hook
                Runtime.getRuntime().addShutdownHook(new Thread(() -> {
                    logger.info("PlagiGuard shutting down...");
                    try {
                        dbManager.close();
                        logger.info("Database connection closed.");
                    } catch (Exception e) {
                        logger.error("Error during shutdown", e);
                    }
                }, "shutdown-hook"));

            } catch (Exception e) {
                logger.error("Fatal error during startup", e);
                JOptionPane.showMessageDialog(
                        null,
                        "PlagiGuard failed to start:\n" + e.getMessage(),
                        "Startup Error",
                        JOptionPane.ERROR_MESSAGE
                );
                System.exit(1);
            }
        });
    }
}

