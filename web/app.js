// ==========================================================================
// PlagiGuard — Frontend Application Controller
// C++20 Native Plagiarism Detection & Document Integrity Platform
// ==========================================================================

let currentAnalysisData = null;
let networkGraphData = null;
let documentsList = [];
let animationInterval = null;
let activeModalDocId = null;
let duplicatePendingData = null;

// Graph & Heap Global Engine State
let graphSimulator = null;
let graphParticles = [];
let graphAnimFrameId = null;
let selectedGraphNode = null;
let hoveredGraphNode = null;
let hoveredGraphEdge = null;
let draggedGraphNode = null;
let isDraggingNode = false;
let activeGraphLayout = 'physics'; // 'physics' | 'circle'
let showMSTOnly = false;
let mstEdgeKeys = new Set();
let heapRankedEntries = [];
let activeTraversal = {
    running: false,
    paused: false,
    type: 'BFS',
    structName: 'FIFO QUEUE',
    sequence: [],
    stepIdx: 0,
    visitedNodes: new Set(),
    traversedEdges: new Set(),
    activeNode: null,
    queueOrStack: [],
    timer: null,
    stepDelay: 480
};

document.addEventListener('DOMContentLoaded', () => {
    initPreloader();
    initNavigation();
    initTheme();
    initSliders();
    initDropZone();
    initComparisonModes();
    initEngineHealthCheck();
    initLivePlayground();
    initScrollytelling();
    initScrollTracker();
    initCanvasInteractions();
    init3DCardParallax();
    initScrollReveal();

    window.addEventListener('resize', () => {
        if (graphSimulator) graphSimulator.resize();
    });

    loadDashboard();
    loadDocuments();
});

// ==========================================================================
// 1. ENGINE HEALTH CHECK (C++ STATUS HEARTBEAT)
// ==========================================================================
async function initEngineHealthCheck() {
    checkEngineStatus();
    setInterval(checkEngineStatus, 6000);
}

async function checkEngineStatus() {
    const dot = document.getElementById('engineStatusDot');
    const text = document.getElementById('engineStatusText');
    try {
        const res = await fetch('/api/status');
        if (res.ok) {
            const data = await res.json();
            if (dot) {
                dot.style.backgroundColor = '#F59E0B';
                dot.style.boxShadow = '0 0 8px #F59E0B';
            }
            if (text) {
                text.textContent = `C++20 Engine Active (${data.version || 'v2.4'})`;
            }
        } else {
            throw new Error('HTTP error');
        }
    } catch (e) {
        if (dot) {
            dot.style.backgroundColor = '#EF4444';
            dot.style.boxShadow = '0 0 8px #EF4444';
        }
        if (text) {
            text.textContent = 'Engine Offline (Reconnecting...)';
        }
    }
}

// ==========================================================================
// 2. NAVIGATION & TABS
// ==========================================================================
function initNavigation() {
    document.querySelectorAll('.nav-item').forEach(btn => {
        btn.addEventListener('click', () => {
            const tab = btn.getAttribute('data-tab');
            switchTab(tab);
        });
    });
}


// ==========================================================================
// WORKFLOW STEPPER CONTROLLER
// ==========================================================================
let currentWorkflowStep = 1;

function updateWorkflowStep(stepNum) {
    currentWorkflowStep = stepNum;
    for (let i = 1; i <= 4; i++) {
        const item = document.getElementById("stepIndicator" + i);
        const line = document.getElementById("stepLine" + (i - 1));
        if (!item) continue;

        item.classList.remove('active', 'completed');
        const circle = item.querySelector('.stepper-circle');

        if (i < stepNum) {
            item.classList.add('completed');
            if (circle) circle.innerHTML = '✓';
            if (line) line.classList.add('active');
        } else if (i === stepNum) {
            item.classList.add('active');
            if (circle) circle.innerHTML = String(i);
            if (line) line.classList.remove('active');
        } else {
            if (circle) circle.innerHTML = String(i);
            if (line) line.classList.remove('active');
        }
    }
}

function switchTab(tabId) {
    document.querySelectorAll('.nav-item').forEach(b => b.classList.remove('active'));
    document.querySelectorAll('.view-panel').forEach(p => p.classList.remove('active'));

    const targetNav = document.querySelector(`.nav-item[data-tab="${tabId}"]`);
    if (targetNav) targetNav.classList.add('active');

    const targetPanel = document.getElementById(`view-${tabId}`);
    if (targetPanel) targetPanel.classList.add('active');

    const titleEl = document.getElementById('currentSectionTitle');
    if (titleEl) {
        titleEl.textContent = tabId.replace('-', ' ').toUpperCase();
    }

    if (tabId === 'dashboard') loadDashboard();
    if (tabId === 'new-analysis') {
        populateAnalysisSelectors();
        updatePreflightPreview();
        updateWorkflowStep(1);
    }
    if (tabId === 'library') loadDocumentsTable();
    if (tabId === 'history') loadHistoryTable();
    if (tabId === 'network') {
        loadNetworkGraph(0);
        setTimeout(() => { if (graphSimulator) graphSimulator.resize(); }, 120);
    }
    if (tabId === 'version-diff') populateDiffSelectors();

    setTimeout(() => {
        init3DCardParallax();
        initScrollReveal();
    }, 150);
}

// ==========================================================================
// 3D INTERACTIVE CARD PARALLAX & SPECULAR GLARE ENGINE
// ==========================================================================
function init3DCardParallax() {
    const selector = '.card, .stat-card, .manifesto-hero, .flow-card, .problem-card, .doc-card, .tel-stat-card, .dsa-metric-card, .playground-box, .narrative-beat, .interactive-playground-card';
    const targets = document.querySelectorAll(selector);

    targets.forEach(card => {
        if (!card.querySelector('.card-3d-glare')) {
            const glare = document.createElement('div');
            glare.className = 'card-3d-glare';
            card.appendChild(glare);
        }

        card.onmousemove = (e) => {
            const rect = card.getBoundingClientRect();
            const x = e.clientX - rect.left;
            const y = e.clientY - rect.top;
            const centerX = rect.width / 2;
            const centerY = rect.height / 2;

            const rotX = ((y - centerY) / centerY) * -6.5;
            const rotY = ((x - centerX) / centerX) * 6.5;

            card.style.transform = `perspective(1000px) rotateX(${rotX.toFixed(2)}deg) rotateY(${rotY.toFixed(2)}deg) translateZ(8px) scale3d(1.015, 1.015, 1.015)`;
            card.style.setProperty('--mouse-x', `${(x / rect.width * 100).toFixed(1)}%`);
            card.style.setProperty('--mouse-y', `${(y / rect.height * 100).toFixed(1)}%`);
        };

        card.onmouseleave = () => {
            card.style.transform = '';
            card.style.setProperty('--mouse-x', '50%');
            card.style.setProperty('--mouse-y', '50%');
        };
    });
}

// ==========================================================================
// 3. THEME
// ==========================================================================
function initTheme() {
    const toggle = document.getElementById('themeToggleBtn');
    if (toggle) {
        toggle.addEventListener('click', () => {
            const current = document.documentElement.getAttribute('data-theme') || 'dark';
            const next = current === 'dark' ? 'light' : 'dark';
            changeTheme(next);
        });
    }
}

function changeTheme(theme) {
    document.documentElement.setAttribute('data-theme', theme);
    const sel = document.getElementById('settingsThemeSelect');
    if (sel) sel.value = theme;
}

// ==========================================================================
// 4. SLIDERS & SENSITIVITY
// ==========================================================================
function initSliders() {
    const slider = document.getElementById('ngramSlider');
    const display = document.getElementById('ngramDisplay');
    const helpText = document.getElementById('ngramSensitivityHelp');

    if (slider && display) {
        slider.addEventListener('input', (e) => {
            const val = parseInt(e.target.value);
            display.textContent = `${val}-grams`;

            if (helpText) {
                if (val <= 4) {
                    helpText.textContent = `High Sensitivity (${val}-grams): Detects short shared idioms and phrases. May increase incidental overlap.`;
                } else if (val <= 6) {
                    helpText.textContent = `Balanced Academic (${val}-grams): Standard academic sliding window. Balanced precision and verbatim detection.`;
                } else {
                    helpText.textContent = `Verbatim Matching (${val}-grams): Targets long verbatim paragraph duplication. Minimizes incidental matches.`;
                }
            }
            updatePreflightPreview();
        });
    }

    const gSlider = document.getElementById('graphThresholdSlider');
    const gDisplay = document.getElementById('graphThresholdVal');
    if (gSlider && gDisplay) {
        gSlider.addEventListener('input', (e) => {
            gDisplay.textContent = e.target.value + '%';
            loadNetworkGraph(parseFloat(e.target.value));
        });
    }
}

// ==========================================================================
// 5. COMPARISON MODES & SELECTORS
// ==========================================================================
function initComparisonModes() {
    const radioInputs = document.querySelectorAll('input[name="cmpMode"]');
    radioInputs.forEach(radio => {
        radio.addEventListener('change', () => {
            document.querySelectorAll('.radio-card').forEach(c => c.classList.remove('active'));
            radio.closest('.radio-card')?.classList.add('active');

            const mode = radio.value;
            const twoDocSel = document.getElementById('twoDocSelectors');
            const batchBox = document.getElementById('batchPreviewBox');

            if (mode === 'two-doc') {
                if (twoDocSel) twoDocSel.style.display = 'block';
                if (batchBox) batchBox.style.display = 'none';
            } else if (mode === 'batch') {
                if (twoDocSel) twoDocSel.style.display = 'none';
                if (batchBox) batchBox.style.display = 'block';
            } else if (mode === 'version') {
                if (twoDocSel) twoDocSel.style.display = 'block';
                if (batchBox) batchBox.style.display = 'none';
            }
            updatePreflightPreview();
        });
    });

    const selA = document.getElementById('selectDocA');
    const selB = document.getElementById('selectDocB');
    if (selA) selA.addEventListener('change', updatePreflightPreview);
    if (selB) selB.addEventListener('change', updatePreflightPreview);
}

function swapSelectedDocs(event) {
    if (event) event.stopPropagation();
    const selA = document.getElementById('selectDocA');
    const selB = document.getElementById('selectDocB');
    if (!selA || !selB) return;
    const temp = selA.value;
    selA.value = selB.value;
    selB.value = temp;

    const swapBtn = document.getElementById('btnSwapDocs');
    if (swapBtn) {
        swapBtn.style.transform = 'scale(1.25) rotate(180deg)';
        setTimeout(() => { swapBtn.style.transform = ''; }, 350);
    }

    updateDocPickerCards();
    updatePreflightPreview();
    showToast('Swapped Document A and Document B', 'info');
}

function updatePreflightPreview() {
    const mode = document.querySelector('input[name="cmpMode"]:checked')?.value || 'two-doc';
    const windowSize = document.getElementById('ngramSlider')?.value || '5';
    const preMode = document.getElementById('preflightMode');
    const prePairs = document.getElementById('preflightPairs');
    const preWindow = document.getElementById('preflightWindow');
    const btnStart = document.getElementById('btnStartAnalysis');

    if (preWindow) preWindow.textContent = `${windowSize}-word N-grams`;

    if (mode === 'two-doc') {
        if (preMode) preMode.textContent = 'Two-Document Pair';
        const selA = document.getElementById('selectDocA');
        const selB = document.getElementById('selectDocB');
        const nameA = selA?.options[selA.selectedIndex]?.text.split(' (')[0] || 'Doc A';
        const nameB = selB?.options[selB.selectedIndex]?.text.split(' (')[0] || 'Doc B';

        if (selA && selB && selA.value && selB.value && selA.value !== selB.value) {
            if (prePairs) prePairs.textContent = `${nameA} vs ${nameB}`;
            if (btnStart) {
                btnStart.disabled = false;
                btnStart.style.opacity = '1';
                btnStart.style.cursor = 'pointer';
            }
        } else {
            if (prePairs) prePairs.textContent = 'Select two distinct documents';
            if (btnStart) {
                btnStart.disabled = true;
                btnStart.style.opacity = '0.5';
                btnStart.style.cursor = 'not-allowed';
            }
        }
    } else if (mode === 'batch') {
        if (preMode) preMode.textContent = 'Batch Collection Analysis';
        const n = documentsList.length;
        const totalPairs = (n * (n - 1)) / 2;
        if (prePairs) prePairs.textContent = `${totalPairs} Comparisons (${n} Documents)`;

        const calcLabel = document.getElementById('batchCalcLabel');
        const countLabel = document.getElementById('batchComparisonsCount');
        if (calcLabel) calcLabel.textContent = `Collection: ${n} indexed documents`;
        if (countLabel) countLabel.textContent = `Expected comparisons: ${totalPairs} unique pairs ($N \\times (N-1)/2$). Ranked via Max-Heap.`;

        if (btnStart) {
            btnStart.disabled = (n < 2);
            btnStart.style.opacity = (n < 2) ? '0.5' : '1';
            btnStart.style.cursor = (n < 2) ? 'not-allowed' : 'pointer';
        }
    } else if (mode === 'version') {
        if (preMode) preMode.textContent = 'Version Comparison (LCS)';
        if (prePairs) prePairs.textContent = 'Revision Alignment Diff';
        if (btnStart) {
            btnStart.disabled = false;
            btnStart.style.opacity = '1';
            btnStart.style.cursor = 'pointer';
        }
    }
}

// ==========================================================================
// 6. DROP ZONE & MULTI-DOCUMENT UPLOAD
// ==========================================================================
function initDropZone() {
    const dropZone = document.getElementById('dropZone');
    const fileInput = document.getElementById('fileInput');

    if (dropZone && fileInput) {
        dropZone.addEventListener('dragover', (e) => {
            e.preventDefault();
            dropZone.classList.add('drag-active');
        });
        dropZone.addEventListener('dragleave', (e) => {
            if (!dropZone.contains(e.relatedTarget)) {
                dropZone.classList.remove('drag-active');
            }
        });
        dropZone.addEventListener('drop', (e) => {
            e.preventDefault();
            dropZone.classList.remove('drag-active');
            if (e.dataTransfer.files.length > 0) {
                handleFiles(e.dataTransfer.files);
            }
        });
        fileInput.addEventListener('change', (e) => {
            if (e.target.files.length > 0) {
                handleFiles(e.target.files);
            }
        });
    }
}

