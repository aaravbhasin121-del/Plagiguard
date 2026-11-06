package com.plagiguard.ui;

import com.formdev.flatlaf.FlatDarkLaf;
import com.formdev.flatlaf.FlatIntelliJLaf;
import com.formdev.flatlaf.FlatLightLaf;
import com.plagiguard.database.AnalysisRepository;
import com.plagiguard.database.DatabaseManager;
import com.plagiguard.model.Analysis;
import com.plagiguard.model.SimilarityResult;
import com.plagiguard.service.AnalysisService;
import com.plagiguard.service.DocumentService;
import com.plagiguard.service.ReportService;

import javax.swing.*;
import javax.swing.border.EmptyBorder;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;

/**
 * Main application window for PlagiGuard.
 *
 * Layout:
 *  ┌──────────────┬───────────────────────────────┐
 *  │  Sidebar     │  Content (CardLayout)          │
 *  │  220 px      │                               │
 *  └──────────────┴───────────────────────────────┘
 */
public class MainWindow extends JFrame {

    // ── Panel name constants ──────────────────────────────────────────────────
    public static final String PANEL_DASHBOARD = "DASHBOARD";
    public static final String PANEL_ANALYSIS  = "ANALYSIS";
    public static final String PANEL_LIBRARY   = "LIBRARY";
    public static final String PANEL_NETWORK   = "NETWORK";
    public static final String PANEL_HISTORY   = "HISTORY";
    public static final String PANEL_REPORTS   = "REPORTS";
    public static final String PANEL_SETTINGS  = "SETTINGS";
    public static final String PANEL_RESULTS   = "RESULTS";
    public static final String PANEL_COMPARE   = "COMPARE";

    // ── Services ──────────────────────────────────────────────────────────────
    private final DocumentService documentService;
    private final AnalysisService analysisService;
    private final ReportService   reportService;

    // ── Layout ────────────────────────────────────────────────────────────────
    private CardLayout cardLayout;
    private JPanel     contentPanel;
    private JPanel     sidebarPanel;

    // ── Child panels ──────────────────────────────────────────────────────────
    private DashboardPanel       dashboardPanel;
    private UploadPanel          uploadPanel;
    private DocumentLibraryPanel libraryPanel;
    private GraphPanel           graphPanel;
    private HistoryPanel         historyPanel;
    private ReportsPanel         reportsPanel;
    private SettingsPanel        settingsPanel;
    private AnalysisResultPanel  resultsPanel;
    private ComparisonPanel      comparePanel;
    private AnalysisPanel        analysisPanel;

    // ── State ─────────────────────────────────────────────────────────────────
    private String   currentPanel = PANEL_DASHBOARD;
    private JPanel   activeNavItem;
    private boolean  darkMode = true;

    // ── Nav item references (for active-highlight tracking) ──────────────────
    private JPanel navDashboard;
    private JPanel navAnalysis;
    private JPanel navLibrary;
    private JPanel navNetwork;
    private JPanel navHistory;
    private JPanel navReports;
    private JPanel navSettings;

    // =========================================================================
    public MainWindow(DocumentService documentService,
                      AnalysisService analysisService,
                      ReportService   reportService) {
        this.documentService = documentService;
        this.analysisService = analysisService;
        this.reportService   = reportService;
        initComponents();
        setupMenuBar();
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
    }

    // ── Init ──────────────────────────────────────────────────────────────────
    private void initComponents() {
        setTitle("PlagiGuard – Plagiarism Detection");
        setMinimumSize(new Dimension(1280, 800));
        setPreferredSize(new Dimension(1440, 900));

        JPanel root = new JPanel(new BorderLayout(0, 0));
        root.setBackground(UITheme.DARK_BG);
        setContentPane(root);

        // Sidebar
        sidebarPanel = buildSidebar();
        root.add(sidebarPanel, BorderLayout.WEST);

        // Content area
        cardLayout  = new CardLayout();
        contentPanel = new JPanel(cardLayout);
        contentPanel.setBackground(UITheme.DARK_BG);
        root.add(contentPanel, BorderLayout.CENTER);

        // Instantiate child panels
        dashboardPanel = new DashboardPanel(analysisService, documentService, this);
        uploadPanel    = new UploadPanel(documentService, analysisService, this);
        libraryPanel   = new DocumentLibraryPanel(documentService, analysisService, this);
        graphPanel     = new GraphPanel(analysisService, this);
        historyPanel   = new HistoryPanel(analysisService, reportService, this);
        reportsPanel   = new ReportsPanel(analysisService, reportService, this);

        DatabaseManager dbManager = DatabaseManager.getInstance();
        AnalysisRepository analysisRepo = new AnalysisRepository(dbManager);
        settingsPanel  = new SettingsPanel(analysisRepo, this);
        resultsPanel   = new AnalysisResultPanel(this, reportService, analysisService);
        comparePanel   = new ComparisonPanel(this);
        analysisPanel  = new AnalysisPanel(analysisService, this);

        // Register cards
        contentPanel.add(dashboardPanel, PANEL_DASHBOARD);
        contentPanel.add(uploadPanel,    PANEL_ANALYSIS);
        contentPanel.add(libraryPanel,   PANEL_LIBRARY);
        contentPanel.add(graphPanel,     PANEL_NETWORK);
        contentPanel.add(historyPanel,   PANEL_HISTORY);
        contentPanel.add(reportsPanel,   PANEL_REPORTS);
        contentPanel.add(settingsPanel,  PANEL_SETTINGS);
        contentPanel.add(resultsPanel,   PANEL_RESULTS);
        contentPanel.add(comparePanel,   PANEL_COMPARE);
        contentPanel.add(analysisPanel,  "RUNNING_ANALYSIS");

        // Show dashboard first
        cardLayout.show(contentPanel, PANEL_DASHBOARD);
        setActiveNav(navDashboard);

        pack();
        setLocationRelativeTo(null);
    }

