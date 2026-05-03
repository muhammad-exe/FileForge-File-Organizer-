#include "mainwindow.h"
#include "organizerworker.h"

#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPushButton>
#include <QScrollBar>
#include <QSplitter>
#include <QTextEdit>
#include <QThread>
#include <QUrl>
#include <QVBoxLayout>

// ── Custom central widget that paints a background image ─────────────────────
class BackgroundWidget : public QWidget
{
public:
    explicit BackgroundWidget(QWidget *parent = nullptr) : QWidget(parent) {}

    void setBackgroundImage(const QPixmap &px) {
        m_pixmap = px;
        setAttribute(Qt::WA_StyledBackground, m_pixmap.isNull());
        update();
    }

    void clearBackgroundImage() {
        m_pixmap = QPixmap();
        update();
    }

protected:
    void paintEvent(QPaintEvent *e) override {
        if (!m_pixmap.isNull()) {
            QPainter p(this);
            p.drawPixmap(rect(), m_pixmap.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        }
        QWidget::paintEvent(e);
    }

private:
    QPixmap m_pixmap;
};

// ─── helpers ────────────────────────────────────────────────────────────────

static QFrame *makeDivider()
{
    QFrame *f = new QFrame();
    f->setFrameShape(QFrame::HLine);
    f->setObjectName("divider");
    return f;
}

static QLabel *makeSectionTitle(const QString &text, QWidget *parent = nullptr)
{
    QLabel *lbl = new QLabel(text.toUpper(), parent);
    lbl->setObjectName("sectionTitle");
    return lbl;
}

static QLabel *makeStatValue(const QString &val, const QString &color, QWidget *parent = nullptr)
{
    QLabel *lbl = new QLabel(val, parent);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setObjectName("statValue");
    lbl->setStyleSheet(QString("color:%1; font-size:22px; font-weight:700;").arg(color));
    return lbl;
}

static QFrame *makeStatCard(QLabel *value, const QString &labelText, QWidget *parent = nullptr)
{
    QFrame *card = new QFrame(parent);
    card->setObjectName("statCard");
    QVBoxLayout *lay = new QVBoxLayout(card);
    lay->setContentsMargins(8, 8, 8, 8);
    lay->setSpacing(2);
    lay->addWidget(value);
    QLabel *lbl = new QLabel(labelText);
    lbl->setObjectName("statLabel");
    lbl->setAlignment(Qt::AlignCenter);
    lay->addWidget(lbl);
    return card;
}

static QWidget *makeCatRow(const QString &color, const QString &name,
                           QLabel *&countLabel, QWidget *parent = nullptr)
{
    QWidget *w = new QWidget(parent);
    w->setObjectName("catRow");
    QHBoxLayout *lay = new QHBoxLayout(w);
    lay->setContentsMargins(8, 4, 8, 4);
    lay->setSpacing(8);

    QLabel *dot = new QLabel();
    dot->setFixedSize(8, 8);
    dot->setStyleSheet(QString("background:%1; border-radius:4px;").arg(color));

    QLabel *nameLbl = new QLabel(name);
    nameLbl->setObjectName("catName");

    countLabel = new QLabel("0");
    countLabel->setObjectName("catCount");
    countLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    lay->addWidget(dot);
    lay->addWidget(nameLbl, 1);
    lay->addWidget(countLabel);
    return w;
}

// ─── MainWindow ─────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), organizer(&logger)
{
    setWindowTitle("FileForge");
    resize(1060, 720);
    setMinimumSize(900, 640);

    setupUI();
    setupStyles();
    initializeSessionStorage();
    resetStatsPanel();
    updateButtonStates();
}

void MainWindow::setupUI()
{
    bgWidget = new BackgroundWidget(this);
    setCentralWidget(bgWidget);

    QVBoxLayout *root = new QVBoxLayout(bgWidget);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Header ───────────────────────────────────────────────────────────────
    headerFrame = new QFrame();
    headerFrame->setObjectName("header");
    headerFrame->setFixedHeight(80);

    // Background image label (sits behind text, hidden by default)
    lblHeaderBg = new QLabel(headerFrame);
    lblHeaderBg->setObjectName("headerBg");
    lblHeaderBg->setGeometry(0, 0, 1060, 80);
    lblHeaderBg->setScaledContents(true);
    lblHeaderBg->hide();

    QVBoxLayout *headerLay = new QVBoxLayout(headerFrame);
    headerLay->setContentsMargins(24, 16, 24, 12);
    headerLay->setSpacing(3);
    QLabel *lblTitle = new QLabel("FileForge");
    lblTitle->setObjectName("lblTitle");
    QLabel *lblSub = new QLabel("Smart File Organizer");
    lblSub->setObjectName("lblSubtitle");
    headerLay->addWidget(lblTitle);
    headerLay->addWidget(lblSub);
    root->addWidget(headerFrame);

    // ── Toolbar ──────────────────────────────────────────────────────────────
    QFrame *toolbar = new QFrame();
    toolbar->setObjectName("toolbar");
    toolbar->setFixedHeight(52);
    QHBoxLayout *tbLay = new QHBoxLayout(toolbar);
    tbLay->setContentsMargins(14, 8, 14, 8);
    tbLay->setSpacing(8);

    lblPath = new QLabel("No folder selected");
    lblPath->setObjectName("pathBox");
    lblPath->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    btnBrowse   = new QPushButton("Browse");
    btnBrowse->setObjectName("btnSecondary");
    btnPreview  = new QPushButton("Preview");
    btnPreview->setObjectName("btnPreview");
    btnOrganize = new QPushButton("Organize");
    btnOrganize->setObjectName("btnPrimary");
    btnUndo     = new QPushButton("Undo Last");
    btnUndo->setObjectName("btnUndo");
    btnOpenLogs = new QPushButton("Open Logs");
    btnOpenLogs->setObjectName("btnSecondary");
    btnShowLog  = new QPushButton("View Log");
    btnShowLog->setObjectName("btnSecondary");

    cmbTheme = new QComboBox();
    cmbTheme->setObjectName("cmbTheme");
    cmbTheme->addItem("\U0001f311 Dark",        "dark");
    cmbTheme->addItem("\u2600\ufe0f Light",      "light");
    cmbTheme->addItem("\U0001f987 Batman",       "batman");
    cmbTheme->addItem("\U0001f338 Hello Kitty",  "hellokitty");
    cmbTheme->addItem("\U0001f30a Ocean",        "ocean");
    cmbTheme->addItem("\U0001f9db Dracula",      "dracula");
    cmbTheme->addItem("\U0001f5bc Custom Image", "custom");
    cmbTheme->setFixedWidth(150);

    tbLay->addWidget(lblPath, 1);
    tbLay->addWidget(btnBrowse);
    tbLay->addWidget(btnPreview);
    tbLay->addWidget(btnOrganize);
    tbLay->addWidget(btnUndo);
    tbLay->addWidget(btnOpenLogs);
    tbLay->addWidget(btnShowLog);
    tbLay->addWidget(cmbTheme);
    root->addWidget(toolbar);
    root->addWidget(makeDivider());

    // ── Body ─────────────────────────────────────────────────────────────────
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    splitter->setObjectName("bodySplitter");
    splitter->setHandleWidth(1);
    root->addWidget(splitter, 1);

    // ── Left: Preview List ───────────────────────────────────────────────────
    QWidget *leftWidget = new QWidget();
    leftWidget->setObjectName("leftPanel");
    QVBoxLayout *leftLay = new QVBoxLayout(leftWidget);
    leftLay->setContentsMargins(0, 0, 0, 0);
    leftLay->setSpacing(0);

    QWidget *previewHeader = new QWidget();
    previewHeader->setObjectName("panelHeader");
    QHBoxLayout *phLay = new QHBoxLayout(previewHeader);
    phLay->setContentsMargins(14, 10, 14, 8);
    phLay->setSpacing(8);
    phLay->addWidget(makeSectionTitle("Preview"));
    phLay->addStretch();
    lblFileCount = new QLabel("0 files");
    lblFileCount->setObjectName("countBadge");
    phLay->addWidget(lblFileCount);

    listPreview = new QListWidget();
    listPreview->setObjectName("previewList");
    listPreview->setSelectionMode(QAbstractItemView::NoSelection);
    listPreview->setFocusPolicy(Qt::NoFocus);
    listPreview->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    leftLay->addWidget(previewHeader);
    leftLay->addWidget(makeDivider());
    leftLay->addWidget(listPreview, 1);
    splitter->addWidget(leftWidget);

    // ── Right panel ──────────────────────────────────────────────────────────
    QWidget *rightWidget = new QWidget();
    rightWidget->setObjectName("rightPanel");
    QVBoxLayout *rightLay = new QVBoxLayout(rightWidget);
    rightLay->setContentsMargins(0, 0, 0, 0);
    rightLay->setSpacing(0);

    // Session Storage
    QWidget *sessionSec = new QWidget();
    sessionSec->setObjectName("section");
    QVBoxLayout *sessLay = new QVBoxLayout(sessionSec);
    sessLay->setContentsMargins(14, 10, 14, 10);
    sessLay->setSpacing(6);
    sessLay->addWidget(makeSectionTitle("Session Storage"));

    auto makeSessionRow = [&](const QString &label, QLabel *&valLabel) {
        QWidget *row = new QWidget();
        QHBoxLayout *rl = new QHBoxLayout(row);
        rl->setContentsMargins(0, 0, 0, 0);
        rl->setSpacing(10);
        QLabel *lbl = new QLabel(label);
        lbl->setObjectName("sessionKey");
        lbl->setFixedWidth(56);
        valLabel = new QLabel("—");
        valLabel->setObjectName("sessionVal");
        rl->addWidget(lbl);
        rl->addWidget(valLabel, 1);
        sessLay->addWidget(row);
    };
    makeSessionRow("Session:", lblSessionId);
    makeSessionRow("Path:", lblLogPath);
    lblLogPath->setWordWrap(true);

    rightLay->addWidget(sessionSec);
    rightLay->addWidget(makeDivider());

    // Statistics
    QWidget *statsSec = new QWidget();
    statsSec->setObjectName("section");
    QVBoxLayout *statsLay = new QVBoxLayout(statsSec);
    statsLay->setContentsMargins(14, 10, 14, 10);
    statsLay->setSpacing(10);
    statsLay->addWidget(makeSectionTitle("Statistics"));

    QHBoxLayout *bigStats = new QHBoxLayout();
    bigStats->setSpacing(8);
    lblScanned = makeStatValue("0", "#60a5fa");
    lblMoved   = makeStatValue("0", "#34d399");
    lblFailed  = makeStatValue("0", "#f87171");
    bigStats->addWidget(makeStatCard(lblScanned, "Scanned"));
    bigStats->addWidget(makeStatCard(lblMoved,   "Moved"));
    bigStats->addWidget(makeStatCard(lblFailed,  "Failed"));
    statsLay->addLayout(bigStats);

    QGridLayout *catGrid = new QGridLayout();
    catGrid->setSpacing(5);
    catGrid->setContentsMargins(0, 0, 0, 0);

    struct CatDef { QString color; QString name; QLabel **label; };
    CatDef cats[] = {
                     {"#60a5fa", "Documents", &lblDocuments},
                     {"#34d399", "Images",    &lblImages},
                     {"#f59e0b", "Audio",     &lblAudio},
                     {"#a78bfa", "Videos",    &lblVideos},
                     {"#f87171", "Archives",  &lblArchives},
                     {"#94a3b8", "Code",      &lblCode},
                     {"#38bdf8", "Projects",  &lblProjects},
                     {"#e2e8f0", "Other",     &lblOther},
                     };
    int col = 0, row = 0;
    for (auto &cat : cats) {
        QLabel *countLbl = nullptr;
        QWidget *catRowW = makeCatRow(cat.color, cat.name, countLbl);
        *cat.label = countLbl;
        catGrid->addWidget(catRowW, row, col);
        col++;
        if (col == 2) { col = 0; row++; }
    }
    statsLay->addLayout(catGrid);
    rightLay->addWidget(statsSec);
    rightLay->addWidget(makeDivider());

    // Activity Log
    QWidget *logSec = new QWidget();
    logSec->setObjectName("section");
    QVBoxLayout *logSecLay = new QVBoxLayout(logSec);
    logSecLay->setContentsMargins(14, 10, 14, 10);
    logSecLay->setSpacing(6);
    logSecLay->addWidget(makeSectionTitle("Activity Log"));
    txtLog = new QTextEdit();
    txtLog->setObjectName("logArea");
    txtLog->setReadOnly(true);
    txtLog->setMinimumHeight(140);
    logSecLay->addWidget(txtLog, 1);
    rightLay->addWidget(logSec, 1);

    splitter->addWidget(rightWidget);
    splitter->setSizes({420, 640});

    // ── Footer ───────────────────────────────────────────────────────────────
    QFrame *footer = new QFrame();
    footer->setObjectName("footer");
    footer->setFixedHeight(24);
    QHBoxLayout *footerLay = new QHBoxLayout(footer);
    footerLay->setContentsMargins(14, 0, 14, 0);
    footerLay->setSpacing(0);

    QLabel *lblFooterLeft = new QLabel("FileForge  ·  OOP  ·  Sir Ramzan Butt");
    lblFooterLeft->setObjectName("footerText");

    QLabel *lblFooterRight = new QLabel(
        "Khalitus Ali (F25CSC025)  ·  Vansh Raj (F25CSC012)  ·  Muhammad Bin Farooq (F25CSC003)  ·  Hubdar Ali (F25CSC006)"
        );
    lblFooterRight->setObjectName("footerText");
    lblFooterRight->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    footerLay->addWidget(lblFooterLeft, 1);
    footerLay->addWidget(lblFooterRight, 2);

    root->addWidget(footer);

    // Connections
    connect(btnBrowse,   &QPushButton::clicked, this, &MainWindow::selectFolder);
    connect(btnPreview,  &QPushButton::clicked, this, &MainWindow::previewFiles);
    connect(btnOrganize, &QPushButton::clicked, this, &MainWindow::organizeFiles);
    connect(btnUndo,     &QPushButton::clicked, this, &MainWindow::undoOrganize);
    connect(btnShowLog,  &QPushButton::clicked, this, &MainWindow::showLogContents);
    connect(btnOpenLogs, &QPushButton::clicked, this, &MainWindow::openLogsFolder);
    connect(cmbTheme, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onThemeChanged);
}

void MainWindow::setupStyles()
{
    onThemeChanged(0); // default: Dark
}

void MainWindow::onThemeChanged(int index)
{
    const QString key = cmbTheme->itemData(index).toString();

    if (key == "custom") {
        // Open file picker for image
        const QString imgPath = QFileDialog::getOpenFileName(
            this, "Choose Header Background Image", QDir::homePath(),
            "Images (*.png *.jpg *.jpeg *.bmp *.webp)");

        if (imgPath.isEmpty()) {
            // User cancelled — revert to dark
            cmbTheme->blockSignals(true);
            cmbTheme->setCurrentIndex(0);
            cmbTheme->blockSignals(false);
            applyTheme("dark");
            return;
        }
        m_customImagePath = imgPath;
        applyTheme("custom");
        return;
    }

    // Clear custom image when switching away
    if (key != "custom") {
        lblHeaderBg->hide();
        lblHeaderBg->clear();
    }

    applyTheme(key);
}

void MainWindow::applyTheme(const QString &key)
{
    // ── Custom image theme ───────────────────────────────────────────────────
    if (key == "custom") {
        if (!m_customImagePath.isEmpty()) {
            QPixmap px(m_customImagePath);
            if (!px.isNull()) {
                bgWidget->setBackgroundImage(px);
                setStyleSheet(R"(
QWidget{background:transparent;color:#ffffff;font-family:'Segoe UI',sans-serif;font-size:13px;}
#header{background:rgba(0,0,0,0.50);border-bottom:1px solid rgba(255,255,255,0.15);}
#lblTitle{font-size:22px;font-weight:700;color:#ffffff;}
#lblSubtitle{font-size:11px;color:rgba(255,255,255,0.75);}
#toolbar{background:rgba(0,0,0,0.60);border-bottom:1px solid rgba(255,255,255,0.12);}
#pathBox{background:rgba(0,0,0,0.45);border:1px solid rgba(255,255,255,0.25);border-radius:7px;padding:5px 10px;font-family:'Consolas',monospace;font-size:12px;color:#e2e8f0;}
QPushButton{border-radius:7px;padding:6px 14px;font-size:12px;font-weight:600;border:none;}
#btnPrimary{background:rgba(79,70,229,0.9);color:#fff;min-width:90px;}
#btnPrimary:hover{background:rgba(99,90,249,1.0);}
#btnPrimary:disabled{background:rgba(51,65,85,0.6);color:#94a3b8;}
#btnPreview{background:rgba(79,70,229,0.9);color:#fff;min-width:80px;}
#btnPreview:hover{background:rgba(99,90,249,1.0);}
#btnPreview:disabled{background:rgba(51,65,85,0.6);color:#94a3b8;}
#btnSecondary{background:rgba(20,28,45,0.75);color:#e2e8f0;border:1px solid rgba(255,255,255,0.25);}
#btnSecondary:hover{background:rgba(51,65,85,0.9);}
#btnSecondary:disabled{color:#64748b;}
#btnUndo{background:transparent;color:#f59e0b;border:1px solid #f59e0b;min-width:90px;}
#btnUndo:hover{background:rgba(245,158,11,0.2);}
#btnUndo:disabled{color:#64748b;border-color:#334155;}
QComboBox{background:rgba(20,28,45,0.8);color:#e2e8f0;border:1px solid rgba(255,255,255,0.25);border-radius:7px;padding:5px 10px;font-size:12px;font-weight:600;}
QComboBox::drop-down{border:none;width:20px;}
QComboBox QAbstractItemView{background:#1e2533;color:#e2e8f0;border:1px solid #475569;selection-background-color:#4f46e5;}
#bodySplitter::handle{background:rgba(255,255,255,0.1);width:1px;}
#leftPanel,#rightPanel{background:rgba(5,8,18,0.60);}
#panelHeader{background:rgba(0,0,0,0.45);}
#sectionTitle{font-size:10px;font-weight:700;letter-spacing:1.2px;color:#a5b4fc;}
#previewList{background:transparent;border:none;outline:none;padding:2px;}
#previewList::item{padding:7px 10px;border-bottom:1px solid rgba(255,255,255,0.07);color:#e2e8f0;font-size:12px;}
#previewList::item:hover{background:rgba(255,255,255,0.09);}
#countBadge{background:rgba(20,28,45,0.7);border-radius:9px;padding:1px 8px;font-size:10px;color:#94a3b8;}
#sessionKey{font-size:11px;color:rgba(255,255,255,0.45);}
#sessionVal{font-size:11px;color:#e2e8f0;font-family:'Consolas',monospace;}
#statCard{background:rgba(5,8,18,0.65);border:1px solid rgba(255,255,255,0.12);border-radius:8px;}
#statLabel{font-size:10px;color:rgba(255,255,255,0.5);}
#catRow{background:rgba(5,8,18,0.55);border:1px solid rgba(255,255,255,0.08);border-radius:6px;}
#catName{font-size:11px;color:rgba(255,255,255,0.6);}
#catCount{font-size:12px;font-weight:600;color:#ffffff;}
#logArea{background:rgba(0,0,0,0.60);border:1px solid rgba(255,255,255,0.1);border-radius:8px;color:#94a3b8;font-family:'Consolas',monospace;font-size:11px;padding:6px;}
QScrollBar:vertical{background:transparent;width:6px;border-radius:3px;}
QScrollBar::handle:vertical{background:rgba(255,255,255,0.25);border-radius:3px;min-height:20px;}
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}
#footer{background:rgba(0,0,0,0.55);border-top:1px solid rgba(255,255,255,0.1);}
#footerText{font-size:10px;color:rgba(255,255,255,0.35);}
                )");
            }
        }
        return;
    }

    // Reset background image for all other themes
    bgWidget->clearBackgroundImage();
    this->setPalette(QPalette());
    this->setAutoFillBackground(false);

    if (key == "dark") {
        setStyleSheet(R"(
QMainWindow,QWidget{background:#0f1117;color:#e2e8f0;font-family:'Segoe UI',sans-serif;font-size:13px;}
#header{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #171b2d,stop:1 #101826);border-bottom:1px solid #2d3748;}
#lblTitle{font-size:22px;font-weight:700;color:#f8fafc;}
#lblSubtitle{font-size:11px;color:#94a3b8;}
#toolbar{background:#131720;border-bottom:1px solid #2d3748;}
#pathBox{background:#0d1117;border:1px solid #2d3748;border-radius:7px;padding:5px 10px;font-family:'Consolas',monospace;font-size:12px;color:#94a3b8;}
QPushButton{border-radius:7px;padding:6px 14px;font-size:12px;font-weight:600;border:none;}
#btnPrimary{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #4f46e5,stop:1 #7c3aed);color:#fff;min-width:90px;}
#btnPrimary:hover{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #5b52f5,stop:1 #8c4cff);}
#btnPrimary:disabled{background:#334155;color:#64748b;}
#btnPreview{background:#4f46e5;color:#fff;min-width:80px;}
#btnPreview:hover{background:#5b52f5;}
#btnPreview:disabled{background:#334155;color:#64748b;}
#btnSecondary{background:#273142;color:#e2e8f0;border:1px solid #475569;}
#btnSecondary:hover{background:#334155;}
#btnSecondary:disabled{color:#64748b;border-color:#334155;}
#btnUndo{background:transparent;color:#f59e0b;border:1px solid #f59e0b;min-width:90px;}
#btnUndo:hover{background:rgba(245,158,11,0.13);}
#btnUndo:disabled{color:#64748b;border-color:#334155;}
QComboBox{background:#1e2533;color:#e2e8f0;border:1px solid #475569;border-radius:7px;padding:5px 10px;font-size:12px;font-weight:600;}
QComboBox::drop-down{border:none;width:20px;}
QComboBox QAbstractItemView{background:#1e2533;color:#e2e8f0;border:1px solid #475569;selection-background-color:#4f46e5;}
#bodySplitter::handle{background:#2d3748;width:1px;}
#leftPanel,#rightPanel{background:#0f1117;}
#panelHeader{background:#131720;}
#sectionTitle{font-size:10px;font-weight:700;letter-spacing:1.2px;color:#6366f1;}
#previewList{background:#0f1117;border:none;outline:none;padding:2px;}
#previewList::item{padding:7px 10px;border-bottom:1px solid #1e2533;color:#cbd5e1;font-size:12px;}
#previewList::item:hover{background:#1a1f2e;}
#countBadge{background:#1e2533;border-radius:9px;padding:1px 8px;font-size:10px;color:#64748b;}
#sessionKey{font-size:11px;color:#64748b;}
#sessionVal{font-size:11px;color:#cbd5e1;font-family:'Consolas',monospace;}
#statCard{background:#0d1117;border:1px solid #2d3748;border-radius:8px;}
#statLabel{font-size:10px;color:#64748b;}
#catRow{background:#0d1117;border:1px solid #1e2533;border-radius:6px;}
#catName{font-size:11px;color:#94a3b8;}
#catCount{font-size:12px;font-weight:600;color:#e2e8f0;}
#logArea{background:#0d1117;border:1px solid #2d3748;border-radius:8px;color:#64748b;font-family:'Consolas',monospace;font-size:11px;padding:6px;}
QScrollBar:vertical{background:#0d1117;width:6px;border-radius:3px;}
QScrollBar::handle:vertical{background:#334155;border-radius:3px;min-height:20px;}
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}
#footer{background:#0d1117;border-top:1px solid #1e2533;}
#footerText{font-size:10px;color:#3d4f66;}
        )");

    } else if (key == "light") {
        setStyleSheet(R"(
QMainWindow,QWidget{background:#f1f5f9;color:#1e293b;font-family:'Segoe UI',sans-serif;font-size:13px;}
#header{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #e8edf7,stop:1 #dde4f0);border-bottom:1px solid #cbd5e1;}
#lblTitle{font-size:22px;font-weight:700;color:#1e293b;}
#lblSubtitle{font-size:11px;color:#64748b;}
#toolbar{background:#e2e8f0;border-bottom:1px solid #cbd5e1;}
#pathBox{background:#fff;border:1px solid #cbd5e1;border-radius:7px;padding:5px 10px;font-family:'Consolas',monospace;font-size:12px;color:#475569;}
QPushButton{border-radius:7px;padding:6px 14px;font-size:12px;font-weight:600;border:none;}
#btnPrimary{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #4f46e5,stop:1 #7c3aed);color:#fff;min-width:90px;}
#btnPrimary:hover{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #5b52f5,stop:1 #8c4cff);}
#btnPrimary:disabled{background:#cbd5e1;color:#94a3b8;}
#btnPreview{background:#4f46e5;color:#fff;min-width:80px;}
#btnPreview:hover{background:#5b52f5;}
#btnPreview:disabled{background:#cbd5e1;color:#94a3b8;}
#btnSecondary{background:#fff;color:#334155;border:1px solid #cbd5e1;}
#btnSecondary:hover{background:#f8fafc;}
#btnSecondary:disabled{color:#94a3b8;border-color:#e2e8f0;}
#btnUndo{background:transparent;color:#d97706;border:1px solid #d97706;min-width:90px;}
#btnUndo:hover{background:rgba(217,119,6,0.10);}
#btnUndo:disabled{color:#94a3b8;border-color:#cbd5e1;}
QComboBox{background:#fff;color:#334155;border:1px solid #cbd5e1;border-radius:7px;padding:5px 10px;font-size:12px;font-weight:600;}
QComboBox::drop-down{border:none;width:20px;}
QComboBox QAbstractItemView{background:#fff;color:#334155;border:1px solid #cbd5e1;selection-background-color:#4f46e5;selection-color:#fff;}
#bodySplitter::handle{background:#cbd5e1;width:1px;}
#leftPanel,#rightPanel{background:#f1f5f9;}
#panelHeader{background:#e2e8f0;}
#sectionTitle{font-size:10px;font-weight:700;letter-spacing:1.2px;color:#6366f1;}
#previewList{background:#f1f5f9;border:none;outline:none;padding:2px;}
#previewList::item{padding:7px 10px;border-bottom:1px solid #e2e8f0;color:#334155;font-size:12px;}
#previewList::item:hover{background:#e8edf5;}
#countBadge{background:#e2e8f0;border-radius:9px;padding:1px 8px;font-size:10px;color:#64748b;}
#sessionKey{font-size:11px;color:#94a3b8;}
#sessionVal{font-size:11px;color:#334155;font-family:'Consolas',monospace;}
#statCard{background:#fff;border:1px solid #e2e8f0;border-radius:8px;}
#statLabel{font-size:10px;color:#94a3b8;}
#catRow{background:#fff;border:1px solid #e2e8f0;border-radius:6px;}
#catName{font-size:11px;color:#64748b;}
#catCount{font-size:12px;font-weight:600;color:#1e293b;}
#logArea{background:#fff;border:1px solid #e2e8f0;border-radius:8px;color:#475569;font-family:'Consolas',monospace;font-size:11px;padding:6px;}
QScrollBar:vertical{background:#f1f5f9;width:6px;border-radius:3px;}
QScrollBar::handle:vertical{background:#cbd5e1;border-radius:3px;min-height:20px;}
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}
#footer{background:#e2e8f0;border-top:1px solid #cbd5e1;}
#footerText{font-size:10px;color:#94a3b8;}
        )");

    } else if (key == "batman") {
        setStyleSheet(R"(
QMainWindow,QWidget{background:#0a0a0a;color:#f5c518;font-family:'Segoe UI',sans-serif;font-size:13px;}
#header{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #111111,stop:1 #0a0a0a);border-bottom:2px solid #f5c518;}
#lblTitle{font-size:22px;font-weight:700;color:#f5c518;letter-spacing:2px;}
#lblSubtitle{font-size:11px;color:#888;}
#toolbar{background:#111;border-bottom:1px solid #f5c518;}
#pathBox{background:#0a0a0a;border:1px solid #f5c518;border-radius:7px;padding:5px 10px;font-family:'Consolas',monospace;font-size:12px;color:#f5c518;}
QPushButton{border-radius:7px;padding:6px 14px;font-size:12px;font-weight:700;border:none;}
#btnPrimary{background:#f5c518;color:#000;min-width:90px;}
#btnPrimary:hover{background:#ffe033;}
#btnPrimary:disabled{background:#333;color:#666;}
#btnPreview{background:#f5c518;color:#000;min-width:80px;}
#btnPreview:hover{background:#ffe033;}
#btnPreview:disabled{background:#333;color:#666;}
#btnSecondary{background:#1a1a1a;color:#f5c518;border:1px solid #f5c518;}
#btnSecondary:hover{background:#222;}
#btnSecondary:disabled{color:#555;border-color:#333;}
#btnUndo{background:transparent;color:#f5c518;border:1px solid #f5c518;min-width:90px;}
#btnUndo:hover{background:rgba(245,197,24,0.15);}
#btnUndo:disabled{color:#555;border-color:#333;}
QComboBox{background:#1a1a1a;color:#f5c518;border:1px solid #f5c518;border-radius:7px;padding:5px 10px;font-size:12px;font-weight:700;}
QComboBox::drop-down{border:none;width:20px;}
QComboBox QAbstractItemView{background:#1a1a1a;color:#f5c518;border:1px solid #f5c518;selection-background-color:#f5c518;selection-color:#000;}
#bodySplitter::handle{background:#f5c518;width:1px;}
#leftPanel,#rightPanel{background:#0a0a0a;}
#panelHeader{background:#111;}
#sectionTitle{font-size:10px;font-weight:700;letter-spacing:1.2px;color:#f5c518;}
#previewList{background:#0a0a0a;border:none;outline:none;padding:2px;}
#previewList::item{padding:7px 10px;border-bottom:1px solid #1a1a1a;color:#ccc;font-size:12px;}
#previewList::item:hover{background:#111;}
#countBadge{background:#1a1a1a;border-radius:9px;padding:1px 8px;font-size:10px;color:#f5c518;border:1px solid #f5c518;}
#sessionKey{font-size:11px;color:#666;}
#sessionVal{font-size:11px;color:#f5c518;font-family:'Consolas',monospace;}
#statCard{background:#111;border:1px solid #f5c518;border-radius:8px;}
#statLabel{font-size:10px;color:#888;}
#catRow{background:#111;border:1px solid #222;border-radius:6px;}
#catName{font-size:11px;color:#aaa;}
#catCount{font-size:12px;font-weight:700;color:#f5c518;}
#logArea{background:#0a0a0a;border:1px solid #f5c518;border-radius:8px;color:#888;font-family:'Consolas',monospace;font-size:11px;padding:6px;}
QScrollBar:vertical{background:#0a0a0a;width:6px;border-radius:3px;}
QScrollBar::handle:vertical{background:#f5c518;border-radius:3px;min-height:20px;}
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}
#footer{background:#0a0a0a;border-top:2px solid #f5c518;}
#footerText{font-size:10px;color:#555;}
        )");

    } else if (key == "hellokitty") {
        setStyleSheet(R"(
QMainWindow,QWidget{background:#fff0f5;color:#c0006a;font-family:'Segoe UI',sans-serif;font-size:13px;}
#header{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #ffe4f0,stop:1 #ffd6e8);border-bottom:2px solid #ff80b5;}
#lblTitle{font-size:22px;font-weight:700;color:#c0006a;letter-spacing:1px;}
#lblSubtitle{font-size:11px;color:#ff80b5;}
#toolbar{background:#ffe4f0;border-bottom:1px solid #ff80b5;}
#pathBox{background:#fff;border:1px solid #ff80b5;border-radius:7px;padding:5px 10px;font-family:'Consolas',monospace;font-size:12px;color:#c0006a;}
QPushButton{border-radius:7px;padding:6px 14px;font-size:12px;font-weight:700;border:none;}
#btnPrimary{background:#ff4d94;color:#fff;min-width:90px;}
#btnPrimary:hover{background:#ff1a75;}
#btnPrimary:disabled{background:#ffb3d1;color:#fff;}
#btnPreview{background:#ff4d94;color:#fff;min-width:80px;}
#btnPreview:hover{background:#ff1a75;}
#btnPreview:disabled{background:#ffb3d1;color:#fff;}
#btnSecondary{background:#fff;color:#c0006a;border:1px solid #ff80b5;}
#btnSecondary:hover{background:#fff0f5;}
#btnSecondary:disabled{color:#ffb3d1;border-color:#ffd6e8;}
#btnUndo{background:transparent;color:#ff4d94;border:1px solid #ff4d94;min-width:90px;}
#btnUndo:hover{background:rgba(255,77,148,0.10);}
#btnUndo:disabled{color:#ffb3d1;border-color:#ffd6e8;}
QComboBox{background:#fff;color:#c0006a;border:1px solid #ff80b5;border-radius:7px;padding:5px 10px;font-size:12px;font-weight:700;}
QComboBox::drop-down{border:none;width:20px;}
QComboBox QAbstractItemView{background:#fff;color:#c0006a;border:1px solid #ff80b5;selection-background-color:#ff4d94;selection-color:#fff;}
#bodySplitter::handle{background:#ff80b5;width:1px;}
#leftPanel,#rightPanel{background:#fff0f5;}
#panelHeader{background:#ffe4f0;}
#sectionTitle{font-size:10px;font-weight:700;letter-spacing:1.2px;color:#ff4d94;}
#previewList{background:#fff0f5;border:none;outline:none;padding:2px;}
#previewList::item{padding:7px 10px;border-bottom:1px solid #ffd6e8;color:#c0006a;font-size:12px;}
#previewList::item:hover{background:#ffe4f0;}
#countBadge{background:#ffd6e8;border-radius:9px;padding:1px 8px;font-size:10px;color:#c0006a;}
#sessionKey{font-size:11px;color:#ff80b5;}
#sessionVal{font-size:11px;color:#c0006a;font-family:'Consolas',monospace;}
#statCard{background:#fff;border:2px solid #ff80b5;border-radius:8px;}
#statLabel{font-size:10px;color:#ff80b5;}
#catRow{background:#fff;border:1px solid #ffd6e8;border-radius:6px;}
#catName{font-size:11px;color:#ff80b5;}
#catCount{font-size:12px;font-weight:700;color:#c0006a;}
#logArea{background:#fff;border:1px solid #ff80b5;border-radius:8px;color:#ff80b5;font-family:'Consolas',monospace;font-size:11px;padding:6px;}
QScrollBar:vertical{background:#fff0f5;width:6px;border-radius:3px;}
QScrollBar::handle:vertical{background:#ff80b5;border-radius:3px;min-height:20px;}
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}
#footer{background:#ffe4f0;border-top:2px solid #ff80b5;}
#footerText{font-size:10px;color:#ff80b5;}
        )");

    } else if (key == "ocean") {
        setStyleSheet(R"(
QMainWindow,QWidget{background:#0a1628;color:#7dd3fc;font-family:'Segoe UI',sans-serif;font-size:13px;}
#header{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #0c1f3f,stop:1 #0a1628);border-bottom:1px solid #1e4976;}
#lblTitle{font-size:22px;font-weight:700;color:#38bdf8;}
#lblSubtitle{font-size:11px;color:#1e6ea6;}
#toolbar{background:#0c1f3f;border-bottom:1px solid #1e4976;}
#pathBox{background:#071020;border:1px solid #1e4976;border-radius:7px;padding:5px 10px;font-family:'Consolas',monospace;font-size:12px;color:#38bdf8;}
QPushButton{border-radius:7px;padding:6px 14px;font-size:12px;font-weight:600;border:none;}
#btnPrimary{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #0369a1,stop:1 #0284c7);color:#fff;min-width:90px;}
#btnPrimary:hover{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #0284c7,stop:1 #0ea5e9);}
#btnPrimary:disabled{background:#1e3a5f;color:#4a6fa5;}
#btnPreview{background:#0369a1;color:#fff;min-width:80px;}
#btnPreview:hover{background:#0284c7;}
#btnPreview:disabled{background:#1e3a5f;color:#4a6fa5;}
#btnSecondary{background:#0c1f3f;color:#7dd3fc;border:1px solid #1e4976;}
#btnSecondary:hover{background:#112847;}
#btnSecondary:disabled{color:#2d5a8e;border-color:#1e3a5f;}
#btnUndo{background:transparent;color:#38bdf8;border:1px solid #38bdf8;min-width:90px;}
#btnUndo:hover{background:rgba(56,189,248,0.13);}
#btnUndo:disabled{color:#2d5a8e;border-color:#1e3a5f;}
QComboBox{background:#0c1f3f;color:#7dd3fc;border:1px solid #1e4976;border-radius:7px;padding:5px 10px;font-size:12px;font-weight:600;}
QComboBox::drop-down{border:none;width:20px;}
QComboBox QAbstractItemView{background:#0c1f3f;color:#7dd3fc;border:1px solid #1e4976;selection-background-color:#0369a1;selection-color:#fff;}
#bodySplitter::handle{background:#1e4976;width:1px;}
#leftPanel,#rightPanel{background:#0a1628;}
#panelHeader{background:#0c1f3f;}
#sectionTitle{font-size:10px;font-weight:700;letter-spacing:1.2px;color:#38bdf8;}
#previewList{background:#0a1628;border:none;outline:none;padding:2px;}
#previewList::item{padding:7px 10px;border-bottom:1px solid #0c1f3f;color:#7dd3fc;font-size:12px;}
#previewList::item:hover{background:#0c1f3f;}
#countBadge{background:#0c1f3f;border-radius:9px;padding:1px 8px;font-size:10px;color:#38bdf8;border:1px solid #1e4976;}
#sessionKey{font-size:11px;color:#1e6ea6;}
#sessionVal{font-size:11px;color:#38bdf8;font-family:'Consolas',monospace;}
#statCard{background:#071020;border:1px solid #1e4976;border-radius:8px;}
#statLabel{font-size:10px;color:#1e6ea6;}
#catRow{background:#071020;border:1px solid #0c1f3f;border-radius:6px;}
#catName{font-size:11px;color:#1e6ea6;}
#catCount{font-size:12px;font-weight:600;color:#38bdf8;}
#logArea{background:#071020;border:1px solid #1e4976;border-radius:8px;color:#1e6ea6;font-family:'Consolas',monospace;font-size:11px;padding:6px;}
QScrollBar:vertical{background:#071020;width:6px;border-radius:3px;}
QScrollBar::handle:vertical{background:#1e4976;border-radius:3px;min-height:20px;}
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}
#footer{background:#071020;border-top:1px solid #1e4976;}
#footerText{font-size:10px;color:#1e4976;}
        )");

    } else if (key == "dracula") {
        setStyleSheet(R"(
QMainWindow,QWidget{background:#282a36;color:#f8f8f2;font-family:'Segoe UI',sans-serif;font-size:13px;}
#header{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #21222c,stop:1 #282a36);border-bottom:1px solid #6272a4;}
#lblTitle{font-size:22px;font-weight:700;color:#bd93f9;}
#lblSubtitle{font-size:11px;color:#6272a4;}
#toolbar{background:#21222c;border-bottom:1px solid #6272a4;}
#pathBox{background:#191a21;border:1px solid #6272a4;border-radius:7px;padding:5px 10px;font-family:'Consolas',monospace;font-size:12px;color:#8be9fd;}
QPushButton{border-radius:7px;padding:6px 14px;font-size:12px;font-weight:600;border:none;}
#btnPrimary{background:#bd93f9;color:#282a36;min-width:90px;}
#btnPrimary:hover{background:#caa8ff;}
#btnPrimary:disabled{background:#44475a;color:#6272a4;}
#btnPreview{background:#bd93f9;color:#282a36;min-width:80px;}
#btnPreview:hover{background:#caa8ff;}
#btnPreview:disabled{background:#44475a;color:#6272a4;}
#btnSecondary{background:#44475a;color:#f8f8f2;border:1px solid #6272a4;}
#btnSecondary:hover{background:#4d5066;}
#btnSecondary:disabled{color:#6272a4;border-color:#44475a;}
#btnUndo{background:transparent;color:#ffb86c;border:1px solid #ffb86c;min-width:90px;}
#btnUndo:hover{background:rgba(255,184,108,0.13);}
#btnUndo:disabled{color:#6272a4;border-color:#44475a;}
QComboBox{background:#44475a;color:#f8f8f2;border:1px solid #6272a4;border-radius:7px;padding:5px 10px;font-size:12px;font-weight:600;}
QComboBox::drop-down{border:none;width:20px;}
QComboBox QAbstractItemView{background:#44475a;color:#f8f8f2;border:1px solid #6272a4;selection-background-color:#bd93f9;selection-color:#282a36;}
#bodySplitter::handle{background:#6272a4;width:1px;}
#leftPanel,#rightPanel{background:#282a36;}
#panelHeader{background:#21222c;}
#sectionTitle{font-size:10px;font-weight:700;letter-spacing:1.2px;color:#bd93f9;}
#previewList{background:#282a36;border:none;outline:none;padding:2px;}
#previewList::item{padding:7px 10px;border-bottom:1px solid #44475a;color:#f8f8f2;font-size:12px;}
#previewList::item:hover{background:#44475a;}
#countBadge{background:#44475a;border-radius:9px;padding:1px 8px;font-size:10px;color:#bd93f9;}
#sessionKey{font-size:11px;color:#6272a4;}
#sessionVal{font-size:11px;color:#8be9fd;font-family:'Consolas',monospace;}
#statCard{background:#191a21;border:1px solid #6272a4;border-radius:8px;}
#statLabel{font-size:10px;color:#6272a4;}
#catRow{background:#191a21;border:1px solid #44475a;border-radius:6px;}
#catName{font-size:11px;color:#6272a4;}
#catCount{font-size:12px;font-weight:600;color:#f8f8f2;}
#logArea{background:#191a21;border:1px solid #6272a4;border-radius:8px;color:#6272a4;font-family:'Consolas',monospace;font-size:11px;padding:6px;}
QScrollBar:vertical{background:#191a21;width:6px;border-radius:3px;}
QScrollBar::handle:vertical{background:#6272a4;border-radius:3px;min-height:20px;}
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}
#footer{background:#191a21;border-top:1px solid #44475a;}
#footerText{font-size:10px;color:#44475a;}
        )");
    }
}

// ─── Session ────────────────────────────────────────────────────────────────

void MainWindow::initializeSessionStorage()
{
    QString errorMessage;
    if (!logger.initializeSessionStorage(&errorMessage)) {
        lblSessionId->setText("Initialization failed");
        lblLogPath->setText(errorMessage.isEmpty() ? "Session storage unavailable." : errorMessage);
        appendLogMessage("Session storage could not be initialized.", "err");
        updateButtonStates();
        return;
    }
    lblSessionId->setText(QString("log%1").arg(logger.getSessionNumber()));
    lblLogPath->setText(logger.getLogFileName());
    appendLogMessage(QString("Session ready — log%1").arg(logger.getSessionNumber()), "info");
    appendLogMessage(QString("Logs: %1").arg(logger.getLogsDirectory()), "muted");
    updateButtonStates();
}

// ─── Busy state ─────────────────────────────────────────────────────────────

void MainWindow::setBusy(bool busy)
{
    m_busy = busy;
    btnBrowse->setEnabled(!busy);
    btnPreview->setEnabled(!busy);
    btnOrganize->setEnabled(!busy);
    btnUndo->setEnabled(!busy);
    btnShowLog->setEnabled(!busy);
    btnOpenLogs->setEnabled(!busy);

    if (busy) {
        btnOrganize->setText("Working...");
    } else {
        btnOrganize->setText("Organize");
        updateButtonStates();
    }
}

// ─── Logging ────────────────────────────────────────────────────────────────

void MainWindow::appendLogMessage(const QString &text, const QString &level)
{
    const QString ts = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString tsColor  = "#475569";
    QString msgColor = "#94a3b8";
    if      (level == "ok")   msgColor = "#34d399";
    else if (level == "err")  msgColor = "#f87171";
    else if (level == "info") msgColor = "#60a5fa";
    else if (level == "warn") msgColor = "#f59e0b";

    txtLog->append(
        QString("<span style='color:%1'>[%2]</span> <span style='color:%3'>%4</span>")
            .arg(tsColor, ts, msgColor, text.toHtmlEscaped())
        );
    txtLog->verticalScrollBar()->setValue(txtLog->verticalScrollBar()->maximum());
}

// ─── Helpers ────────────────────────────────────────────────────────────────

bool MainWindow::selectedFolderIsLogsFolder() const
{
    if (folderPath.isEmpty() || logger.getLogsDirectory().isEmpty()) return false;
    return QFileInfo(folderPath).canonicalFilePath() ==
           QFileInfo(logger.getLogsDirectory()).canonicalFilePath();
}

void MainWindow::refreshPreviewList()
{
    listPreview->clear();
    if (folderPath.isEmpty()) { lblFileCount->setText("0 files"); return; }

    const QDir dir(folderPath);
    if (!dir.exists()) {
        lblFileCount->setText("0 files");
        listPreview->addItem("Selected folder is not available.");
        return;
    }

    const QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    if (files.isEmpty()) {
        lblFileCount->setText("0 files");
        listPreview->addItem("No files found in this folder.");
        return;
    }

    for (const QFileInfo &fi : files)
        listPreview->addItem(fi.fileName());

    const int count = files.size();
    lblFileCount->setText(QString::number(count) + (count == 1 ? " file" : " files"));
}

void MainWindow::resetStatsPanel()    { updateStatsPanel(StatsReport()); }

void MainWindow::updateStatsPanel(const StatsReport &s)
{
    lblScanned->setText(QString::number(s.totalScanned));
    lblMoved->setText(QString::number(s.totalMoved));
    lblFailed->setText(QString::number(s.totalFailed));
    lblDocuments->setText(QString::number(s.documentCount));
    lblImages->setText(QString::number(s.imageCount));
    lblAudio->setText(QString::number(s.audioCount));
    lblVideos->setText(QString::number(s.videoCount));
    lblArchives->setText(QString::number(s.archiveCount));
    lblCode->setText(QString::number(s.codeCount));
    lblProjects->setText(QString::number(s.projectCount));
    lblOther->setText(QString::number(s.otherCount));
}

void MainWindow::updateButtonStates()
{
    if (m_busy) return;
    const bool ready = !folderPath.isEmpty() && logger.isReady();
    btnBrowse->setEnabled(true);
    btnPreview->setEnabled(!folderPath.isEmpty());
    btnOrganize->setEnabled(ready);
    btnUndo->setEnabled(organizer.canUndo());
    btnShowLog->setEnabled(logger.isReady());
    btnOpenLogs->setEnabled(!logger.getLogsDirectory().isEmpty());
}

// ─── Slots ──────────────────────────────────────────────────────────────────

void MainWindow::selectFolder()
{
    if (m_busy) return;
    const QString dir = QFileDialog::getExistingDirectory(
        this, "Select Folder to Organize", QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty()) return;

    folderPath = dir;
    lblPath->setText(folderPath);
    refreshPreviewList();
    resetStatsPanel();
    appendLogMessage("Folder selected: " + folderPath, "info");
    updateButtonStates();
}

void MainWindow::previewFiles()
{
    if (m_busy || folderPath.isEmpty()) return;
    listPreview->clear();

    QString err;
    const QList<PreviewItem> items = organizer.preview(folderPath, &err);
    if (!err.isEmpty()) { appendLogMessage(err, "err"); return; }
    if (items.isEmpty()) {
        listPreview->addItem("No files found.");
        lblFileCount->setText("0 files");
        appendLogMessage("No files to preview.", "warn");
        return;
    }

    for (const PreviewItem &item : items) {
        QString line = item.fileName + "   \u2192   " + item.folder;
        if (item.renamed) line += "  (as " + item.finalName + ")";
        listPreview->addItem(line);
    }
    lblFileCount->setText(QString::number(items.size()) + " preview items");
    appendLogMessage(QString("Preview: %1 file(s) ready to organize.").arg(items.size()), "info");
}

void MainWindow::organizeFiles()
{
    if (m_busy) return;
    if (folderPath.isEmpty()) {
        QMessageBox::warning(this, "No Folder", "Select a folder before organizing.");
        return;
    }
    if (!logger.isReady()) {
        QMessageBox::warning(this, "Session Error", "Session storage is not ready.");
        return;
    }
    if (selectedFolderIsLogsFolder()) {
        QMessageBox::warning(this, "Restricted", "The logs folder cannot be used as the source folder.");
        appendLogMessage("Blocked: logs folder selected as source.", "err");
        return;
    }

    QString previewErr;
    const QList<PreviewItem> previewItems = organizer.preview(folderPath, &previewErr);
    if (!previewErr.isEmpty()) { appendLogMessage(previewErr, "err"); return; }
    if (previewItems.isEmpty()) { appendLogMessage("No files to organize.", "warn"); return; }

    QString confirmText = QString("Organize %1 file(s)?\n\n").arg(previewItems.size());
    const int limit = qMin(previewItems.size(), 10);
    for (int i = 0; i < limit; ++i)
        confirmText += "  " + previewItems[i].fileName + "  \u2192  " + previewItems[i].folder + "\n";
    if (previewItems.size() > limit)
        confirmText += QString("  ...and %1 more\n").arg(previewItems.size() - limit);

    const auto reply = QMessageBox::question(
        this, "Confirm Organization", confirmText,
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        appendLogMessage("Organization cancelled.", "warn");
        return;
    }

    setBusy(true);
    appendLogMessage("Organization started...", "info");

    workerThread = new QThread(this);
    OrganizerWorker *worker = new OrganizerWorker(&organizer, folderPath);
    worker->moveToThread(workerThread);

    connect(workerThread, &QThread::started,  worker,       &OrganizerWorker::run);
    connect(worker,  &OrganizerWorker::finished, this, &MainWindow::onOrganizeDone);
    connect(worker,  &OrganizerWorker::finished, workerThread, &QThread::quit);
    connect(workerThread, &QThread::finished, worker,       &QObject::deleteLater);
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

    workerThread->start();
}

void MainWindow::onOrganizeDone(const QStringList &messages, const StatsReport &stats)
{
    for (const QString &msg : messages)
        appendLogMessage(msg, msg.startsWith("Error") ? "err" : "ok");

    updateStatsPanel(stats);
    refreshPreviewList();
    setBusy(false);
    appendLogMessage(
        QString("Done. Scanned: %1 | Moved: %2 | Failed: %3")
            .arg(stats.totalScanned).arg(stats.totalMoved).arg(stats.totalFailed),
        "info");
}

void MainWindow::undoOrganize()
{
    if (m_busy) return;
    if (!organizer.canUndo()) {
        appendLogMessage("No undo data available for this session.", "warn");
        updateButtonStates();
        return;
    }

    const auto reply = QMessageBox::question(
        this, "Undo Organization",
        "Restore all files moved in the current session?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        appendLogMessage("Undo cancelled.", "warn");
        return;
    }

    setBusy(true);
    appendLogMessage("Restoring files...", "info");

    workerThread = new QThread(this);
    UndoWorker *worker = new UndoWorker(&organizer);
    worker->moveToThread(workerThread);

    connect(workerThread, &QThread::started,  worker,       &UndoWorker::run);
    connect(worker,  &UndoWorker::finished,   this,         &MainWindow::onUndoDone);
    connect(worker,  &UndoWorker::finished,   workerThread, &QThread::quit);
    connect(workerThread, &QThread::finished, worker,       &QObject::deleteLater);
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

    workerThread->start();
}

void MainWindow::onUndoDone(const QStringList &messages)
{
    for (const QString &msg : messages)
        appendLogMessage(msg, msg.startsWith("Could not") ? "err" : "ok");

    resetStatsPanel();
    refreshPreviewList();
    setBusy(false);
}

void MainWindow::showLogContents()
{
    if (!logger.isReady()) { appendLogMessage("Session log not available.", "err"); return; }

    QDialog dialog(this);
    dialog.setWindowTitle(QString("Session Log — log%1").arg(logger.getSessionNumber()));
    dialog.resize(760, 500);
    dialog.setStyleSheet("background:#0d1117; color:#cbd5e1;");

    QVBoxLayout *lay = new QVBoxLayout(&dialog);
    lay->setContentsMargins(14, 14, 14, 14);
    lay->setSpacing(10);

    QTextEdit *viewer = new QTextEdit(&dialog);
    viewer->setReadOnly(true);
    viewer->setPlainText(logger.readLogText());
    viewer->setStyleSheet(
        "background:#0d1117; color:#cbd5e1; border:1px solid #2d3748;"
        "border-radius:8px; font-family:'Consolas','Courier New',monospace;"
        "font-size:12px; padding:6px;");
    lay->addWidget(viewer);

    QPushButton *closeBtn = new QPushButton("Close", &dialog);
    closeBtn->setStyleSheet(
        "background:#273142; color:#e2e8f0; border:1px solid #475569;"
        "border-radius:7px; padding:6px 18px; font-weight:600;");
    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    btnRow->addWidget(closeBtn);
    lay->addLayout(btnRow);

    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    dialog.exec();
}

void MainWindow::openLogsFolder()
{
    if (logger.getLogsDirectory().isEmpty()) {
        appendLogMessage("Logs directory not available.", "err");
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(logger.getLogsDirectory()));
}