async function handleFiles(files) {
    const list = document.getElementById('selectedFilesList');
    const fileArray = Array.from(files);
    const fileInput = document.getElementById('fileInput');
    if (fileInput) fileInput.value = '';

    const newlyUploadedIds = [];
    const countLabel = document.getElementById('queueCountLabel');

    for (let f of fileArray) {
        const item = document.createElement('div');
        item.style.padding = '10px 14px';
        item.style.background = 'var(--bg-surface-elevated)';
        item.style.borderRadius = 'var(--radius-sm)';
        item.style.fontSize = '13px';
        item.style.display = 'flex';
        item.style.justifyContent = 'space-between';
        item.style.alignItems = 'center';
        item.style.border = '1px solid var(--border-subtle)';

        const ext = f.name.split('.').pop().toUpperCase();
        let icon = '📄';
        if (ext === 'PDF') icon = '📕';
        else if (ext === 'DOCX') icon = '📘';

        item.innerHTML = `
            <div style="display:flex; align-items:center; gap:10px;">
                <span style="font-size:16px;">${icon}</span>
                <div>
                    <strong style="color:var(--text-primary);">${f.name}</strong>
                    <div style="font-size:11px; color:var(--text-muted);">${(f.size/1024).toFixed(1)} KB • ${ext}</div>
                </div>
            </div>
            <span style="color:var(--accent-amber); font-weight:600; font-size:12px;">
                ⏳ Processing & Neural OCR...
            </span>
        `;
        list.appendChild(item);

        try {
            const dataUrl = await new Promise((resolve, reject) => {
                const reader = new FileReader();
                reader.onload = () => resolve(reader.result);
                reader.onerror = () => reject(new Error('Failed to read file data'));
                reader.readAsDataURL(f);
            });

            const res = await fetch('/api/upload', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ filename: f.name, data: dataUrl })
            });
            const resData = await res.json();

            if (res.ok && resData.status === 'success') {
                item.querySelector('span:last-child').outerHTML = `
                    <span style="color:#F59E0B; font-weight:600; font-size:12px;">
                        ✓ Ingested (#${resData.id})
                    </span>
                `;
                newlyUploadedIds.push(resData.id);
                showToast(`Indexed "${f.name}" into C++ DSA engine`, 'success');
            } else if (resData.status === 'duplicate') {
                item.querySelector('span:last-child').outerHTML = `
                    <button class="btn btn-secondary btn-sm" onclick="showDuplicateModal('${escapeQuotes(f.name)}', '${escapeQuotes(resData.duplicateOf)}', '${resData.fingerprint}', ${resData.existingId})">
                        ⚠ Duplicate Details
                    </button>
                `;
                newlyUploadedIds.push(resData.existingId);
                showToast(`Duplicate detected: "${f.name}" matches "${resData.duplicateOf}"`, 'warning');
            } else {
                item.querySelector('span:last-child').outerHTML = `
                    <span style="color:#ef4444; font-weight:600; font-size:12px;">
                        ✗ ${resData.message || 'Error'}
                    </span>
                `;
                showToast(`Failed to parse "${f.name}": ${resData.message || 'Unknown error'}`, 'error');
            }
        } catch (err) {
            item.querySelector('span:last-child').outerHTML = `
                <span style="color:#ef4444; font-weight:600; font-size:12px;">
                    ✗ Error: ${err.message}
                </span>
            `;
        }
    }

    await loadDocuments();
    if (countLabel) countLabel.textContent = `${list.children.length} in session`;

    if (newlyUploadedIds.length >= 2) {
        populateAnalysisSelectors(newlyUploadedIds[newlyUploadedIds.length - 2], newlyUploadedIds[newlyUploadedIds.length - 1]);
    } else if (newlyUploadedIds.length === 1) {
        populateAnalysisSelectors(newlyUploadedIds[0]);
    } else {
        populateAnalysisSelectors();
    }
    updatePreflightPreview();
}

function escapeQuotes(str) {
    if (!str) return '';
    return str.replace(/'/g, "\\'").replace(/"/g, '&quot;');
}

function showDuplicateModal(uploadedName, existingName, fingerprint, existingId) {
    duplicatePendingData = { existingId, existingName };
    document.getElementById('dupUploadedName').textContent = uploadedName;
    document.getElementById('dupExistingName').textContent = existingName;
    document.getElementById('dupFingerprint').textContent = fingerprint || '128-bit Content Digest';
    document.getElementById('duplicateWarningModal').classList.add('active');
}

function closeDuplicateModal() {
    document.getElementById('duplicateWarningModal').classList.remove('active');
}

function useExistingDocForAnalysis() {
    closeDuplicateModal();
    if (duplicatePendingData && duplicatePendingData.existingId) {
        populateAnalysisSelectors(duplicatePendingData.existingId);
        updatePreflightPreview();
    }
}

function populateAnalysisSelectors(preferredA = null, preferredB = null) {
    const selA = document.getElementById('selectDocA');
    const selB = document.getElementById('selectDocB');
    if (!selA || !selB) return;

    const valA = preferredA || selA.value;
    const valB = preferredB || selB.value;

    selA.innerHTML = '';
    selB.innerHTML = '';

    documentsList.forEach((d) => {
        const optA = new Option(`${d.name} (${d.words} words)`, d.id);
        const optB = new Option(`${d.name} (${d.words} words)`, d.id);
        selA.add(optA);
        selB.add(optB);
    });

    if (valA && Array.from(selA.options).some(o => o.value == valA)) {
        selA.value = valA;
    } else if (documentsList.length > 0) {
        selA.value = documentsList[0].id;
    }

    if (valB && Array.from(selB.options).some(o => o.value == valB)) {
        selB.value = valB;
    } else if (documentsList.length > 1) {
        selB.value = documentsList[1].id;
    }

    updateDocPickerCards();
}

// ==========================================================================
// FAST ZERO-LAG DOCUMENT PICKER COMPONENT
// ==========================================================================
let currentPickerTarget = 'A'; // 'A' or 'B'
let currentPickerFilterType = 'ALL';
let currentPickerSearchQuery = '';

function openDocPicker(target) {
    currentPickerTarget = target;
    const modal = document.getElementById('docPickerModal');
    const titleEl = document.getElementById('docPickerTitle');
    const subtitleEl = document.getElementById('docPickerSubtitle');
    const searchInput = document.getElementById('docPickerSearchInput');

    if (titleEl) {
        titleEl.textContent = target === 'A' ? 'Select Document A (Subject)' : 'Select Document B (Comparison Source)';
    }
    if (subtitleEl) {
        subtitleEl.textContent = target === 'A' ? 'Select the primary academic manuscript to analyze' : 'Select the baseline document to check against';
    }
    if (searchInput) {
        searchInput.value = '';
    }
    currentPickerSearchQuery = '';
    currentPickerFilterType = 'ALL';
    document.querySelectorAll('.picker-chip').forEach(c => {
        c.classList.toggle('active', c.getAttribute('data-type') === 'ALL');
    });

    renderDocPickerItems();
    if (modal) {
        modal.style.display = 'flex';
        setTimeout(() => searchInput?.focus(), 50);
    }
}

function closeDocPicker() {
    const modal = document.getElementById('docPickerModal');
    if (modal) modal.style.display = 'none';
}

function setDocPickerFilter(type, chipEl) {
    currentPickerFilterType = type;
    document.querySelectorAll('.picker-chip').forEach(c => c.classList.remove('active'));
    if (chipEl) chipEl.classList.add('active');
    renderDocPickerItems();
}

function filterDocPicker(query) {
    currentPickerSearchQuery = (query || '').toLowerCase().trim();
    renderDocPickerItems();
}

function renderDocPickerItems() {
    const listEl = document.getElementById('docPickerList');
    const countBadge = document.getElementById('docPickerCount');
    if (!listEl) return;

    const selA = document.getElementById('selectDocA');
    const selB = document.getElementById('selectDocB');
    const currentVal = (currentPickerTarget === 'A' ? selA?.value : selB?.value);

    // Instant in-memory filter: 0ms latency!
    const filtered = documentsList.filter(d => {
        const docExt = (d.type || d.name.split('.').pop() || '').toUpperCase();
        const matchesType = (currentPickerFilterType === 'ALL' || docExt === currentPickerFilterType);
        const matchesSearch = !currentPickerSearchQuery ||
                              (d.name && d.name.toLowerCase().includes(currentPickerSearchQuery)) ||
                              docExt.toLowerCase().includes(currentPickerSearchQuery);
        return matchesType && matchesSearch;
    });

    if (countBadge) {
        countBadge.textContent = `${filtered.length} of ${documentsList.length}`;
    }

    if (filtered.length === 0) {
        listEl.innerHTML = `
            <div style="text-align: center; padding: 36px 16px; color: var(--text-muted);">
                <div style="font-size: 32px; margin-bottom: 8px;">🔍</div>
                <div style="font-weight: 600; color: var(--text-secondary); margin-bottom: 4px;">No matching documents found</div>
                <div style="font-size: 12px;">Try adjusting your search query or upload new files above.</div>
            </div>
        `;
        return;
    }

    listEl.innerHTML = filtered.map(d => {
        const isSelected = (d.id == currentVal);
        const ext = (d.type || d.name.split('.').pop() || 'TXT').toUpperCase();
        let icon = '📄';
        if (ext === 'PDF') icon = '📕';
        else if (ext === 'DOCX') icon = '📘';

        return `
            <div class="doc-picker-item ${isSelected ? 'is-selected' : ''}" onclick="selectDocFromPicker(${d.id})">
                <div class="doc-picker-item-left">
                    <div class="doc-picker-item-icon">${icon}</div>
                    <div>
                        <div class="doc-picker-item-title" title="${escapeQuotes(d.name)}">${d.name}</div>
                        <div class="doc-picker-item-meta">${d.words ? d.words.toLocaleString() : 0} words • <span class="badge badge-subtle" style="font-size:10px; padding:1px 5px;">${ext}</span> • #${d.id} • ${d.uploadDate || 'recently'}</div>
                    </div>
                </div>
                <div class="doc-picker-item-right">
                    ${isSelected ? '<span class="badge badge-emerald" style="font-size:11px;">Current</span>' : '<span class="picker-select-badge">Select</span>'}
                </div>
            </div>
        `;
    }).join('');
}

function selectDocFromPicker(docId) {
    const selA = document.getElementById('selectDocA');
    const selB = document.getElementById('selectDocB');

    if (currentPickerTarget === 'A') {
        if (selA) selA.value = docId;
    } else {
        if (selB) selB.value = docId;
    }

    updateDocPickerCards();
    updatePreflightPreview();
    updateWorkflowStep(2);
    closeDocPicker();

    const selectedDoc = documentsList.find(d => d.id == docId);
    if (selectedDoc) {
        showToast(`Document ${currentPickerTarget} set to "${selectedDoc.name}"`, 'info');
    }
}

function updateDocPickerCards() {
    const selA = document.getElementById('selectDocA');
    const selB = document.getElementById('selectDocB');

    const docA = documentsList.find(d => d.id == selA?.value);
    const docB = documentsList.find(d => d.id == selB?.value);

    // Card A
    const nameA = document.getElementById('docA_name');
    const metaA = document.getElementById('docA_meta');
    const iconA = document.getElementById('docA_icon');
    if (docA) {
        if (nameA) nameA.textContent = docA.name;
        if (metaA) metaA.textContent = `${docA.words ? docA.words.toLocaleString() : 0} words • ${(docA.type || 'TXT').toUpperCase()} • #${docA.id}`;
        if (iconA) {
            const ext = (docA.type || docA.name.split('.').pop() || '').toUpperCase();
            iconA.textContent = ext === 'PDF' ? '📕' : (ext === 'DOCX' ? '📘' : '📄');
        }
    } else {
        if (nameA) nameA.textContent = 'Select Document A...';
        if (metaA) metaA.textContent = 'Click to choose from library';
        if (iconA) iconA.textContent = '📄';
    }

    // Card B
    const nameB = document.getElementById('docB_name');
    const metaB = document.getElementById('docB_meta');
    const iconB = document.getElementById('docB_icon');
    if (docB) {
        if (nameB) nameB.textContent = docB.name;
        if (metaB) metaB.textContent = `${docB.words ? docB.words.toLocaleString() : 0} words • ${(docB.type || 'TXT').toUpperCase()} • #${docB.id}`;
        if (iconB) {
            const ext = (docB.type || docB.name.split('.').pop() || '').toUpperCase();
            iconB.textContent = ext === 'PDF' ? '📕' : (ext === 'DOCX' ? '📘' : '📄');
        }
    } else {
        if (nameB) nameB.textContent = 'Select Document B...';
        if (metaB) metaB.textContent = 'Click to choose from library';
        if (iconB) iconB.textContent = '📄';
    }
}

// ==========================================================================
// FLOATING TOAST NOTIFICATION SYSTEM
// ==========================================================================
function showToast(message, type = 'info') {
    const container = document.getElementById('toastContainer');
    if (!container) return;

    const toast = document.createElement('div');
    toast.className = `toast-item toast-${type}`;

    let icon = 'ℹ️';
    if (type === 'success') icon = '✅';
    else if (type === 'error') icon = '❌';
    else if (type === 'warning') icon = '⚠️';

    toast.innerHTML = `
        <div class="toast-icon">${icon}</div>
        <div class="toast-content">
            <div class="toast-message">${message}</div>
        </div>
    `;

    container.appendChild(toast);

    setTimeout(() => {
        toast.style.animation = 'toastSlideOut 0.3s cubic-bezier(0.16, 1, 0.3, 1) forwards';
        setTimeout(() => toast.remove(), 300);
    }, 3500);
}

// ==========================================================================
// CINEMATIC PRELOADER & BOOTSTRAPPER
// ==========================================================================
function initPreloader() {
    const screen = document.getElementById('appLoadingScreen');
    const bar = document.getElementById('loadingProgressBar');
    const text = document.getElementById('loadingStatusText');
    if (!screen || !bar || !text) return;

    const stages = [
        { progress: 25, label: 'Bootstrapping C++20 Core Engine...', delay: 180 },
        { progress: 55, label: 'Initializing Polynomial Rolling Hash (Mod 10⁹+7)...', delay: 480 },
        { progress: 80, label: 'Mounting Inverted Index & Separate Chaining...', delay: 780 },
        { progress: 100, label: 'Syncing SQLite Document Graph & Max-Heap...', delay: 1080 }
    ];

    stages.forEach(s => {
        setTimeout(() => {
            bar.style.width = s.progress + '%';
            text.textContent = s.label;
        }, s.delay);
    });

    setTimeout(() => {
        text.textContent = 'Engine Online • Verification Complete.';
        setTimeout(() => {
            screen.classList.add('fade-out');
            setTimeout(() => {
                screen.style.display = 'none';
            }, 600);
        }, 300);
    }, 1300);
}

// ==========================================================================
// SCROLL-DRIVEN INTERACTIVE ENGINE STORYTELLING JOURNEY
// ==========================================================================
let currentStoryStep = 1;
const storyStepTitles = {
    1: 'C++20 ENGINE • STAGE 01: ARCHITECTURE OVERVIEW',
    2: 'C++20 ENGINE • STAGE 02: MULTI-FORMAT INGESTION & VISION OCR',
    3: 'C++20 ENGINE • STAGE 03: POLYNOMIAL ROLLING HASH WINDOW (O(1))',
    4: 'C++20 ENGINE • STAGE 04: SEPARATE-CHAINING INVERTED INDEX',
    5: 'C++20 ENGINE • STAGE 05: RABIN-KARP PASSAGE VERIFICATION',
    6: 'C++20 ENGINE • STAGE 06: SIMILARITY NETWORK GRAPH & MAX-HEAP'
};

function scrollToJourney() {
    const el = document.getElementById('engineJourney');
    if (el) {
        el.scrollIntoView({ behavior: 'smooth', block: 'start' });
    }
}

function scrollToModernWhy() {
    const el = document.getElementById('modernWhySection');
    if (el) {
        el.scrollIntoView({ behavior: 'smooth', block: 'start' });
    }
}

function scrollDownStep() {
    const scrollEl = document.querySelector('.main-content');
    if (scrollEl) {
        scrollEl.scrollBy({ top: window.innerHeight * 0.72, behavior: 'smooth' });
    } else {
        window.scrollBy({ top: window.innerHeight * 0.72, behavior: 'smooth' });
    }
}

function initScrollTracker() {
    const scrollEl = document.querySelector('.main-content');
    const carrier = document.getElementById('scrollArrowCarrier');
    const badge = document.getElementById('scrollPercentBadge');
    const railFill = document.getElementById('scrollRailProgress');
    const auroraBg = document.querySelector('.aurora-mesh-background');
    const orb1 = document.querySelector('.aurora-orb-1');
    const orb2 = document.querySelector('.aurora-orb-2');
    const orb3 = document.querySelector('.aurora-orb-3');
    const orb4 = document.querySelector('.aurora-orb-4');

    function onScroll(target) {
        const max = target.scrollHeight - target.clientHeight;
        const progress = max > 0 ? (target.scrollTop / max) : 0;
        const bounded = Math.min(1, Math.max(0, progress));
        const percent = Math.round(bounded * 100);

        if (carrier) {
            carrier.style.top = `calc(${bounded * 100}% - 14px)`;
        }
        if (railFill) {
            railFill.style.height = `${bounded * 100}%`;
        }
        if (badge) {
            badge.textContent = `${percent}%`;
        }

        // Dynamic 3D lighting & aurora transition based on scroll position
        if (auroraBg) {
            auroraBg.classList.toggle('scroll-warm', bounded > 0.25);
            const yOffset = bounded * 90;
            const xOffset = Math.sin(bounded * Math.PI) * 45;
            if (orb1) orb1.style.transform = `translate3d(${xOffset}px, ${yOffset * 0.7}px, 0) scale(${1 + bounded * 0.2})`;
            if (orb2) orb2.style.transform = `translate3d(${-xOffset}px, ${-yOffset * 0.4}px, 0) scale(${1 - bounded * 0.12})`;
            if (orb3) orb3.style.transform = `translate3d(${xOffset * 0.5}px, ${yOffset * 0.5}px, 0)`;
            if (orb4) orb4.style.transform = `translate3d(${-xOffset * 0.6}px, ${-yOffset * 0.3}px, 0)`;
        }

        // Real-time bidirectional check for story beats (works when scrolling UP or DOWN)
        const beats = document.querySelectorAll('.narrative-beat');
        if (beats.length > 0) {
            const viewportCenter = window.innerHeight * 0.45;
            let closestBeat = null;
            let minDistance = Infinity;

            beats.forEach(b => {
                const rect = b.getBoundingClientRect();
                const beatCenter = rect.top + rect.height * 0.5;
                const distance = Math.abs(beatCenter - viewportCenter);
                if (distance < minDistance && rect.bottom > 80 && rect.top < window.innerHeight - 80) {
                    minDistance = distance;
                    closestBeat = b;
                }
            });

            if (closestBeat) {
                const step = parseInt(closestBeat.getAttribute('data-step') || '1');
                if (step !== currentStoryStep) {
                    activateStoryStep(step, false);
                }
            }
        }

        // Trigger section scroll-reveals as user scrolls down or up
        triggerScrollReveals(target);
    }

    if (scrollEl) {
        scrollEl.addEventListener('scroll', () => onScroll(scrollEl), { passive: true });
        onScroll(scrollEl);
    }
    window.addEventListener('scroll', () => onScroll(document.documentElement), { passive: true });
}