    // ── Sidebar ───────────────────────────────────────────────────────────────
    private JPanel buildSidebar() {
        JPanel sidebar = new JPanel();
        sidebar.setLayout(new BoxLayout(sidebar, BoxLayout.Y_AXIS));
        sidebar.setBackground(UITheme.DARK_SIDEBAR);
        sidebar.setPreferredSize(new Dimension(220, Integer.MAX_VALUE));
        sidebar.setBorder(new EmptyBorder(0, 0, 0, 0));

        // Logo / brand
        JPanel brand = new JPanel(new FlowLayout(FlowLayout.LEFT, 20, 18));
        brand.setBackground(UITheme.DARK_SIDEBAR);
        brand.setMaximumSize(new Dimension(220, 64));
        JLabel logo = new JLabel("🛡 PlagiGuard");
        logo.setFont(new Font("Segoe UI", Font.BOLD, 16));
        logo.setForeground(UITheme.ACCENT);
        brand.add(logo);
        sidebar.add(brand);

        // Thin separator
        sidebar.add(createSeparator());

        // Nav items
        navDashboard = createNavItem("🏠  Dashboard",      PANEL_DASHBOARD);
        navAnalysis  = createNavItem("➕  New Analysis",   PANEL_ANALYSIS);
        navLibrary   = createNavItem("📁  Document Library", PANEL_LIBRARY);
        navNetwork   = createNavItem("🕸  Similarity Network", PANEL_NETWORK);
        navHistory   = createNavItem("🕐  Analysis History",   PANEL_HISTORY);
        navReports   = createNavItem("📄  Reports",          PANEL_REPORTS);

        sidebar.add(navDashboard);
        sidebar.add(navAnalysis);
        sidebar.add(navLibrary);
        sidebar.add(navNetwork);
        sidebar.add(navHistory);
        sidebar.add(navReports);

        // Spacer
        sidebar.add(Box.createVerticalGlue());
        sidebar.add(createSeparator());

        navSettings = createNavItem("⚙  Settings", PANEL_SETTINGS);
        sidebar.add(navSettings);
        sidebar.add(Box.createVerticalStrut(12));

        return sidebar;
    }

