package com.plagiguard.ui;

import java.awt.*;

/**
 * Shared colour/font constants for all PlagiGuard UI panels.
 */
public final class UITheme {

    private UITheme() {}

    // ── Primary accent ──────────────────────────────────────────────────────
    public static final Color ACCENT       = new Color(16, 185, 129);   // emerald-500
    public static final Color ACCENT_DARK  = new Color(5,  150, 105);   // emerald-600
    public static final Color ACCENT_LIGHT = new Color(52, 211, 153);   // emerald-400

    // ── Semantic colours ─────────────────────────────────────────────────────
    public static final Color DANGER   = new Color(239,  68,  68);  // red-500
    public static final Color WARNING  = new Color(245, 158,  11);  // amber-500
    public static final Color SUCCESS  = ACCENT;
    public static final Color INFO     = new Color( 59, 130, 246);  // blue-500

    // ── Dark-mode palette ────────────────────────────────────────────────────
    public static final Color DARK_BG         = new Color( 15,  23,  42);  // slate-900
    public static final Color DARK_SURFACE     = new Color( 30,  41,  59);  // slate-800
    public static final Color DARK_SURFACE2    = new Color( 51,  65,  85);  // slate-700
    public static final Color DARK_BORDER      = new Color( 71,  85, 105);  // slate-600
    public static final Color DARK_TEXT        = new Color(248, 250, 252);  // slate-50
    public static final Color DARK_SUBTEXT     = new Color(148, 163, 184);  // slate-400
    public static final Color DARK_SIDEBAR     = new Color( 15,  23,  42);
    public static final Color DARK_NAV_HOVER   = new Color( 30,  41,  59);
    public static final Color DARK_NAV_ACTIVE  = new Color(  5,  46,  22);  // green-950 tint

    // ── Light-mode palette ───────────────────────────────────────────────────
    public static final Color LIGHT_BG         = new Color(248, 250, 252);  // slate-50
    public static final Color LIGHT_SURFACE     = Color.WHITE;
    public static final Color LIGHT_SURFACE2    = new Color(241, 245, 249);  // slate-100
    public static final Color LIGHT_BORDER      = new Color(226, 232, 240);  // slate-200
    public static final Color LIGHT_TEXT        = new Color( 15,  23,  42);  // slate-900
    public static final Color LIGHT_SUBTEXT     = new Color(100, 116, 139);  // slate-500
    public static final Color LIGHT_SIDEBAR     = new Color( 15,  23,  42);  // sidebar stays dark
    public static final Color LIGHT_NAV_HOVER   = new Color( 30,  41,  59);
    public static final Color LIGHT_NAV_ACTIVE  = new Color(  5,  46,  22);

    // ── Typography ───────────────────────────────────────────────────────────
    public static final Font FONT_TITLE  = new Font("Segoe UI", Font.BOLD,  22);
    public static final Font FONT_H2     = new Font("Segoe UI", Font.BOLD,  16);
    public static final Font FONT_H3     = new Font("Segoe UI", Font.BOLD,  13);
    public static final Font FONT_BODY   = new Font("Segoe UI", Font.PLAIN, 13);
    public static final Font FONT_SMALL  = new Font("Segoe UI", Font.PLAIN, 11);
    public static final Font FONT_MONO   = new Font("JetBrains Mono", Font.PLAIN, 12);
    public static final Font FONT_BIG    = new Font("Segoe UI", Font.BOLD,  36);

    // ── Corner radii / spacing ───────────────────────────────────────────────
    public static final int  RADIUS_CARD    = 12;
    public static final int  RADIUS_BUTTON  =  8;
    public static final int  GAP            = 12;
    public static final int  PAD            = 20;

    // ── Similarity badge colours (Jaccard score) ─────────────────────────────
    public static Color similarityColor(double pct) {
        if (pct >= 70) return DANGER;
        if (pct >= 40) return WARNING;
        if (pct >= 15) return new Color(234, 179,   8);  // yellow-500
        return SUCCESS;
    }
}