function initScrollytelling() {
    const beats = document.querySelectorAll('.narrative-beat');
    if (beats.length === 0) return;

    // Use IntersectionObserver with comfortable threshold to switch animations as user scrolls
    const observer = new IntersectionObserver((entries) => {
        entries.forEach(entry => {
            if (entry.isIntersecting) {
                const step = parseInt(entry.target.getAttribute('data-step') || '1');
                activateStoryStep(step, false);
            }
        });
    }, {
        root: null,
        rootMargin: '-15% 0px -35% 0px',
        threshold: 0.15
    });

    beats.forEach(b => observer.observe(b));
}

function activateStoryStep(stepNum, shouldScroll = false) {
    currentStoryStep = stepNum;

    // Update Monitor Title & Step Counter
    const titleEl = document.getElementById('stageMonitorTitle');
    const labelEl = document.getElementById('stepCounterLabel');
    if (titleEl) titleEl.textContent = storyStepTitles[stepNum] || `STAGE 0${stepNum}`;
    if (labelEl) labelEl.textContent = `STAGE ${stepNum} / 6`;

    // Toggle Monitor Visual with 3D pulse
    document.querySelectorAll('.stage-visual').forEach((vis, idx) => {
        const isActive = (idx + 1) === stepNum;
        vis.classList.toggle('active', isActive);
    });

    // Toggle Monitor Bottom Dock Pills
    document.querySelectorAll('.step-nav-btn').forEach((btn, idx) => {
        btn.classList.toggle('active', (idx + 1) === stepNum);
    });

    // Toggle Narrative Beat Active State
    document.querySelectorAll('.narrative-beat').forEach(b => {
        const bStep = parseInt(b.getAttribute('data-step') || '1');
        b.classList.toggle('active', bStep === stepNum);
    });

    if (shouldScroll) {
        const targetBeat = document.getElementById(`beat-${stepNum}`);
        if (targetBeat) {
            targetBeat.scrollIntoView({ behavior: 'smooth', block: 'center' });
        }
    }
}

function jumpToStoryStep(stepNum) {
    activateStoryStep(stepNum, true);
}

// ==========================================================================
// SCROLL-DRIVEN 3D REVEAL & STATISTIC ENGINE
// ==========================================================================
let scrollRevealObserver = null;

function initScrollReveal() {
    const revealElements = document.querySelectorAll(
        '.scroll-reveal, .scroll-reveal-left, .scroll-reveal-right, .scroll-reveal-scale, .scroll-reveal-flip, .scroll-reveal-zoom'
    );
    if (revealElements.length === 0) return;

    if ('IntersectionObserver' in window) {
        if (scrollRevealObserver) {
            scrollRevealObserver.disconnect();
        }

        scrollRevealObserver = new IntersectionObserver((entries) => {
            entries.forEach(entry => {
                if (entry.isIntersecting) {
                    revealElement(entry.target);
                }
            });
        }, {
            root: null,
            rootMargin: '0px 0px -40px 0px',
            threshold: 0.08
        });

        revealElements.forEach(el => scrollRevealObserver.observe(el));
    }

    // Immediately reveal elements that are already within visible screen
    const scrollEl = document.querySelector('.main-content') || window;
    triggerScrollReveals(scrollEl);
}

function triggerScrollReveals(container) {
    const vHeight = window.innerHeight;
    const revealElements = document.querySelectorAll(
        '.scroll-reveal:not(.revealed), .scroll-reveal-left:not(.revealed), .scroll-reveal-right:not(.revealed), .scroll-reveal-scale:not(.revealed), .scroll-reveal-flip:not(.revealed), .scroll-reveal-zoom:not(.revealed)'
    );

    revealElements.forEach(el => {
        const rect = el.getBoundingClientRect();
        if (rect.top < vHeight - 30 && rect.bottom > 30) {
            revealElement(el);
        }
    });
}

function revealElement(el) {
    el.classList.add('revealed');

    // If divider line is inside, trigger its width expansion
    const divider = el.querySelector('.scroll-divider-line');
    if (divider) divider.classList.add('revealed');

    // Trigger number counter animations for problem stats
    const statNums = el.querySelectorAll('.problem-stat-num:not(.animated)');
    statNums.forEach(numEl => {
        numEl.classList.add('animated');
        const targetAttr = numEl.getAttribute('data-target');
        const suffixAttr = numEl.getAttribute('data-suffix') || '';
        if (targetAttr !== null) {
            animateNumberEl(numEl, 0, parseFloat(targetAttr), suffixAttr, 850);
        } else {
            const match = numEl.textContent.trim().match(/([0-9.]+)(%?)/);
            if (match) {
                animateNumberEl(numEl, 0, parseFloat(match[1]), match[2] || '', 850);
            }
        }
    });
}

function animateNumberEl(el, start, end, suffix = '', duration = 850) {
    const startTime = performance.now();
    function tick(now) {
        const elapsed = now - startTime;
        const progress = Math.min(elapsed / duration, 1);
        const ease = 1 - Math.pow(1 - progress, 3);
        const current = start + (end - start) * ease;
        el.textContent = Math.round(current) + suffix;
        if (progress < 1) {
            requestAnimationFrame(tick);
        } else {
            el.textContent = end + suffix;
        }
    }
    requestAnimationFrame(tick);
}

// --------------------------------------------------------------------------
// ROLLING HASH WINDOW INTERACTIVE SIMULATION (VISUAL 3)
// --------------------------------------------------------------------------
let rollingWindowStartIdx = 1;
const rollingWindowTokens = [
    { word: "academic", val: "0x12A9" },
    { word: "integrity", val: "0x4F88" },
    { word: "platform", val: "0x32D1" },
    { word: "verifies", val: "0x7CE4" },
    { word: "overlapping", val: "0x8B3E" },
    { word: "passages", val: "0x91F0" },
    { word: "cleanly", val: "0x5E0A" }
];
let autoSlideInterval = null;

function stepRollingWindow(dir) {
    const cells = document.querySelectorAll('.token-cell');
    if (cells.length === 0) return;

    rollingWindowStartIdx += dir;
    if (rollingWindowStartIdx < 0) rollingWindowStartIdx = 0;
    if (rollingWindowStartIdx > cells.length - 4) rollingWindowStartIdx = cells.length - 4;

    cells.forEach((c, idx) => {
        const inWindow = (idx >= rollingWindowStartIdx && idx < rollingWindowStartIdx + 4);
        c.classList.toggle('active', inWindow);
    });

    const outWord = rollingWindowTokens[Math.max(0, rollingWindowStartIdx - 1)]?.word || "academic";
    const inWord = rollingWindowTokens[rollingWindowStartIdx + 3]?.word || "passages";

    const outEl = document.getElementById('hashTokenOut');
    const inEl = document.getElementById('hashTokenIn');
    const hashEl = document.getElementById('liveRollingHashVal');

    if (outEl) outEl.textContent = `"${outWord}"`;
    if (inEl) inEl.textContent = `"${inWord}"`;

    // Synthetic rolling digest update
    const hashes = ["0x4A81BC22", "0x8B3E421A", "0x91F02A4C", "0x63DE1780"];
    if (hashEl) hashEl.textContent = hashes[rollingWindowStartIdx % hashes.length];
}

function toggleAutoSlide() {
    const btn = document.getElementById('btnAutoSlide');
    if (autoSlideInterval) {
        clearInterval(autoSlideInterval);
        autoSlideInterval = null;
        if (btn) {
            btn.textContent = '⚡ Auto-Slide';
            btn.classList.remove('btn-primary');
            btn.classList.add('btn-secondary');
        }
        showToast('Auto-slide paused', 'info');
    } else {
        autoSlideInterval = setInterval(() => {
            const cells = document.querySelectorAll('.token-cell');
            if (rollingWindowStartIdx >= cells.length - 4) {
                rollingWindowStartIdx = -1;
            }
            stepRollingWindow(1);
        }, 1400);
        if (btn) {
            btn.textContent = '⏸ Pause';
            btn.classList.remove('btn-secondary');
            btn.classList.add('btn-primary');
        }
        showToast('Running live sliding window animation', 'success');
    }
}

// ==========================================================================
// INTERACTIVE LIVE ALGORITHM PLAYGROUND (PULLING IN NATURE)
// ==========================================================================
function initLivePlayground() {
    updateLivePlayground();
}

function scrollToPlayground() {
    const el = document.getElementById('landingPlayground');
    if (el) {
        el.scrollIntoView({ behavior: 'smooth', block: 'center' });
        el.style.transform = 'scale(1.02)';
        el.style.borderColor = 'var(--accent-emerald)';
        setTimeout(() => {
            el.style.transform = '';
            el.style.borderColor = '';
        }, 600);
    }
}

function loadSamplePlaygroundTexts() {
    const inA = document.getElementById('playgroundInputA');
    const inB = document.getElementById('playgroundInputB');
    if (!inA || !inB) return;

    inA.value = "Deep learning and natural language processing architectures have transformed academic document analysis and automated similarity extraction.";
    inB.value = "Modern deep learning and natural language processing systems have drastically altered academic manuscript analysis and document similarity detection.";
    updateLivePlayground();
    showToast('Loaded academic comparison sample into playground', 'info');
}

function updateLivePlayground() {
    const textA = (document.getElementById('playgroundInputA')?.value || '').trim();
    const textB = (document.getElementById('playgroundInputB')?.value || '').trim();

    // Word counts
    const wordsA = textA ? textA.split(/\s+/) : [];
    const wordsB = textB ? textB.split(/\s+/) : [];

    const countA = document.getElementById('tokenCountA');
    const countB = document.getElementById('tokenCountB');
    if (countA) countA.textContent = `${wordsA.length} words`;
    if (countB) countB.textContent = `${wordsB.length} words`;

    // Polynomial Rolling Hash computation (Base B=31, Modulo M = 10^9 + 7)
    const B = 31n;
    const M = 1000000007n;

    function computePolyHash(tokens) {
        if (tokens.length === 0) return 0n;
        let h = 0n;
        for (let word of tokens) {
            let clean = word.toLowerCase().replace(/[^a-z0-9]/g, '');
            let wVal = 0n;
            for (let i = 0; i < clean.length; i++) {
                wVal = (wVal * 31n + BigInt(clean.charCodeAt(i))) % M;
            }
            h = (h * B + wVal) % M;
        }
        return h;
    }

    const hashA = computePolyHash(wordsA);
    const hashB = computePolyHash(wordsB);

    const hashAEl = document.getElementById('playgroundHashA');
    const hashBEl = document.getElementById('playgroundHashB');
    if (hashAEl) hashAEl.textContent = '0x' + hashA.toString(16).toUpperCase().padStart(8, '0');
    if (hashBEl) hashBEl.textContent = '0x' + hashB.toString(16).toUpperCase().padStart(8, '0');

    // 3-Gram Extraction
    function get3Grams(tokens) {
        const ngrams = new Set();
        const clean = tokens.map(t => t.toLowerCase().replace(/[^a-z0-9]/g, '')).filter(Boolean);
        for (let i = 0; i <= clean.length - 3; i++) {
            ngrams.add(`${clean[i]}_${clean[i+1]}_${clean[i+2]}`);
        }
        return ngrams;
    }

    const ngramsA = get3Grams(wordsA);
    const ngramsB = get3Grams(wordsB);

    let intersectionCount = 0;
    ngramsA.forEach(ng => {
        if (ngramsB.has(ng)) intersectionCount++;
    });

    const unionCount = new Set([...ngramsA, ...ngramsB]).size;
    let jaccard = unionCount > 0 ? (intersectionCount / unionCount) * 100 : 0;
    let directionalAtoB = ngramsA.size > 0 ? (intersectionCount / ngramsA.size) * 100 : 0;

    // Display
    const simValEl = document.getElementById('playgroundSimValue');
    const sharedEl = document.getElementById('playgroundSharedNgrams');
    const dirEl = document.getElementById('playgroundDirScore');
    const gauge = document.getElementById('playgroundGaugeCircle');

    if (simValEl) simValEl.textContent = jaccard.toFixed(1) + '%';
    if (sharedEl) sharedEl.textContent = `${intersectionCount} matching 3-grams`;
    if (dirEl) dirEl.textContent = directionalAtoB.toFixed(1) + '%';

    if (gauge) {
        const offset = 251.3 * (1 - (jaccard / 100));
        gauge.style.strokeDashoffset = offset;
        if (jaccard >= 60) gauge.style.stroke = '#F59E0B';
        else if (jaccard >= 30) gauge.style.stroke = '#38BDF8';
        else gauge.style.stroke = '#F59E0B';
    }

    // Render Interactive 3D Rolling Shingle Tokens
    const stripA = document.getElementById('playgroundTokensA');
    const stripB = document.getElementById('playgroundTokensB');

    function renderTokenChips(tokens, otherNgrams, container) {
        if (!container) return;
        container.innerHTML = '';
        const clean = tokens.map(t => t.toLowerCase().replace(/[^a-z0-9]/g, '')).filter(Boolean);
        if (clean.length === 0) {
            container.innerHTML = '<span style="font-size:11px; color:var(--text-muted);">Type text above to extract rolling polynomial shingles...</span>';
            return;
        }

        const maxTokens = Math.min(clean.length - 2, 8);
        for (let i = 0; i < maxTokens; i++) {
            const shingleKey = `${clean[i]}_${clean[i+1]}_${clean[i+2]}`;
            const isMatch = otherNgrams.has(shingleKey);
            const chip = document.createElement('div');
            chip.className = `hash-token-3d ${isMatch ? 'match' : ''}`;
            chip.innerHTML = `${isMatch ? '★ ' : ''}<span>${clean[i]} ${clean[i+1]} ${clean[i+2]}</span>`;
            container.appendChild(chip);
        }
        if (clean.length - 2 > maxTokens) {
            const more = document.createElement('span');
            more.style.fontSize = '11px';
            more.style.color = 'var(--text-muted)';
            more.style.alignSelf = 'center';
            more.textContent = `+${clean.length - 2 - maxTokens} more shingles`;
            container.appendChild(more);
        }
    }

    renderTokenChips(wordsA, ngramsB, stripA);
    renderTokenChips(wordsB, ngramsA, stripB);
}

function highlightPipelineStage(stageNum) {
    document.querySelectorAll('.pipeline-stage-card').forEach((card, idx) => {
        card.classList.toggle('active', idx === (stageNum - 1));
    });
}