    /** Creates one sidebar navigation button. */
    private JPanel createNavItem(String label, String panelName) {
        JPanel item = new JPanel(new FlowLayout(FlowLayout.LEFT, 20, 12));
        item.setBackground(UITheme.DARK_SIDEBAR);
        item.setMaximumSize(new Dimension(220, 48));
        item.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));

        JLabel lbl = new JLabel(label);
        lbl.setFont(UITheme.FONT_BODY);
        lbl.setForeground(UITheme.DARK_SUBTEXT);
        item.add(lbl);

        // Hover effect
        item.addMouseListener(new MouseAdapter() {
            @Override public void mouseEntered(MouseEvent e) {
                if (item != activeNavItem) {
                    item.setBackground(UITheme.DARK_NAV_HOVER);
                }
            }
            @Override public void mouseExited(MouseEvent e) {
                if (item != activeNavItem) {
                    item.setBackground(UITheme.DARK_SIDEBAR);
                }
            }
            @Override public void mouseClicked(MouseEvent e) {
                selectPanel(panelName);
            }
        });
        return item;
    }

    /** Highlight the selected nav item and show the matching card. */
    private void selectPanel(String panelName) {
        JPanel target = navForPanel(panelName);
        setActiveNav(target);
        cardLayout.show(contentPanel, panelName);
        currentPanel = panelName;

        // Refresh data when switching panels
        switch (panelName) {
            case PANEL_DASHBOARD -> dashboardPanel.refresh();
            case PANEL_LIBRARY   -> libraryPanel.refresh();
            case PANEL_HISTORY   -> historyPanel.refresh();
            case PANEL_REPORTS   -> reportsPanel.refresh();
            case PANEL_NETWORK   -> graphPanel.refresh();
        }
    }

    private JPanel navForPanel(String panel) {
        return switch (panel) {
            case PANEL_DASHBOARD -> navDashboard;
            case PANEL_ANALYSIS  -> navAnalysis;
            case PANEL_LIBRARY   -> navLibrary;
            case PANEL_NETWORK   -> navNetwork;
            case PANEL_HISTORY   -> navHistory;
            case PANEL_REPORTS   -> navReports;
            case PANEL_SETTINGS  -> navSettings;
            default              -> null;
        };
    }

    private void setActiveNav(JPanel item) {
        // Reset previous
        if (activeNavItem != null) {
            activeNavItem.setBackground(UITheme.DARK_SIDEBAR);
            for (Component c : activeNavItem.getComponents()) {
                if (c instanceof JLabel l) l.setForeground(UITheme.DARK_SUBTEXT);
            }
        }
        activeNavItem = item;
        if (item == null) return;
        item.setBackground(UITheme.DARK_NAV_ACTIVE);
        for (Component c : item.getComponents()) {
            if (c instanceof JLabel l) l.setForeground(UITheme.ACCENT);
        }
    }

    private Component createSeparator() {
        JPanel sep = new JPanel();
        sep.setBackground(UITheme.DARK_BORDER);
        sep.setPreferredSize(new Dimension(220, 1));
        sep.setMaximumSize(new Dimension(220, 1));
        return sep;
    }

    // ── Menu bar ──────────────────────────────────────────────────────────────
    private void setupMenuBar() {
        JMenuBar menuBar = new JMenuBar();
        menuBar.setBackground(UITheme.DARK_SURFACE);

        // File menu
        JMenu fileMenu = new JMenu("File");
        fileMenu.setForeground(UITheme.DARK_TEXT);
        JMenuItem newAnalysis = new JMenuItem("New Analysis");
        newAnalysis.addActionListener(e -> selectPanel(PANEL_ANALYSIS));
        JMenuItem exit = new JMenuItem("Exit");
        exit.addActionListener(e -> System.exit(0));
        fileMenu.add(newAnalysis);
        fileMenu.addSeparator();
        fileMenu.add(exit);
        menuBar.add(fileMenu);

        // View menu
        JMenu viewMenu = new JMenu("View");
        viewMenu.setForeground(UITheme.DARK_TEXT);
        JMenuItem toggleTheme = new JMenuItem("Toggle Light/Dark Theme");
        toggleTheme.addActionListener(e -> handleThemeToggle());
        viewMenu.add(toggleTheme);
        menuBar.add(viewMenu);

        // Help menu
        JMenu helpMenu = new JMenu("Help");
        helpMenu.setForeground(UITheme.DARK_TEXT);
        JMenuItem about = new JMenuItem("About PlagiGuard");
        about.addActionListener(e -> JOptionPane.showMessageDialog(this,
                "PlagiGuard v1.0\nA DSA-powered plagiarism detection tool.\n\n"
              + "Algorithms: Rabin-Karp rolling hash, N-gram shingling,\n"
              + "Jaccard similarity, BFS/DFS graph traversal.",
                "About PlagiGuard", JOptionPane.INFORMATION_MESSAGE));
        helpMenu.add(about);
        menuBar.add(helpMenu);

        setJMenuBar(menuBar);
    }

    private void handleThemeToggle() {
        try {
            darkMode = !darkMode;
            if (darkMode) {
                UIManager.setLookAndFeel(new FlatDarkLaf());
            } else {
                UIManager.setLookAndFeel(new FlatIntelliJLaf());
            }
            SwingUtilities.updateComponentTreeUI(this);
        } catch (Exception ex) {
            JOptionPane.showMessageDialog(this, "Theme change failed: " + ex.getMessage(),
                    "Theme Error", JOptionPane.ERROR_MESSAGE);
        }
    }

    // ── Public navigation API ─────────────────────────────────────────────────
    /** Refresh all panels (e.g. after a new analysis completes). */
    public void refreshAllPanels() {
        SwingUtilities.invokeLater(() -> {
            dashboardPanel.refresh();
            libraryPanel.refresh();
            historyPanel.refresh();
            reportsPanel.refresh();
            graphPanel.refresh();
        });
    }

    /** Navigate to a specific panel programmatically. */
    public void navigateTo(String panelName) {
        SwingUtilities.invokeLater(() -> selectPanel(panelName));
    }

    /** Show the results panel for a completed analysis. */
    public void showAnalysisResults(Analysis analysis) {
        SwingUtilities.invokeLater(() -> {
            resultsPanel.loadAnalysis(analysis);
            cardLayout.show(contentPanel, PANEL_RESULTS);
            currentPanel = PANEL_RESULTS;
            setActiveNav(null);
            refreshAllPanels();
        });
    }

    /** Show side-by-side comparison panel. */
    public void showComparison(SimilarityResult result) {
        SwingUtilities.invokeLater(() -> {
            comparePanel.loadResult(result);
            cardLayout.show(contentPanel, PANEL_COMPARE);
            currentPanel = PANEL_COMPARE;
            setActiveNav(null);
        });
    }

    /** Show the live analysis progress panel. */
    public void showRunningAnalysis() {
        SwingUtilities.invokeLater(() -> {
            cardLayout.show(contentPanel, "RUNNING_ANALYSIS");
            currentPanel = "RUNNING_ANALYSIS";
            setActiveNav(null);
        });
    }

    // ── Getters used by child panels ─────────────────────────────────────────
    public AnalysisPanel getAnalysisPanel() { return analysisPanel; }
}