function animateCounter(elementId, targetValue, suffix = '', decimals = 0) {
    const el = document.getElementById(elementId);
    if (!el) return;

    const start = 0;
    const duration = 900;
    const startTime = performance.now();

    function update(now) {
        const elapsed = now - startTime;
        const progress = Math.min(elapsed / duration, 1);
        const ease = 1 - Math.pow(1 - progress, 3);
        const current = start + (targetValue - start) * ease;

        el.textContent = (decimals > 0 ? current.toFixed(decimals) : Math.round(current)) + suffix;

        if (progress < 1) {
            requestAnimationFrame(update);
        } else {
            el.textContent = (decimals > 0 ? targetValue.toFixed(decimals) : targetValue) + suffix;
        }
    }

    requestAnimationFrame(update);
}

// ==========================================================================
// 7. API FETCHES (DASHBOARD & STATS)
// ==========================================================================
async function loadDashboard() {
    try {
        const res = await fetch('/api/stats');
        if (res.ok) {
            const data = await res.json();
            animateCounter('statDocsCount', data.totalDocuments);
            animateCounter('statAnalysesCount', data.totalAnalyses);
            animateCounter('statAvgSimilarity', data.averageSimilarity, '%', 1);
            animateCounter('statMatchesCount', data.totalMatchingPassages);

            if (data.histogram) {
                drawHistogram(data.histogram);
            }
        }

        const hRes = await fetch('/api/history');
        if (hRes.ok) {
            const history = await hRes.json();
            const tbody = document.querySelector('#dashboardRecentTable tbody');
            tbody.innerHTML = '';

            if (history.length === 0) {
                tbody.innerHTML = `
                    <tr>
                        <td colspan="8" style="text-align:center; padding: 32px; color:var(--text-muted);">
                            No analyses run yet. Launch your first comparison from the New Analysis panel.
                        </td>
                    </tr>
                `;
            } else {
                history.slice(0, 5).forEach(h => {
                    const tr = document.createElement('tr');
                    let scoreBadge = '#F59E0B';
                    if (h.avgSimilarity >= 60) scoreBadge = '#ef4444';
                    else if (h.avgSimilarity >= 25) scoreBadge = '#f59e0b';

                    tr.innerHTML = `
                        <td><strong>#${h.id}</strong></td>
                        <td>${h.documents || 'Document Pair'}</td>
                        <td><span class="badge badge-subtle">${h.type}</span></td>
                        <td style="color:var(--text-secondary); font-size:12px;">${h.date}</td>
                        <td><strong style="color:${scoreBadge};">${h.avgSimilarity.toFixed(1)}%</strong></td>
                        <td>${h.matchingPassages}</td>
                        <td><span style="color:#F59E0B; font-weight:600;">● ${h.status}</span></td>
                        <td>
                            <button class="btn btn-primary btn-sm" onclick="loadAnalysisDetail(${h.id})">Open</button>
                            <button class="btn btn-secondary btn-sm" onclick="downloadPdfReport(${h.id})">PDF</button>
                        </td>
                    `;
                    tbody.appendChild(tr);
                });
            }
        }
    } catch (e) {
        console.error('Dashboard error:', e);
    }
}

function drawHistogram(histogram) {
    const canvas = document.getElementById('histogramCanvas');
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    const labels = ['0-20%', '21-40%', '41-60%', '61-80%', '81-100%'];
    const colors = ['#F59E0B', '#38bdf8', '#818cf8', '#f59e0b', '#ef4444'];
    const maxVal = Math.max(...histogram, 4);

    const barWidth = 56;
    const gap = 34;
    const startX = 40;
    const baseY = 145;

    histogram.forEach((count, i) => {
        const x = startX + i * (barWidth + gap);
        const barHeight = Math.max(6, (count / maxVal) * 110);
        const y = baseY - barHeight;

        // Draw Bar
        ctx.fillStyle = colors[i];
        ctx.beginPath();
        ctx.roundRect(x, y, barWidth, barHeight, [4, 4, 0, 0]);
        ctx.fill();

        // Draw Value
        ctx.fillStyle = '#f8fafc';
        ctx.font = 'bold 11px Inter';
        ctx.textAlign = 'center';
        ctx.fillText(count.toString(), x + barWidth / 2, y - 6);

        // Draw Label
        ctx.fillStyle = '#94a3b8';
        ctx.font = '10.5px Inter';
        ctx.fillText(labels[i], x + barWidth / 2, baseY + 18);
    });
}

// ==========================================================================
// 8. DOCUMENT LIBRARY
// ==========================================================================
async function loadDocuments() {
    try {
        const res = await fetch('/api/documents');
        if (res.ok) {
            documentsList = await res.json();
            populateAnalysisSelectors();
            populateDiffSelectors();
            updatePreflightPreview();
        }
    } catch (e) {
        console.error('Error fetching documents:', e);
    }
}

async function loadDocumentsTable() {
    await loadDocuments();
    renderLibraryTable(documentsList);
}

function renderLibraryTable(docs) {
    const tbody = document.querySelector('#libraryTable tbody');
    if (!tbody) return;
    tbody.innerHTML = '';

    if (docs.length === 0) {
        tbody.innerHTML = `
            <tr>
                <td colspan="8">
                    <div class="empty-state-3d">
                        <div class="empty-state-3d-icon">
                            <svg width="34" height="34" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><polyline points="14 2 14 8 20 8"/><line x1="16" y1="13" x2="8" y2="13"/><line x1="16" y1="17" x2="8" y2="17"/></svg>
                        </div>
                        <h4 class="empty-state-3d-title">Library is Currently Empty</h4>
                        <p class="empty-state-3d-sub">Ingest academic PDFs, DOCX essays, or TXT manuscripts to generate SHA-256 fingerprints, token shingles, and inverted index mappings.</p>
                        <button class="btn btn-primary btn-sm btn-glow" onclick="switchTab('new-analysis')">
                            <span>+ Ingest First Document</span>
                        </button>
                    </div>
                </td>
            </tr>
        `;
        return;
    }

    docs.forEach(d => {
        const tr = document.createElement('tr');
        const shortFp = d.fingerprint ? d.fingerprint.substring(0, 14) + '...' : 'N/A';
        tr.innerHTML = `
            <td>#${d.id}</td>
            <td><strong>${d.name}</strong></td>
            <td><span class="badge badge-subtle">${d.type}</span></td>
            <td>${(d.size/1024).toFixed(1)} KB</td>
            <td>${d.words} words</td>
            <td style="color:var(--text-secondary); font-size:12px;">${d.uploadDate}</td>
            <td>
                <span class="code-pill" style="cursor:pointer;" onclick="copyText('${d.fingerprint}', this)" title="Click to copy full 128-bit hash">
                    <code>${shortFp}</code> 📋
                </span>
            </td>
            <td>
                <button class="btn btn-secondary btn-sm" onclick="inspectDocument(${d.id})">Inspect</button>
                <button class="btn btn-primary btn-sm" onclick="selectDocForComparison(${d.id})">Compare</button>
            </td>
        `;
        tbody.appendChild(tr);
    });
}

function filterLibraryTable(query) {
    const q = query.toLowerCase();
    const filtered = documentsList.filter(d => d.name.toLowerCase().includes(q) || d.type.toLowerCase().includes(q));
    renderLibraryTable(filtered);
}

function copyText(str, el) {
    navigator.clipboard.writeText(str);
    const orig = el.innerHTML;
    el.innerHTML = '<span style="color:#F59E0B;">Copied!</span>';
    setTimeout(() => { el.innerHTML = orig; }, 1500);
}

async function inspectDocument(id) {
    try {
        const res = await fetch(`/api/document?id=${id}`);
        if (res.ok) {
            const doc = await res.json();
            activeModalDocId = doc.id;
            document.getElementById('modalDocTitle').textContent = doc.name;
            document.getElementById('modalDocType').textContent = doc.type;
            document.getElementById('modalDocSize').textContent = `${(doc.size / 1024).toFixed(1)} KB`;
            document.getElementById('modalDocWords').textContent = `${doc.words} words (${doc.chars} chars)`;
            document.getElementById('modalDocDate').textContent = doc.uploadDate;
            document.getElementById('modalDocFingerprint').textContent = doc.fingerprint;
            document.getElementById('modalDocContent').textContent = doc.content || 'Content preview unavailable.';
            document.getElementById('documentDetailsModal').classList.add('active');
        }
    } catch (e) {
        console.error(e);
    }
}

function closeDocModal() {
    document.getElementById('documentDetailsModal').classList.remove('active');
}

function copyModalFingerprint() {
    const fp = document.getElementById('modalDocFingerprint').textContent;
    navigator.clipboard.writeText(fp);
    alert('Fingerprint copied to clipboard!');
}

function selectDocForComparison(id) {
    closeDocModal();
    switchTab('new-analysis');
    populateAnalysisSelectors(id);
    updatePreflightPreview();
}

function compareDocumentFromModal() {
    if (activeModalDocId) {
        selectDocForComparison(activeModalDocId);
    }
}

async function deleteDocumentFromModal() {
    if (!activeModalDocId) return;
    if (!confirm('Are you sure you want to delete this document from the library and graph?')) return;

    try {
        const res = await fetch('/api/delete-doc', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ id: activeModalDocId })
        });
        if (res.ok) {
            closeDocModal();
            await loadDocuments();
            loadDocumentsTable();
            loadDashboard();
        }
    } catch (e) {
        alert('Delete failed: ' + e.message);
    }
}

// ==========================================================================
// 9. ANALYSIS EXECUTION & RESULTS
// ==========================================================================
async function executeAnalysis() {
    await loadDocuments();
    if (documentsList.length < 2) {
        alert('Please ensure at least two documents are in the library before analyzing.');
        return;
    }

    const mode = document.querySelector('input[name="cmpMode"]:checked')?.value || 'two-doc';
    const windowSize = parseInt(document.getElementById('ngramSlider').value) || 5;

    if (mode === 'version') {
        const selA = document.getElementById('selectDocA');
        const selB = document.getElementById('selectDocB');
        if (selA && selB) {
            document.getElementById('diffDocASelect').value = selA.value;
            document.getElementById('diffDocBSelect').value = selB.value;
        }
        switchTab('version-diff');
        executeVersionDiff();
        return;
    }

    if (mode === 'batch') {
        runBatchAnalysis(windowSize);
        return;
    }

    // Two-Document Pair Mode
    const selA = document.getElementById('selectDocA');
    const selB = document.getElementById('selectDocB');
    let docAId = selA ? parseInt(selA.value) : 0;
    let docBId = selB ? parseInt(selB.value) : 0;

    let docA = documentsList.find(d => d.id === docAId);
    let docB = documentsList.find(d => d.id === docBId);

    if (!docA || !docB || docA.id === docB.id) {
        alert('Please select two distinct documents to compare.');
        return;
    }

    updateWorkflowStep(3);
    // Show Progress Modal
    const modal = document.getElementById('processingModal');
    modal.classList.add('active');

    const bar = document.getElementById('processingBar');
    const statusText = document.getElementById('processingStatusText');

    const stages = [
        { pct: 15, text: 'Extracting text (Native PDF/DOCX & Apple Vision OCR)...', step: 1 },
        { pct: 30, text: 'Preprocessing, lowercase conversion, and tokenization...', step: 2 },
        { pct: 45, text: `Generating overlapping ${windowSize}-gram sliding windows...`, step: 3 },
        { pct: 60, text: 'Computing Polynomial Rolling Hashes (Moduli 10^9+7, 998244353)...', step: 4 },
        { pct: 75, text: 'Running Rabin-Karp matching and deep collision verification...', step: 5 },
        { pct: 88, text: 'Calculating Jaccard and Directional Overlap coefficients...', step: 6 },
        { pct: 100, text: 'Constructing paragraph heatmap and report records...', step: 7 }
    ];

    for (let s of stages) {
        bar.style.width = s.pct + '%';
        statusText.textContent = s.text;
        document.querySelectorAll('.stage-step').forEach(st => st.classList.remove('active'));
        const stepEl = document.getElementById(`step${s.step}`);
        if (stepEl) stepEl.classList.add('active');
        await new Promise(r => setTimeout(r, 110));
    }

    try {
        const res = await fetch('/api/analyze-two', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ docAId: docA.id, docBId: docB.id, windowSize })
        });

        if (res.ok) {
            currentAnalysisData = await res.json();
            modal.classList.remove('active');
            displayResults(currentAnalysisData, docA.name, docB.name);
            document.getElementById('navResults').style.display = 'flex';
            document.getElementById('navComparison').style.display = 'flex';
            switchTab('results');
        } else {
            alert('Analysis failed.');
            modal.classList.remove('active');
        }
    } catch (e) {
        console.error('Analysis error:', e);
        modal.classList.remove('active');
    }
}

async function runBatchAnalysis(windowSize) {
    const modal = document.getElementById('processingModal');
    modal.classList.add('active');
    const bar = document.getElementById('processingBar');
    const statusText = document.getElementById('processingStatusText');

    bar.style.width = '40%';
    statusText.textContent = 'Comparing all document pairs in parallel via C++ engine...';

    try {
        const res = await fetch('/api/analyze-batch', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ windowSize })
        });
        if (res.ok) {
            const data = await res.json();
            bar.style.width = '100%';
            statusText.textContent = 'Batch complete! Updating Document Similarity Network...';
            await new Promise(r => setTimeout(r, 400));
            modal.classList.remove('active');
            switchTab('network');
            loadNetworkGraph();
        } else {
            alert('Batch analysis failed.');
            modal.classList.remove('active');
        }
    } catch (e) {
        modal.classList.remove('active');
        alert('Batch error: ' + e.message);
    }
}

function displayResults(data, nameA, nameB) {
    updateWorkflowStep(4);
    document.getElementById('resultsSubtitle').textContent = `${nameA} vs ${nameB}`;

    const score = data.jaccardScore || 0;
    document.getElementById('resJaccard').textContent = score.toFixed(1) + '%';
    document.getElementById('resDirectional').textContent = (data.directionalAtoB || 0).toFixed(1) + '%';
    document.getElementById('resMatchingShingles').textContent = data.matchingNgrams || 0;
    document.getElementById('resPassageCount').textContent = data.matchingPassagesCount || (data.passages ? data.passages.length : 0);
    document.getElementById('resUniqueShingles').textContent = `${data.uniqueNgramsA || 0} / ${data.uniqueNgramsB || 0}`;

    // Update Radial Circle
    const circle = document.getElementById('radialScoreCircle');
    if (circle) {
        const maxOffset = 465;
        const targetOffset = maxOffset - (maxOffset * (score / 100));
        circle.style.strokeDashoffset = targetOffset;
        if (score >= 60) circle.style.stroke = '#ef4444';
        else if (score >= 25) circle.style.stroke = '#f59e0b';
        else circle.style.stroke = '#F59E0B';
    }

    // Headline & Interpretation
    const headline = document.getElementById('resScoreHeadline');
    if (headline) {
        if (score >= 80) headline.textContent = 'Substantial Verbatim Overlap';
        else if (score >= 60) headline.textContent = 'High Similarity Detected';
        else if (score >= 40) headline.textContent = 'Moderate Shared Sequences';
        else if (score >= 20) headline.textContent = 'Low Incidental Overlap';
        else headline.textContent = 'Minimal Textual Overlap';
    }
    document.getElementById('resInterpretation').textContent = data.interpretation || 'No significant overlap detected.';

    // Render Heatmap
    const hmGrid = document.getElementById('paragraphHeatmapGrid');
    hmGrid.innerHTML = '';
    const heatmap = data.heatmap || [];
    if (heatmap.length === 0) {
        hmGrid.innerHTML = '<span style="color:var(--text-muted);font-size:12px;">Heatmap unavailable.</span>';
    } else {
        heatmap.forEach((val, idx) => {
            const cell = document.createElement('div');
            cell.className = 'heatmap-cell';
            let bg = 'rgba(16, 185, 129, 0.2)';
            let color = '#F59E0B';
            if (val >= 60) {
                bg = 'rgba(239, 68, 68, 0.35)';
                color = '#ef4444';
            } else if (val >= 25) {
                bg = 'rgba(245, 158, 11, 0.3)';
                color = '#f59e0b';
            }
            cell.style.background = bg;
            cell.style.color = color;
            cell.textContent = `P${idx+1}`;
            cell.title = `Paragraph ${idx+1}: ${val.toFixed(0)}% overlap`;
            hmGrid.appendChild(cell);
        });
    }

    // Render Matching Passages
    const passContainer = document.getElementById('passagesListContainer');
    passContainer.innerHTML = '';
    const passages = data.passages || [];
    document.getElementById('evidenceCountBadge').textContent = `${passages.length} passages confirmed`;

    if (passages.length === 0) {
        passContainer.innerHTML = '<p style="color:var(--text-muted);font-style:italic;padding:8px 0;">No identical or partial passages exceeded the minimum threshold.</p>';
    } else {
        passages.forEach((p, idx) => {
            const card = document.createElement('div');
            card.className = 'passage-card';
            card.innerHTML = `
                <div class="passage-header">
                    <span class="passage-num">Passage #${idx+1} (${p.length} words, Tokens ${p.sourceCharStart}–${p.sourceCharEnd})</span>
                    <button class="btn btn-secondary btn-sm" onclick="jumpToMatch(${p.sourceCharStart})">Inspect in Side-by-Side</button>
                </div>
                <div class="passage-text">
                    "${escapeHtml(p.text)}"
                </div>
            `;
            passContainer.appendChild(card);
        });
    }

    // Populate Side-by-Side
    populateSideBySide(nameA, nameB, data.docAContent || '', data.docBContent || '', passages);
}

async function loadAnalysisDetail(id) {
    try {
        const res = await fetch(`/api/analysis-detail?id=${id}`);
        if (res.ok) {
            const data = await res.json();
            currentAnalysisData = data;
            displayResults(data, data.docAName, data.docBName);
            document.getElementById('navResults').style.display = 'flex';
            document.getElementById('navComparison').style.display = 'flex';
            switchTab('results');
        } else {
            alert('Failed to load analysis record.');
        }
    } catch (e) {
        alert('Error: ' + e.message);
    }
}

// ==========================================================================
// 10. SIDE-BY-SIDE HIGH-CONTRAST VIEWER
// ==========================================================================
function populateSideBySide(nameA, nameB, textA, textB, passages) {
    document.getElementById('compDocAName').textContent = nameA;
    document.getElementById('compDocBName').textContent = nameB;

    const wordsA = textA.trim().split(/\s+/).filter(w => w.length > 0).length;
    const wordsB = textB.trim().split(/\s+/).filter(w => w.length > 0).length;
    document.getElementById('compDocAWords').textContent = `${wordsA} words`;
    document.getElementById('compDocBWords').textContent = `${wordsB} words`;

    const paneA = document.getElementById('compDocAPane');
    const paneB = document.getElementById('compDocBPane');

    let htmlA = escapeHtml(textA);
    let htmlB = escapeHtml(textB);

    passages.forEach(p => {
        if (p.text && p.text.length > 10) {
            const escaped = escapeHtml(p.text);
            const mark = `<mark class="highlight-match" id="match-a-${p.sourceCharStart}">${escaped}</mark>`;
            htmlA = htmlA.split(escaped).join(mark);
            htmlB = htmlB.split(escaped).join(mark);
        }
    });

    paneA.innerHTML = htmlA;
    paneB.innerHTML = htmlB;
}

function jumpToMatch(charStart) {
    switchTab('comparison');
    setTimeout(() => {
        const el = document.getElementById(`match-a-${charStart}`);
        if (el) {
            el.scrollIntoView({ behavior: 'smooth', block: 'center' });
            el.style.outline = '2px solid #ffffff';
        }
    }, 150);
}

function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text || '';
    return div.innerHTML;
}

// ==========================================================================
// 11. ADVANCED SIMILARITY NETWORK & MAX-HEAP ENGINE
// ==========================================================================

// --- Graph Simulation & Physics Controller ---
class GraphPhysicsEngine {
    constructor(canvas) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.nodes = [];
        this.edges = [];
        this.particles = [];
        this.width = canvas.width;
        this.height = canvas.height;
        this.center = { x: canvas.width / 2, y: canvas.height / 2 };
        this.alpha = 1.0;
        this.alphaMin = 0.001;
        this.alphaDecay = 0.022;
        this.isRunning = false;
        this.rippleRings = []; // Animated shockwaves during BFS/DFS
        this.resize();
    }

    resize() {
        if (!this.canvas) return;
        const wrapper = this.canvas.parentElement;
        if (!wrapper) return;
        const rect = wrapper.getBoundingClientRect();
        if (rect.width <= 0) return;

        const dpr = window.devicePixelRatio || 1;
        const targetWidth = Math.max(340, Math.floor(rect.width));
        const targetHeight = Math.max(400, Math.min(560, Math.floor(window.innerHeight * 0.56)));

        this.canvas.width = targetWidth * dpr;
        this.canvas.height = targetHeight * dpr;
        this.canvas.style.width = targetWidth + 'px';
        this.canvas.style.height = targetHeight + 'px';

        this.width = targetWidth;
        this.height = targetHeight;
        this.center = { x: targetWidth / 2, y: targetHeight / 2 };
        this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
        this.alpha = 0.8;
    }

    setGraph(nodes, edges, layout = 'physics') {
        this.resize();
        const existingMap = new Map(this.nodes.map(n => [n.id, n]));
        this.width = this.canvas.width;
        this.height = this.canvas.height;
        this.center = { x: this.width / 2, y: this.height / 2 };

        const count = nodes.length;
        const radius = Math.min(this.width, this.height) * 0.38;

        this.nodes = nodes.map((n, i) => {
            const existing = existingMap.get(n.id);
            const angle = (2 * Math.PI * i) / (count || 1);
            const circleX = this.center.x + radius * Math.cos(angle);
            const circleY = this.center.y + radius * Math.sin(angle);

            let initialX = n.x || circleX;
            let initialY = n.y || circleY;

            if (existing) {
                initialX = existing.x;
                initialY = existing.y;
            } else if (layout === 'circle') {
                initialX = circleX;
                initialY = circleY;
            } else {
                // Add jitter for spring physics
                initialX = this.center.x + (Math.random() - 0.5) * 220;
                initialY = this.center.y + (Math.random() - 0.5) * 220;
            }

            return {
                id: n.id,
                name: n.name || ('Doc ' + n.id),
                wordCount: n.wordCount || 0,
                x: initialX,
                y: initialY,
                vx: 0,
                vy: 0,
                radius: 24,
                circleX: circleX,
                circleY: circleY,
                isFixed: false
            };
        });

        this.edges = edges.map(e => ({
            source: e.source,
            target: e.target,
            weight: e.weight
        }));

        this.initParticles();
        this.alpha = 1.0;
        this.start();
    }

    initParticles() {
        this.particles = [];
        this.edges.forEach(e => {
            const count = Math.max(1, Math.min(4, Math.floor(e.weight / 25)));
            for (let i = 0; i < count; i++) {
                this.particles.push({
                    sourceId: e.source,
                    targetId: e.target,
                    weight: e.weight,
                    progress: Math.random(),
                    speed: 0.003 + (e.weight / 100) * 0.009
                });
            }
        });
    }

    tick(layout = 'physics') {
        if (layout === 'circle') {
            // Smoothly interpolate towards circle positions
            this.nodes.forEach(n => {
                if (!n.isFixed) {
                    n.x += (n.circleX - n.x) * 0.12;
                    n.y += (n.circleY - n.y) * 0.12;
                }
            });
            return true;
        }

        if (this.alpha < this.alphaMin) return false;

        const kRepel = 7500;
        const kCenter = 0.012;
        const kSpring = 0.055;
        const idealDist = 175;

        // 1. Coulomb Repulsion between all pairs of nodes
        for (let i = 0; i < this.nodes.length; i++) {
            const na = this.nodes[i];
            for (let j = i + 1; j < this.nodes.length; j++) {
                const nb = this.nodes[j];
                const dx = nb.x - na.x;
                const dy = nb.y - na.y;
                let distSq = dx * dx + dy * dy;
                if (distSq < 1) distSq = 1;
                const dist = Math.sqrt(distSq);

                const force = (kRepel / distSq) * this.alpha;
                const fx = (dx / dist) * force;
                const fy = (dy / dist) * force;

                if (!na.isFixed) { na.vx -= fx; na.vy -= fy; }
                if (!nb.isFixed) { nb.vx += fx; nb.vy += fy; }
            }
        }

        // 2. Hooke Spring Attraction along edges
        const nodeMap = new Map(this.nodes.map(n => [n.id, n]));
        this.edges.forEach(e => {
            const na = nodeMap.get(e.source);
            const nb = nodeMap.get(e.target);
            if (!na || !nb) return;

            const dx = nb.x - na.x;
            const dy = nb.y - na.y;
            const dist = Math.sqrt(dx * dx + dy * dy) || 1;
            const delta = dist - idealDist;
            const force = delta * kSpring * (e.weight / 100 + 0.4) * this.alpha;

            const fx = (dx / dist) * force;
            const fy = (dy / dist) * force;

            if (!na.isFixed) { na.vx -= fx; na.vy -= fy; }
            if (!nb.isFixed) { nb.vx += fx; nb.vy += fy; }
        });

        // 3. Center Gravity & Position Integration
        const padding = 45;
        this.nodes.forEach(n => {
            if (n.isFixed) {
                n.vx = 0;
                n.vy = 0;
                return;
            }

            // Gravity towards center
            n.vx += (this.center.x - n.x) * kCenter * this.alpha;
            n.vy += (this.center.y - n.y) * kCenter * this.alpha;

            // Velocity damping
            n.vx *= 0.84;
            n.vy *= 0.84;

            n.x += n.vx;
            n.y += n.vy;

            // Boundary collision
            if (n.x < padding) { n.x = padding; n.vx *= -0.5; }
            if (n.x > this.width - padding) { n.x = this.width - padding; n.vx *= -0.5; }
            if (n.y < padding) { n.y = padding; n.vy *= -0.5; }
            if (n.y > this.height - padding) { n.y = this.height - padding; n.vy *= -0.5; }
        });

        this.alpha *= (1 - this.alphaDecay);
        return true;
    }

    triggerShockwave(nodeId, color = '#F59E0B') {
        const node = this.nodes.find(n => n.id === nodeId);
        if (!node) return;
        this.rippleRings.push({
            x: node.x,
            y: node.y,
            radius: 20,
            maxRadius: 75,
            opacity: 0.9,
            color: color
        });
        this.wake();
    }

    wake() {
        this.alpha = Math.max(this.alpha, 0.45);
        if (!this.isRunning) this.start();
    }

    start() {
        if (this.isRunning) return;
        this.isRunning = true;
        const loop = () => {
            if (!this.isRunning) return;
            const hasEnergy = this.tick(activeGraphLayout);

            // Update particles
            this.particles.forEach(p => {
                p.progress += p.speed;
                if (p.progress > 1) p.progress = 0;
            });

            // Update shockwaves
            for (let i = this.rippleRings.length - 1; i >= 0; i--) {
                const r = this.rippleRings[i];
                r.radius += 2.4;
                r.opacity -= 0.035;
                if (r.opacity <= 0 || r.radius >= r.maxRadius) {
                    this.rippleRings.splice(i, 1);
                }
            }

            this.render();

            const hudEnergy = document.getElementById('graphHudEnergy');
            if (hudEnergy) {
                hudEnergy.textContent = (hasEnergy || isDraggingNode || activeTraversal.running) ? 'Simulation: Active' : 'Simulation: Settled (Idle)';
            }

            graphAnimFrameId = requestAnimationFrame(loop);
        };
        graphAnimFrameId = requestAnimationFrame(loop);
    }

    stop() {
        this.isRunning = false;
        if (graphAnimFrameId) cancelAnimationFrame(graphAnimFrameId);
    }

    render() {
        const ctx = this.ctx;
        ctx.clearRect(0, 0, this.width, this.height);

        const nodeMap = new Map(this.nodes.map(n => [n.id, n]));
        const isFocusMode = (hoveredGraphNode !== null);
        const focusNode = hoveredGraphNode;
        const cutoff = parseFloat(document.getElementById('graphThresholdSlider')?.value || 20);

        // Identify neighbor IDs if in hover focus mode
        const focusedNeighbors = new Set();
        if (focusNode) {
            focusedNeighbors.add(focusNode.id);
            this.edges.forEach(e => {
                if (e.source === focusNode.id) focusedNeighbors.add(e.target);
                if (e.target === focusNode.id) focusedNeighbors.add(e.source);
            });
        }

        // 1. Draw Edges
        let visibleEdgeCount = 0;
        this.edges.forEach(e => {
            const na = nodeMap.get(e.source);
            const nb = nodeMap.get(e.target);
            if (!na || !nb) return;

            const minId = Math.min(e.source, e.target);
            const maxId = Math.max(e.source, e.target);
            const isMSTEdge = showMSTOnly && mstEdgeKeys.has(minId + '-' + maxId);
            if (showMSTOnly && !isMSTEdge) return; // Hidden in MST mode

            if (e.weight < cutoff && !isMSTEdge) return; // Filtered by threshold slider
            visibleEdgeCount++;

            const isIncident = focusNode && (e.source === focusNode.id || e.target === focusNode.id);
            const isHoveredEdge = hoveredGraphEdge && (
                (hoveredGraphEdge.source === e.source && hoveredGraphEdge.target === e.target) ||
                (hoveredGraphEdge.source === e.target && hoveredGraphEdge.target === e.source)
            );
            const isTraversed = activeTraversal.traversedEdges.has(e.source + '-' + e.target) ||
                                activeTraversal.traversedEdges.has(e.target + '-' + e.source);

            ctx.beginPath();
            ctx.moveTo(na.x, na.y);
            ctx.lineTo(nb.x, nb.y);

            let edgeColor = 'rgba(148, 163, 184, 0.3)';
            let edgeWidth = 1.6;

            if (isTraversed) {
                edgeColor = '#38BDF8';
                edgeWidth = 4.2;
            } else if (isHoveredEdge || isIncident) {
                edgeColor = '#F59E0B';
                edgeWidth = 3.8;
            } else if (isMSTEdge) {
                edgeColor = '#8B5CF6';
                edgeWidth = 3.2;
            } else if (e.weight >= 60) {
                edgeColor = 'rgba(239, 68, 68, 0.85)';
                edgeWidth = 3.5;
            } else if (e.weight >= 30) {
                edgeColor = 'rgba(245, 158, 11, 0.75)';
                edgeWidth = 2.4;
            } else {
                edgeColor = 'rgba(56, 189, 248, 0.55)';
                edgeWidth = 1.8;
            }

            if (isFocusMode && !isIncident && !isHoveredEdge) {
                ctx.strokeStyle = 'rgba(148, 163, 184, 0.12)';
                ctx.lineWidth = 1;
            } else {
                ctx.strokeStyle = edgeColor;
                ctx.lineWidth = edgeWidth;
            }

            if (isMSTEdge) {
                ctx.setLineDash([5, 4]);
            } else {
                ctx.setLineDash([]);
            }
            ctx.stroke();
            ctx.setLineDash([]);

            // Draw Edge Weight Badge
            if (!isFocusMode || isIncident || isHoveredEdge) {
                const midX = (na.x + nb.x) / 2;
                const midY = (na.y + nb.y) / 2;
                ctx.save();
                ctx.fillStyle = isIncident ? '#F59E0B' : (isTraversed ? '#38BDF8' : '#64748B');
                ctx.font = 'bold 10px JetBrains Mono, monospace';
                ctx.textAlign = 'center';
                ctx.textBaseline = 'middle';
                ctx.fillText(e.weight.toFixed(0) + '%', midX, midY - 6);
                ctx.restore();
            }
        });

        const hudEdges = document.getElementById('graphHudEdgesCount');
        if (hudEdges) hudEdges.textContent = 'Edges: ' + visibleEdgeCount + ' rendered';

        // 2. Draw Dynamic Flow Particles along Edges
        this.particles.forEach(p => {
            const na = nodeMap.get(p.sourceId);
            const nb = nodeMap.get(p.targetId);
            if (!na || !nb) return;
            if (p.weight < cutoff && !showMSTOnly) return;

            const px = na.x + (nb.x - na.x) * p.progress;
            const py = na.y + (nb.y - na.y) * p.progress;

            ctx.beginPath();
            ctx.arc(px, py, 2.8, 0, Math.PI * 2);
            ctx.fillStyle = (p.weight >= 60) ? '#EF4444' : ((p.weight >= 30) ? '#F59E0B' : '#38BDF8');
            ctx.shadowColor = ctx.fillStyle;
            ctx.shadowBlur = 8;
            ctx.fill();
            ctx.shadowBlur = 0;
        });

        // 3. Draw Shockwave Ripples
        this.rippleRings.forEach(r => {
            ctx.beginPath();
            ctx.arc(r.x, r.y, r.radius, 0, Math.PI * 2);
            ctx.strokeStyle = r.color;
            ctx.lineWidth = 2.5;
            ctx.globalAlpha = Math.max(0, r.opacity);
            ctx.stroke();
            ctx.globalAlpha = 1.0;
        });

        // 4. Draw Nodes
        this.nodes.forEach(n => {
            const isHovered = (hoveredGraphNode && hoveredGraphNode.id === n.id);
            const isSelected = (selectedGraphNode === n.id);
            const isNeighbor = focusedNeighbors.has(n.id);
            const isVisited = activeTraversal.visitedNodes.has(n.id);
            const isActiveTraversalStep = (activeTraversal.activeNode === n.id);

            let nodeAlpha = 1.0;
            if (isFocusMode && !isHovered && !isNeighbor) {
                nodeAlpha = 0.22;
            }

            ctx.save();
            ctx.globalAlpha = nodeAlpha;

            // Halo for Active or Hovered
            if (isActiveTraversalStep) {
                ctx.beginPath();
                ctx.arc(n.x, n.y, n.radius + 10, 0, Math.PI * 2);
                ctx.fillStyle = 'rgba(56, 189, 248, 0.35)';
                ctx.fill();
            } else if (isHovered || isSelected) {
                ctx.beginPath();
                ctx.arc(n.x, n.y, n.radius + 8, 0, Math.PI * 2);
                ctx.fillStyle = 'rgba(245, 158, 11, 0.28)';
                ctx.fill();
            }

            // Node Circle
            ctx.beginPath();
            ctx.arc(n.x, n.y, n.radius, 0, Math.PI * 2);

            // Styling based on state
            if (isActiveTraversalStep) {
                ctx.fillStyle = '#0284C7';
                ctx.strokeStyle = '#38BDF8';
                ctx.lineWidth = 3.5;
            } else if (isVisited) {
                ctx.fillStyle = '#065F46';
                ctx.strokeStyle = '#10B981';
                ctx.lineWidth = 3;
            } else if (isHovered || isSelected) {
                ctx.fillStyle = '#78350F';
                ctx.strokeStyle = '#F59E0B';
                ctx.lineWidth = 3.5;
            } else {
                ctx.fillStyle = '#1E293B';
                ctx.strokeStyle = '#475569';
                ctx.lineWidth = 2;
            }

            ctx.shadowColor = ctx.strokeStyle;
            ctx.shadowBlur = (isHovered || isSelected || isActiveTraversalStep) ? 14 : 4;
            ctx.fill();
            ctx.stroke();
            ctx.shadowBlur = 0;

            // Text Label: Document ID / Name
            ctx.fillStyle = '#FFFFFF';
            ctx.font = 'bold 11px Inter, sans-serif';
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText(isVisited ? ('Doc ' + n.id + ' ✓') : ('Doc ' + n.id), n.x, n.y);

            // Origin Flag Badge
            if (isSelected) {
                ctx.fillStyle = '#F59E0B';
                ctx.font = 'bold 9px JetBrains Mono, monospace';
                ctx.fillText('★ ORIGIN', n.x, n.y - n.radius - 8);
            }

            // Subtitle Document Name
            ctx.fillStyle = (isHovered || isSelected) ? '#F59E0B' : '#94A3B8';
            ctx.font = '10px Inter, sans-serif';
            const shortName = (n.name || '').substring(0, 16);
            ctx.fillText(shortName, n.x, n.y + n.radius + 14);

            ctx.restore();
        });
    }
}

// --- Canvas Interactions (Hover, Drag, Selection) ---
function initCanvasInteractions() {
    const canvas = document.getElementById('networkCanvas');
    if (!canvas) return;

    function getCanvasCoords(e) {
        const rect = canvas.getBoundingClientRect();
        const scaleX = canvas.width / rect.width;
        const scaleY = canvas.height / rect.height;
        return {
            x: (e.clientX - rect.left) * scaleX,
            y: (e.clientY - rect.top) * scaleY,
            screenX: e.clientX,
            screenY: e.clientY
        };
    }

    function findNodeAt(x, y) {
        if (!graphSimulator || !graphSimulator.nodes) return null;
        for (let i = graphSimulator.nodes.length - 1; i >= 0; i--) {
            const n = graphSimulator.nodes[i];
            const dx = x - n.x;
            const dy = y - n.y;
            if (dx * dx + dy * dy <= (n.radius + 16) * (n.radius + 16)) {
                return n;
            }
        }
        return null;
    }

    function findEdgeAt(x, y) {
        if (!graphSimulator || !graphSimulator.edges) return null;
        const nodeMap = new Map(graphSimulator.nodes.map(n => [n.id, n]));
        for (const e of graphSimulator.edges) {
            const na = nodeMap.get(e.source);
            const nb = nodeMap.get(e.target);
            if (!na || !nb) continue;

            const l2 = (nb.x - na.x) * (nb.x - na.x) + (nb.y - na.y) * (nb.y - na.y);
            if (l2 === 0) continue;
            let t = ((x - na.x) * (nb.x - na.x) + (y - na.y) * (nb.y - na.y)) / l2;
            t = Math.max(0, Math.min(1, t));
            const projX = na.x + t * (nb.x - na.x);
            const projY = na.y + t * (nb.y - na.y);
            const distSq = (x - projX) * (x - projX) + (y - projY) * (y - projY);

            if (distSq <= 36) { // 6px tolerance
                return { ...e, sourceName: na.name, targetName: nb.name };
            }
        }
        return null;
    }

    const tooltipEl = document.getElementById('graphTooltip');

    canvas.addEventListener('mousemove', (e) => {
        const coords = getCanvasCoords(e);

        if (isDraggingNode && draggedGraphNode) {
            draggedGraphNode.x = coords.x;
            draggedGraphNode.y = coords.y;
            draggedGraphNode.vx = 0;
            draggedGraphNode.vy = 0;
            if (graphSimulator) graphSimulator.wake();
            return;
        }

        const node = findNodeAt(coords.x, coords.y);
        const edge = node ? null : findEdgeAt(coords.x, coords.y);

        hoveredGraphNode = node;
        hoveredGraphEdge = edge;

        if (node) {
            canvas.style.cursor = 'pointer';
            if (tooltipEl) {
                let highestSim = 0;
                let highestMatchName = 'None';
                let incidentCount = 0;
                if (networkGraphData && networkGraphData.edges) {
                    networkGraphData.edges.forEach(ed => {
                        if (ed.source === node.id || ed.target === node.id) {
                            incidentCount++;
                            if (ed.weight > highestSim) {
                                highestSim = ed.weight;
                                const otherId = (ed.source === node.id) ? ed.target : ed.source;
                                const otherNode = graphSimulator.nodes.find(on => on.id === otherId);
                                highestMatchName = otherNode ? otherNode.name : ('Doc ' + otherId);
                            }
                        }
                    });
                }

                tooltipEl.innerHTML = `
                    <div class="tt-title">📄 ${escapeHtml(node.name)}</div>
                    <div class="tt-meta">Doc ID: #${node.id} • ${node.wordCount.toLocaleString()} words • ${incidentCount} connections</div>
                    ${highestSim > 0 ? `<div class="tt-match">Max Similarity: ${highestSim.toFixed(1)}% with ${escapeHtml(highestMatchName.substring(0, 18))}</div>` : ''}
                    <div class="tt-action">💡 Click to set as Traversal Origin • Drag to move</div>
                `;
                tooltipEl.style.display = 'block';
                tooltipEl.style.left = (coords.screenX - canvas.getBoundingClientRect().left + 14) + 'px';
                tooltipEl.style.top = (coords.screenY - canvas.getBoundingClientRect().top + 14) + 'px';
            }
            if (graphSimulator) graphSimulator.wake();
        } else if (edge) {
            canvas.style.cursor = 'pointer';
            if (tooltipEl) {
                tooltipEl.innerHTML = `
                    <div class="tt-title">⚡ Pairwise Similarity</div>
                    <div class="tt-match" style="color:#F59E0B; border-color:#F59E0B;">${edge.weight.toFixed(1)}% Jaccard Match</div>
                    <div class="tt-meta">${escapeHtml(edge.sourceName)} ↔ ${escapeHtml(edge.targetName)}</div>
                    <div class="tt-action">💡 Click Extract-Max or compare documents</div>
                `;
                tooltipEl.style.display = 'block';
                tooltipEl.style.left = (coords.screenX - canvas.getBoundingClientRect().left + 14) + 'px';
                tooltipEl.style.top = (coords.screenY - canvas.getBoundingClientRect().top + 14) + 'px';
            }
            if (graphSimulator) graphSimulator.wake();
        } else {
            canvas.style.cursor = 'default';
            if (tooltipEl) tooltipEl.style.display = 'none';
        }
    });

    canvas.addEventListener('mousedown', (e) => {
        const coords = getCanvasCoords(e);
        const node = findNodeAt(coords.x, coords.y);
        if (node) {
            isDraggingNode = true;
            draggedGraphNode = node;
            node.isFixed = true;
            if (graphSimulator) graphSimulator.wake();
        }
    });

    window.addEventListener('mouseup', () => {
        if (isDraggingNode && draggedGraphNode) {
            draggedGraphNode.isFixed = false;
            isDraggingNode = false;
            draggedGraphNode = null;
            if (graphSimulator) graphSimulator.wake();
        }
    });

    canvas.addEventListener('mouseleave', () => {
        hoveredGraphNode = null;
        hoveredGraphEdge = null;
        if (tooltipEl) tooltipEl.style.display = 'none';
        if (isDraggingNode && draggedGraphNode) {
            draggedGraphNode.isFixed = false;
            isDraggingNode = false;
            draggedGraphNode = null;
        }
    });

    canvas.addEventListener('click', (e) => {
        const coords = getCanvasCoords(e);
        const node = findNodeAt(coords.x, coords.y);
        if (node) {
            selectedGraphNode = node.id;
            const sel = document.getElementById('graphOriginSelect');
            if (sel) sel.value = String(node.id);

            const hudOrigin = document.getElementById('graphHudOrigin');
            if (hudOrigin) hudOrigin.textContent = `Origin: Doc #${node.id} (${node.name.substring(0, 14)})`;

            if (graphSimulator) {
                graphSimulator.triggerShockwave(node.id, '#F59E0B');
                graphSimulator.wake();
            }

            showToast(`Set Doc #${node.id} (${node.name.substring(0, 16)}) as Traversal Origin`, 'info');
        }
    });

    canvas.addEventListener('dblclick', (e) => {
        const coords = getCanvasCoords(e);
        const node = findNodeAt(coords.x, coords.y);
        if (node && networkGraphData && networkGraphData.edges) {
            let maxEdge = null;
            networkGraphData.edges.forEach(ed => {
                if (ed.source === node.id || ed.target === node.id) {
                    if (!maxEdge || ed.weight > maxEdge.weight) maxEdge = ed;
                }
            });
            if (maxEdge) {
                inspectPairComparison(maxEdge.source, maxEdge.target);
            }
        }
    });
}

// --- Main Graph Loader ---
async function loadNetworkGraph(threshold = 0) {
    try {
        const res = await fetch('/api/graph?threshold=' + threshold);
        if (!res.ok) throw new Error('Network graph HTTP ' + res.status);
        networkGraphData = await res.json();

        // 1. Update Telemetry Cards
        const totalNodes = networkGraphData.nodes.length;
        const totalEdges = networkGraphData.edges.length;
        const totalClusters = (networkGraphData.components || []).length;

        const nodeEl = document.getElementById('graphTotalNodes');
        const edgeEl = document.getElementById('graphTotalEdges');
        const clusterEl = document.getElementById('graphTotalClusters');
        const heapEl = document.getElementById('graphHeapSize');

        if (nodeEl) nodeEl.textContent = totalNodes;
        if (edgeEl) edgeEl.textContent = totalEdges;
        if (clusterEl) clusterEl.textContent = totalClusters;
        if (heapEl) heapEl.textContent = totalEdges;

        // 2. Populate Origin & Target Selector Dropdowns
        const originSelect = document.getElementById('graphOriginSelect');
        const targetSelect = document.getElementById('graphTargetSelect');
        if (originSelect) {
            const currentVal = originSelect.value;
            originSelect.innerHTML = '<option value="auto">Auto (Doc 1)</option>';
            networkGraphData.nodes.forEach(n => {
                const opt = document.createElement('option');
                opt.value = String(n.id);
                opt.textContent = `Doc #${n.id} — ${n.name.substring(0, 20)}`;
                originSelect.appendChild(opt);
            });
            if (currentVal && currentVal !== 'auto') originSelect.value = currentVal;
            else if (selectedGraphNode) originSelect.value = String(selectedGraphNode);
        }

        if (targetSelect) {
            const currentTgt = targetSelect.value;
            targetSelect.innerHTML = '<option value="auto">Auto (Doc 4)</option>';
            networkGraphData.nodes.forEach(n => {
                const opt = document.createElement('option');
                opt.value = String(n.id);
                opt.textContent = `Doc #${n.id} — ${n.name.substring(0, 20)}`;
                targetSelect.appendChild(opt);
            });
            if (currentTgt && currentTgt !== 'auto') targetSelect.value = currentTgt;
        }

        // 3. Render Max-Heap Ladder
        renderMaxHeap(networkGraphData);

        // 4. Compute Kruskal MST edges
        computeKruskalMST(networkGraphData.nodes, networkGraphData.edges);

        // 5. Initialize & Start Canvas Simulator
        const canvas = document.getElementById('networkCanvas');
        if (canvas) {
            if (!graphSimulator) {
                graphSimulator = new GraphPhysicsEngine(canvas);
            }
            graphSimulator.setGraph(networkGraphData.nodes, networkGraphData.edges, activeGraphLayout);
        }

        // 6. Render Cluster Pills
        renderClusters(networkGraphData.components);

    } catch (e) {
        console.error('Failed to load similarity network:', e);
    }
}

// --- Max-Heap Priority Queue Visualization ---
function renderMaxHeap(graphData) {
    if (!graphData || !graphData.edges) return;

    // Build Max-Heap array: sort pairs descending by similarity weight
    heapRankedEntries = [...graphData.edges].sort((a, b) => b.weight - a.weight);

    const rootBanner = document.getElementById('heapRootInfo');
    const rankedList = document.getElementById('heapRankedList');

    if (heapRankedEntries.length === 0) {
        if (rootBanner) {
            rootBanner.innerHTML = '<strong>No similarity pairs above threshold</strong><span>Adjust cutoff slider to populate max-heap</span>';
        }
        if (rankedList) {
            rankedList.innerHTML = '<div style="padding:12px; font-size:12px; color:var(--text-muted); text-align:center;">Heap empty at current cutoff.</div>';
        }
        return;
    }

    const nodeMap = new Map(graphData.nodes.map(n => [n.id, n]));
    const rootPair = heapRankedEntries[0];
    const srcNode = nodeMap.get(rootPair.source) || { name: 'Doc ' + rootPair.source };
    const tgtNode = nodeMap.get(rootPair.target) || { name: 'Doc ' + rootPair.target };

    // Update Heap Root Banner
    if (rootBanner) {
        rootBanner.innerHTML = `
            <div style="display:flex; justify-content:space-between; align-items:center; width:100%;">
                <div>
                    <strong style="font-size:14px; color:var(--text-primary); display:block;">${escapeHtml(srcNode.name)} ↔ ${escapeHtml(tgtNode.name)}</strong>
                    <span style="font-size:11.5px; color:#F59E0B; font-weight:700;">Score: ${rootPair.weight.toFixed(1)}% Jaccard Overlap</span>
                </div>
                <button class="btn btn-xs btn-primary" onclick="inspectPairComparison(${rootPair.source}, ${rootPair.target})">
                    <span>⚡ Side-by-Side</span>
                </button>
            </div>
        `;
    }

    // Render Top-K Ranked Pairs List
    if (rankedList) {
        rankedList.innerHTML = '';
        const topK = heapRankedEntries.slice(0, 8);

        topK.forEach((item, idx) => {
            const na = nodeMap.get(item.source) || { name: 'Doc ' + item.source };
            const nb = nodeMap.get(item.target) || { name: 'Doc ' + item.target };

            let medalClass = 'medal-default';
            let medalText = '#' + (idx + 1);
            if (idx === 0) { medalClass = 'medal-gold'; medalText = '🥇'; }
            else if (idx === 1) { medalClass = 'medal-silver'; medalText = '🥈'; }
            else if (idx === 2) { medalClass = 'medal-bronze'; medalText = '🥉'; }

            let scoreClass = 'score-low';
            if (item.weight >= 60) scoreClass = 'score-high';
            else if (item.weight >= 30) scoreClass = 'score-med';

            const div = document.createElement('div');
            div.className = 'heap-rank-item';
            div.innerHTML = `
                <div class="heap-rank-left">
                    <div class="heap-medal ${medalClass}">${medalText}</div>
                    <div>
                        <div class="heap-pair-names">${escapeHtml(na.name.substring(0, 16))} ↔ ${escapeHtml(nb.name.substring(0, 16))}</div>
                        <div class="heap-pair-meta">Pair IDs: [${item.source}, ${item.target}] • Heap Index: [${idx}]</div>
                    </div>
                </div>
                <div style="display:flex; align-items:center; gap:8px;">
                    <span class="heap-score-badge ${scoreClass}">${item.weight.toFixed(1)}%</span>
                    <button class="btn btn-xs btn-secondary" onclick="inspectPairComparison(${item.source}, ${item.target})" title="Open in Side-by-Side Diff">Inspect</button>
                </div>
            `;
            rankedList.appendChild(div);
        });
    }
}

// --- Extract-Max Heap Operation (O(log N) Animated Demo) ---
function extractMaxHeapItem() {
    if (!heapRankedEntries || heapRankedEntries.length === 0) {
        showToast('Max-Heap is currently empty. Lower the cutoff threshold to populate.', 'warning');
        return;
    }

    // O(log N) Binary Heap Extraction Simulation
    const extracted = heapRankedEntries.shift(); // Pop root max
    const heapSizeEl = document.getElementById('graphHeapSize');
    if (heapSizeEl) heapSizeEl.textContent = heapRankedEntries.length;

    // Trigger golden shockwave on both nodes in canvas
    if (graphSimulator) {
        graphSimulator.triggerShockwave(extracted.source, '#F59E0B');
        graphSimulator.triggerShockwave(extracted.target, '#10B981');
    }

    // Re-render heap with new promoted root (Heapify-Down)
    renderMaxHeap(networkGraphData);

    const nodeMap = new Map((networkGraphData?.nodes || []).map(n => [n.id, n]));
    const srcName = nodeMap.get(extracted.source)?.name || ('Doc ' + extracted.source);
    const tgtName = nodeMap.get(extracted.target)?.name || ('Doc ' + extracted.target);

    showToast(`🏆 Extracted Max Pair [${srcName.substring(0, 14)} ↔ ${tgtName.substring(0, 14)}] (${extracted.weight.toFixed(1)}%) in O(log N) heapify operations!`, 'success');
}

// --- Kruskal Maximum Spanning Tree (MST) ---
function computeKruskalMST(nodes, edges) {
    mstEdgeKeys.clear();
    if (!nodes || !edges || nodes.length === 0) return;

    // Disjoint Set Union (Union-Find)
    const parent = new Map();
    nodes.forEach(n => parent.set(n.id, n.id));

    function find(i) {
        if (parent.get(i) === i) return i;
        const root = find(parent.get(i));
        parent.set(i, root);
        return root;
    }

    function union(i, j) {
        const rootI = find(i);
        const rootJ = find(j);
        if (rootI !== rootJ) {
            parent.set(rootI, rootJ);
            return true;
        }
        return false;
    }

    // Sort edges descending by weight for Maximum Spanning Tree
    const sortedEdges = [...edges].sort((a, b) => b.weight - a.weight);

    sortedEdges.forEach(e => {
        if (union(e.source, e.target)) {
            const minId = Math.min(e.source, e.target);
            const maxId = Math.max(e.source, e.target);
            mstEdgeKeys.add(minId + '-' + maxId);
        }
    });
}

function toggleSpanningTree() {
    showMSTOnly = !showMSTOnly;
    const btn = document.getElementById('btnLayoutMST');
    const pill = document.getElementById('mstLegendPill');

    if (btn) btn.classList.toggle('active', showMSTOnly);
    if (pill) pill.style.display = showMSTOnly ? 'inline-flex' : 'none';

    if (graphSimulator) {
        graphSimulator.wake();
    }

    showToast(showMSTOnly ? '🌲 Maximum Spanning Tree (MST) Active: Filtered cycles to essential collusion backbone.' : 'Reset to Full Pairwise Similarity Graph.', 'info');
}

// --- Layout Switcher ---
function switchGraphLayout(layoutName) {
    activeGraphLayout = layoutName;
    document.querySelectorAll('.graph-canvas-controls .btn').forEach(b => {
        if (b.id !== 'btnLayoutMST') b.classList.remove('active');
    });

    if (layoutName === 'physics') {
        document.getElementById('btnLayoutPhysics')?.classList.add('active');
    } else if (layoutName === 'circle') {
        document.getElementById('btnLayoutCircle')?.classList.add('active');
    }

    if (graphSimulator) {
        graphSimulator.wake();
    }
}

// --- Origin Change & Search ---
function onOriginNodeChanged(val) {
    if (val === 'auto') {
        selectedGraphNode = null;
    } else {
        selectedGraphNode = parseInt(val);
    }

    const hudOrigin = document.getElementById('graphHudOrigin');
    if (hudOrigin) {
        const node = graphSimulator?.nodes.find(n => n.id === selectedGraphNode);
        hudOrigin.textContent = node ? `Origin: Doc #${node.id} (${node.name.substring(0, 14)})` : 'Origin: Auto (Highest Match)';
    }

    if (graphSimulator && selectedGraphNode) {
        graphSimulator.triggerShockwave(selectedGraphNode, '#F59E0B');
    }
}


function onTargetNodeChanged(val) {
    if (val !== 'auto' && graphSimulator) {
        const targetId = parseInt(val);
        graphSimulator.triggerShockwave(targetId, '#38BDF8');
    }
}

function animateSelectedPairFlow() {
    if (!networkGraphData || networkGraphData.nodes.length < 2) {
        showToast('At least two documents required to animate pair flow.', 'warning');
        return;
    }

    const originSel = document.getElementById('graphOriginSelect');
    const targetSel = document.getElementById('graphTargetSelect');

    let originId = (originSel && originSel.value !== 'auto') ? parseInt(originSel.value) : (selectedGraphNode || networkGraphData.nodes[0].id);
    let targetId = (targetSel && targetSel.value !== 'auto') ? parseInt(targetSel.value) : null;

    if (!targetId || targetId === originId) {
        // Pick top neighbor or next document
        const otherNode = networkGraphData.nodes.find(n => n.id !== originId);
        targetId = otherNode ? otherNode.id : networkGraphData.nodes[1].id;
    }

    selectedGraphNode = originId;
    if (originSel) originSel.value = String(originId);
    if (targetSel) targetSel.value = String(targetId);

    // Find similarity score between them
    let pairScore = 0.0;
    networkGraphData.edges.forEach(e => {
        if ((e.source === originId && e.target === targetId) || (e.source === targetId && e.target === originId)) {
            pairScore = e.weight;
        }
    });

    const nodeA = networkGraphData.nodes.find(n => n.id === originId) || { name: 'Doc ' + originId };
    const nodeB = networkGraphData.nodes.find(n => n.id === targetId) || { name: 'Doc ' + targetId };

    // Trigger double shockwaves and wake simulation
    if (graphSimulator) {
        graphSimulator.triggerShockwave(originId, '#F59E0B');
        graphSimulator.triggerShockwave(targetId, '#38BDF8');

        // Add 8 rapid high-speed stream particles along this exact pair
        for (let i = 0; i < 8; i++) {
            graphSimulator.particles.push({
                sourceId: originId,
                targetId: targetId,
                weight: Math.max(pairScore, 50.0),
                progress: i * 0.125,
                speed: 0.018 // high-speed stream
            });
        }
        graphSimulator.wake();
    }

    // Update traversal sequence log and HUD
    const logBox = document.getElementById('traversalSequenceDisplay');
    if (logBox) {
        logBox.innerHTML = `<strong>⚡ Active Pair Stream:</strong> <span style="color:#F59E0B; font-weight:800;">Doc #${originId} (${nodeA.name.substring(0, 14)})</span> ➔ <span style="color:#38BDF8; font-weight:800;">Doc #${targetId} (${nodeB.name.substring(0, 14)})</span>  |  <strong>Score: ${pairScore.toFixed(1)}% Jaccard Match</strong>`;
    }

    const hudOrigin = document.getElementById('graphHudOrigin');
    if (hudOrigin) {
        hudOrigin.textContent = `Streaming: Doc #${originId} ➔ Doc #${targetId} (${pairScore.toFixed(1)}%)`;
    }

    showToast(`Streaming pair comparison flow between "${nodeA.name.substring(0, 14)}" and "${nodeB.name.substring(0, 14)}" (${pairScore.toFixed(1)}%)`, 'success');
}

function onGraphSearch(query) {
    const q = (query || '').trim().toLowerCase();
    if (!graphSimulator) return;

    if (!q) {
        hoveredGraphNode = null;
        graphSimulator.wake();
        return;
    }

    const matched = graphSimulator.nodes.find(n =>
        n.name.toLowerCase().includes(q) || String(n.id) === q
    );

    if (matched) {
        hoveredGraphNode = matched;
        selectedGraphNode = matched.id;
        graphSimulator.triggerShockwave(matched.id, '#38BDF8');
    }
}

function resetNetworkView() {
    selectedGraphNode = null;
    hoveredGraphNode = null;
    hoveredGraphEdge = null;
    showMSTOnly = false;
    activeGraphLayout = 'physics';

    const slider = document.getElementById('graphThresholdSlider');
    const display = document.getElementById('graphThresholdVal');
    const originSel = document.getElementById('graphOriginSelect');
    const searchInput = document.getElementById('graphSearchInput');
    const btnMST = document.getElementById('btnLayoutMST');
    const mstPill = document.getElementById('mstLegendPill');

    if (slider) slider.value = 20;
    if (display) display.textContent = '20%';
    if (originSel) originSel.value = 'auto';
    if (searchInput) searchInput.value = '';
    if (btnMST) btnMST.classList.remove('active');
    if (mstPill) mstPill.style.display = 'none';

    switchGraphLayout('physics');
    loadNetworkGraph(0);
    showToast('Similarity network layout and filters reset.', 'info');
}

// --- Breadth-First Search (BFS) & Depth-First Search (DFS) Traversal Engine ---
async function runGraphBFS() {
    if (!networkGraphData || networkGraphData.nodes.length === 0) {
        showToast('No documents in network. Upload documents first.', 'warning');
        return;
    }

    // Determine start vertex
    let startNode = selectedGraphNode;
    if (!startNode) {
        startNode = heapRankedEntries.length > 0 ? heapRankedEntries[0].source : networkGraphData.nodes[0].id;
    }

    const threshold = document.getElementById('graphThresholdSlider')?.value || 20;
    const badge = document.getElementById('traversalModeBadge');
    if (badge) {
        badge.innerHTML = '<span style="color:#F59E0B; font-weight:bold;">RUNNING BFS (Queue)</span>';
    }

    try {
        const res = await fetch('/api/graph/bfs?start=' + startNode + '&threshold=' + threshold);
        let traversal = [];
        if (res.ok) {
            const data = await res.json();
            traversal = data.traversal || [];
        }

        // Client-side fallback if disconnected or component isolated
        if (!traversal || traversal.length <= 1) {
            traversal = clientSideBFS(startNode, threshold);
        }

        startAnimatedTraversal(traversal, 'BFS', 'FIFO QUEUE', startNode);
    } catch (e) {
        console.warn('API BFS failed, running client-side BFS:', e);
        const traversal = clientSideBFS(startNode, threshold);
        startAnimatedTraversal(traversal, 'BFS', 'FIFO QUEUE', startNode);
    }
}

async function runGraphDFS() {
    if (!networkGraphData || networkGraphData.nodes.length === 0) {
        showToast('No documents in network. Upload documents first.', 'warning');
        return;
    }

    let startNode = selectedGraphNode;
    if (!startNode) {
        startNode = heapRankedEntries.length > 0 ? heapRankedEntries[0].source : networkGraphData.nodes[0].id;
    }

    const threshold = document.getElementById('graphThresholdSlider')?.value || 20;
    const badge = document.getElementById('traversalModeBadge');
    if (badge) {
        badge.innerHTML = '<span style="color:#38BDF8; font-weight:bold;">RUNNING DFS (Stack)</span>';
    }

    try {
        const res = await fetch('/api/graph/dfs?start=' + startNode + '&threshold=' + threshold);
        let traversal = [];
        if (res.ok) {
            const data = await res.json();
            traversal = data.traversal || [];
        }

        if (!traversal || traversal.length <= 1) {
            traversal = clientSideDFS(startNode, threshold);
        }

        startAnimatedTraversal(traversal, 'DFS', 'LIFO STACK', startNode);
    } catch (e) {
        console.warn('API DFS failed, running client-side DFS:', e);
        const traversal = clientSideDFS(startNode, threshold);
        startAnimatedTraversal(traversal, 'DFS', 'LIFO STACK', startNode);
    }
}

// Deterministic client-side graph traversal algorithms
function clientSideBFS(startId, threshold = 0) {
    if (!networkGraphData) return [startId];
    const adj = new Map();
    networkGraphData.nodes.forEach(n => adj.set(n.id, []));
    networkGraphData.edges.forEach(e => {
        if (e.weight >= threshold) {
            adj.get(e.source)?.push(e.target);
            adj.get(e.target)?.push(e.source);
        }
    });

    if ((adj.get(startId) || []).length === 0) {
        networkGraphData.edges.forEach(e => {
            adj.get(e.source)?.push(e.target);
            adj.get(e.target)?.push(e.source);
        });
    }

    const visited = new Set();
    const queue = [startId];
    visited.add(startId);
    const order = [];

    while (queue.length > 0) {
        const u = queue.shift();
        order.push(u);
        const neighbors = adj.get(u) || [];
        for (const v of neighbors) {
            if (!visited.has(v)) {
                visited.add(v);
                queue.push(v);
            }
        }
    }
    return order;
}

function clientSideDFS(startId, threshold = 0) {
    if (!networkGraphData) return [startId];
    const adj = new Map();
    networkGraphData.nodes.forEach(n => adj.set(n.id, []));
    networkGraphData.edges.forEach(e => {
        if (e.weight >= threshold) {
            adj.get(e.source)?.push(e.target);
            adj.get(e.target)?.push(e.source);
        }
    });

    if ((adj.get(startId) || []).length === 0) {
        networkGraphData.edges.forEach(e => {
            adj.get(e.source)?.push(e.target);
            adj.get(e.target)?.push(e.source);
        });
    }

    const visited = new Set();
    const stack = [startId];
    const order = [];

    while (stack.length > 0) {
        const u = stack.pop();
        if (!visited.has(u)) {
            visited.add(u);
            order.push(u);
            const neighbors = adj.get(u) || [];
            for (const v of neighbors) {
                if (!visited.has(v)) stack.push(v);
            }
        }
    }
    return order;
}

// --- Live Step-by-Step Traversal Controller with Memory Tape ---
function startAnimatedTraversal(traversal, algType, structName, startNode) {
    if (activeTraversal.timer) clearInterval(activeTraversal.timer);

    activeTraversal.running = true;
    activeTraversal.paused = false;
    activeTraversal.type = algType;
    activeTraversal.structName = structName;
    activeTraversal.sequence = traversal;
    activeTraversal.stepIdx = 0;
    activeTraversal.visitedNodes.clear();
    activeTraversal.traversedEdges.clear();
    activeTraversal.activeNode = null;
    activeTraversal.queueOrStack = [startNode];

    // UI elements
    const titleEl = document.getElementById('dsaStructTitle');
    const stepBtn = document.getElementById('btnStepTraversal');
    const pauseBtn = document.getElementById('btnPauseTraversal');
    const logBox = document.getElementById('traversalSequenceDisplay');

    if (titleEl) titleEl.textContent = `${structName} — O(1) OPERATIONS`;
    if (stepBtn) stepBtn.disabled = false;
    if (pauseBtn) {
        pauseBtn.disabled = false;
        pauseBtn.textContent = 'Pause ⏸';
    }
    if (logBox) logBox.innerHTML = `<strong>${algType} Sequence (${structName}):</strong> `;

    renderStructTape();

    activeTraversal.timer = setInterval(() => {
        if (!activeTraversal.paused) {
            executeTraversalStep();
        }
    }, activeTraversal.stepDelay);
}

function executeTraversalStep() {
    if (activeTraversal.stepIdx >= activeTraversal.sequence.length) {
        finishTraversal();
        return;
    }

    const currentId = activeTraversal.sequence[activeTraversal.stepIdx];
    const prevId = (activeTraversal.stepIdx > 0) ? activeTraversal.sequence[activeTraversal.stepIdx - 1] : null;

    activeTraversal.activeNode = currentId;
    activeTraversal.visitedNodes.add(currentId);

    if (prevId !== null) {
        activeTraversal.traversedEdges.add(prevId + '-' + currentId);
        activeTraversal.traversedEdges.add(currentId + '-' + prevId);
    }

    // Trigger shockwave pulse on canvas
    if (graphSimulator) {
        graphSimulator.triggerShockwave(currentId, activeTraversal.type === 'BFS' ? '#F59E0B' : '#38BDF8');
    }

    // Update Data Structure Tape
    if (activeTraversal.type === 'BFS') {
        if (activeTraversal.queueOrStack.length > 0) activeTraversal.queueOrStack.shift();
        const nextNeighbors = getUnvisitedNeighbors(currentId, activeTraversal.visitedNodes);
        nextNeighbors.forEach(nId => {
            if (!activeTraversal.queueOrStack.includes(nId)) activeTraversal.queueOrStack.push(nId);
        });
    } else {
        if (activeTraversal.queueOrStack.length > 0) activeTraversal.queueOrStack.pop();
        const nextNeighbors = getUnvisitedNeighbors(currentId, activeTraversal.visitedNodes);
        nextNeighbors.forEach(nId => {
            if (!activeTraversal.queueOrStack.includes(nId)) activeTraversal.queueOrStack.push(nId);
        });
    }

    renderStructTape();

    // Log Breadcrumbs
    const logBox = document.getElementById('traversalSequenceDisplay');
    if (logBox) {
        if (activeTraversal.stepIdx > 0) logBox.innerHTML += ' ➔ ';
        const color = (activeTraversal.type === 'BFS') ? '#F59E0B' : '#38BDF8';
        logBox.innerHTML += `<span style="color:${color}; font-weight:800; font-family:var(--font-mono);">Doc ${currentId}</span>`;
    }

    activeTraversal.stepIdx++;
}

function getUnvisitedNeighbors(nodeId, visitedSet) {
    if (!networkGraphData || !networkGraphData.edges) return [];
    const neighbors = [];
    networkGraphData.edges.forEach(e => {
        if (e.source === nodeId && !visitedSet.has(e.target)) neighbors.push(e.target);
        if (e.target === nodeId && !visitedSet.has(e.source)) neighbors.push(e.source);
    });
    return neighbors;
}

function renderStructTape() {
    const tape = document.getElementById('dsaStructTape');
    const countEl = document.getElementById('dsaStructCount');
    if (!tape) return;

    if (activeTraversal.queueOrStack.length === 0) {
        tape.innerHTML = '<span class="dsa-empty-cue">Data structure buffer empty (Frontier fully explored)</span>';
        if (countEl) countEl.textContent = '0 items';
        return;
    }

    if (countEl) countEl.textContent = `${activeTraversal.queueOrStack.length} items`;
    tape.innerHTML = '';

    activeTraversal.queueOrStack.forEach((id, idx) => {
        const item = document.createElement('div');
        let roleClass = 'normal';
        let label = `Doc ${id}`;

        if (activeTraversal.type === 'BFS') {
            if (idx === 0) { roleClass = 'front'; label = `FRONT: Doc ${id}`; }
            else if (idx === activeTraversal.queueOrStack.length - 1) { roleClass = 'rear'; label = `REAR: Doc ${id}`; }
        } else {
            if (idx === activeTraversal.queueOrStack.length - 1) { roleClass = 'front'; label = `TOP: Doc ${id}`; }
            else if (idx === 0) { roleClass = 'rear'; label = `BASE: Doc ${id}`; }
        }

        item.className = `dsa-struct-item ${roleClass}`;
        item.textContent = label;
        tape.appendChild(item);
    });
}

function stepTraversalForward() {
    if (activeTraversal.running) {
        executeTraversalStep();
    }
}

function toggleTraversalPause() {
    activeTraversal.paused = !activeTraversal.paused;
    const btn = document.getElementById('btnPauseTraversal');
    if (btn) btn.textContent = activeTraversal.paused ? 'Resume ▶' : 'Pause ⏸';
}

function setTraversalSpeed(ms) {
    activeTraversal.stepDelay = parseInt(ms) || 480;
    if (activeTraversal.running && !activeTraversal.paused) {
        clearInterval(activeTraversal.timer);
        activeTraversal.timer = setInterval(executeTraversalStep, activeTraversal.stepDelay);
    }
}

function finishTraversal() {
    if (activeTraversal.timer) clearInterval(activeTraversal.timer);
    activeTraversal.running = false;
    activeTraversal.activeNode = null;

    const pauseBtn = document.getElementById('btnPauseTraversal');
    const stepBtn = document.getElementById('btnStepTraversal');
    const badge = document.getElementById('traversalModeBadge');

    if (pauseBtn) pauseBtn.disabled = true;
    if (stepBtn) stepBtn.disabled = true;
    if (badge) badge.innerHTML = '<span class="badge badge-emerald">Complete ✓</span>';

    if (graphSimulator) graphSimulator.wake();

    showToast(`Explored all ${activeTraversal.sequence.length} reachable documents via ${activeTraversal.type} in O(V + E) time!`, 'success');
}

// Helper to open Side-by-Side comparison for any pair
async function inspectPairComparison(docAId, docBId) {
    showToast(`Analyzing pair: Doc #${docAId} vs Doc #${docBId}...`, 'info');
    await loadDocuments();

    const docA = documentsList.find(d => d.id === docAId) || { id: docAId, name: 'Document ' + docAId, originalContent: '' };
    const docB = documentsList.find(d => d.id === docBId) || { id: docBId, name: 'Document ' + docBId, originalContent: '' };

    try {
        const res = await fetch('/api/analyze-two', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ docAId, docBId, windowSize: 5 })
        });

        if (res.ok) {
            currentAnalysisData = await res.json();
            displayResults(currentAnalysisData, docA.name, docB.name);
            populateSideBySide(docA.name, docB.name, docA.originalContent || '', docB.originalContent || '', currentAnalysisData.passages || []);

            // Reveal Results and Side-by-Side tabs
            const navRes = document.getElementById('navResults');
            const navComp = document.getElementById('navComparison');
            if (navRes) navRes.style.display = 'flex';
            if (navComp) navComp.style.display = 'flex';

            switchTab('results');
            showToast(`Comparative audit complete: ${docA.name} vs ${docB.name} (${currentAnalysisData.jaccardScore.toFixed(1)}%)`, 'success');
        } else {
            // Fallback to Version Diff
            switchTab('version-diff');
            await populateDiffSelectors();
            const selA = document.getElementById('diffDocASelect');
            const selB = document.getElementById('diffDocBSelect');
            if (selA && selB) {
                selA.value = String(docAId);
                selB.value = String(docBId);
                executeVersionDiff();
            }
        }
    } catch (e) {
        console.error('Inspection failed:', e);
        switchTab('version-diff');
    }
}

function renderClusters(components) {
    const list = document.getElementById('graphClustersList');
    if (!list) return;
    list.innerHTML = '';
    if (!components || components.length === 0) {
        list.innerHTML = '<span style="font-size:11px; color:var(--text-muted);">No connected collusion clusters identified.</span>';
        return;
    }
    components.forEach((c, idx) => {
        const item = document.createElement('div');
        item.className = 'cluster-pill';
        item.style.cursor = 'pointer';
        item.title = 'Click to focus this collusion group';
        item.innerHTML = `<strong>Cluster #${idx+1}:</strong> Documents [${c.join(', ')}] • (${c.length} docs)`;
        item.onclick = () => {
            if (c.length > 0 && graphSimulator) {
                selectedGraphNode = c[0];
                graphSimulator.triggerShockwave(c[0], '#F59E0B');
                const sel = document.getElementById('graphOriginSelect');
                if (sel) sel.value = String(c[0]);
                showToast(`Focused Collusion Cluster #${idx+1} (${c.length} documents)`, 'info');
            }
        };
        list.appendChild(item);
    });
}

// 12. ANALYSIS HISTORY
// ==========================================================================
let fullHistoryData = [];

async function loadHistoryTable() {
    try {
        const res = await fetch('/api/history');
        if (res.ok) {
            fullHistoryData = await res.json();
            renderHistoryTable(fullHistoryData);
        }
    } catch (e) {
        console.error('History error:', e);
    }
}

function renderHistoryTable(items) {
    const tbody = document.querySelector('#historyTable tbody');
    if (!tbody) return;
    tbody.innerHTML = '';

    if (items.length === 0) {
        tbody.innerHTML = `
            <tr>
                <td colspan="9">
                    <div class="empty-state-3d">
                        <div class="empty-state-3d-icon">
                            <svg width="34" height="34" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>
                        </div>
                        <h4 class="empty-state-3d-title">No Audit History Recorded Yet</h4>
                        <p class="empty-state-3d-sub">Execute a pairwise comparison or multi-document collusion verification to record reproducible SQLite audit sessions.</p>
                        <button class="btn btn-primary btn-sm btn-glow" onclick="switchTab('new-analysis')">
                            <span>⚡ Launch New Analysis</span>
                        </button>
                    </div>
                </td>
            </tr>
        `;
        return;
    }

    items.forEach(h => {
        const tr = document.createElement('tr');
        let badgeColor = '#F59E0B';
        if (h.avgSimilarity >= 60) badgeColor = '#EF4444';
        else if (h.avgSimilarity >= 25) badgeColor = '#F59E0B';

        tr.innerHTML = `
            <td><strong>#${h.id}</strong></td>
            <td><strong>${h.documents || 'Document Pair'}</strong></td>
            <td><span class="badge badge-subtle">${h.type}</span></td>
            <td style="color:var(--text-secondary); font-size:12px;">${h.date}</td>
            <td><strong style="color:${badgeColor};">${h.avgSimilarity.toFixed(1)}%</strong></td>
            <td>${h.matchingPassages}</td>
            <td style="font-size:12px; color:var(--text-muted);">${h.processingTime} ms</td>
            <td><span style="color:#F59E0B; font-weight:600;">● ${h.status}</span></td>
            <td>
                <button class="btn btn-primary btn-sm" onclick="loadAnalysisDetail(${h.id})">Open</button>
                <button class="btn btn-secondary btn-sm" onclick="downloadPdfReport(${h.id})">PDF</button>
            </td>
        `;
        tbody.appendChild(tr);
    });
}

function filterHistoryTable(query) {
    const q = query.toLowerCase();
    const filtered = fullHistoryData.filter(h => (h.documents || '').toLowerCase().includes(q) || h.type.toLowerCase().includes(q));
    renderHistoryTable(filtered);
}

function viewReport(id) {
    showToast(`Opening HTML audit view for record #${id}...`, 'info');
    window.open(`/api/report?id=${id}`, '_blank');
}

function downloadPdfReport(id, btn) {
    if (btn) {
        const orig = btn.innerHTML;
        btn.innerHTML = '<span>⏳ Compiling...</span>';
        btn.disabled = true;
        setTimeout(() => {
            btn.innerHTML = orig;
            btn.disabled = false;
        }, 1600);
    }
    showToast(`Compiling deterministic 4-page academic audit PDF for Audit #${id}...`, 'info');
    window.open(`/api/report?id=${id}&format=pdf`, '_blank');
}

function exportReport() {
    if (currentAnalysisData) {
        showToast('Opening detailed audit manifest...', 'info');
        window.open(`/api/report?id=${currentAnalysisData.analysisId}`, '_blank');
    }
}

function exportPdfReport(btn) {
    if (!currentAnalysisData) {
        showToast('Please select or complete an analysis first.', 'error');
        return;
    }
    if (btn) {
        const orig = btn.innerHTML;
        btn.innerHTML = '<span>⏳ Compiling 4-Page PDF...</span>';
        btn.disabled = true;
        setTimeout(() => {
            btn.innerHTML = '<span>✓ PDF Ready</span>';
            setTimeout(() => {
                btn.innerHTML = orig;
                btn.disabled = false;
            }, 1200);
        }, 1400);
    }
    showToast('Compiling publication-grade PDF report with mathematical proofs & verbatim catalog...', 'success');
    window.open(`/api/report?id=${currentAnalysisData.analysisId}&format=pdf`, '_blank');
}

// ==========================================================================
// 13. VERSION COMPARISON (LCS DIFF)
// ==========================================================================
async function populateDiffSelectors() {
    await loadDocuments();
    const selA = document.getElementById('diffDocASelect');
    const selB = document.getElementById('diffDocBSelect');
    if (!selA || !selB) return;
    selA.innerHTML = '';
    selB.innerHTML = '';

    documentsList.forEach((d, i) => {
        const optA = new Option(d.name, d.id, i === 0, i === 0);
        const optB = new Option(d.name, d.id, i === 1, i === 1);
        selA.add(optA);
        selB.add(optB);
    });
}

async function executeVersionDiff() {
    const selA = document.getElementById('diffDocASelect');
    const selB = document.getElementById('diffDocBSelect');
    const docAId = parseInt(selA.value);
    const docBId = parseInt(selB.value);

    if (docAId === docBId) {
        alert('Please select two different document versions.');
        return;
    }

    try {
        const res = await fetch('/api/version-diff', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ docAId, docBId })
        });

        if (res.ok) {
            const data = await res.json();
            document.getElementById('diffStatsGrid').style.display = 'grid';
            document.getElementById('diffLcsLen').textContent = data.lcsLength;
            document.getElementById('diffSimRatio').textContent = data.similarityRatio.toFixed(1) + '%';
            document.getElementById('diffAdditions').textContent = `+${data.additions}`;
            document.getElementById('diffDeletions').textContent = `-${data.deletions}`;

            const pane = document.getElementById('diffViewerPane');
            let diffHtml = '';
            data.diffs.forEach(d => {
                if (d.type === 'ADDED') {
                    diffHtml += `<span class="diff-span-added">${escapeHtml(d.text)} </span>`;
                } else if (d.type === 'REMOVED') {
                    diffHtml += `<span class="diff-span-removed">${escapeHtml(d.text)} </span>`;
                } else {
                    diffHtml += `<span>${escapeHtml(d.text)} </span>`;
                }
            });
            pane.innerHTML = diffHtml;
        }
    } catch (e) {
        alert('Version diff error: ' + e.message);
    }
}

// ==========================================================================
// 14. SETTINGS
// ==========================================================================
function saveUserSettings() {
    const ngram = document.getElementById('settingsNgramDefault')?.value || 5;
    const thresh = document.getElementById('settingsThresholdDefault')?.value || 20;
    const slider = document.getElementById('ngramSlider');
    if (slider) slider.value = ngram;
    const gSlider = document.getElementById('graphThresholdSlider');
    if (gSlider) gSlider.value = thresh;
    alert('Settings saved successfully!');
}
