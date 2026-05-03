#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QSplitter>
#include <QTextEdit>
#include <QThread>
#include <QWidget>
#include "fileorganizer.h"

class BackgroundWidget; // forward declaration

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void selectFolder();
    void previewFiles();
    void organizeFiles();
    void undoOrganize();
    void showLogContents();
    void openLogsFolder();
    void onThemeChanged(int index);
    void onOrganizeDone(const QStringList &messages, const StatsReport &stats);
    void onUndoDone(const QStringList &messages);

private:
    void setupUI();
    void setupStyles();
    void applyTheme(const QString &key);
    void initializeSessionStorage();
    void refreshPreviewList();
    void updateStatsPanel(const StatsReport &stats);
    void resetStatsPanel();
    void updateButtonStates();
    void setBusy(bool busy);
    void appendLogMessage(const QString &text, const QString &level = "info");
    bool selectedFolderIsLogsFolder() const;

    // Central background widget
    BackgroundWidget *bgWidget      = nullptr;

    // Header
    QFrame       *headerFrame    = nullptr;
    QLabel       *lblHeaderBg    = nullptr;

    // Toolbar
    QLabel       *lblPath        = nullptr;
    QPushButton  *btnBrowse      = nullptr;
    QPushButton  *btnPreview     = nullptr;
    QPushButton  *btnOrganize    = nullptr;
    QPushButton  *btnUndo        = nullptr;
    QPushButton  *btnOpenLogs    = nullptr;
    QPushButton  *btnShowLog     = nullptr;
    QComboBox    *cmbTheme       = nullptr;

    // Left panel
    QLabel       *lblFileCount   = nullptr;
    QListWidget  *listPreview    = nullptr;

    // Right panel – session
    QLabel       *lblSessionId   = nullptr;
    QLabel       *lblLogPath     = nullptr;

    // Right panel – stats
    QLabel       *lblScanned     = nullptr;
    QLabel       *lblMoved       = nullptr;
    QLabel       *lblFailed      = nullptr;
    QLabel       *lblDocuments   = nullptr;
    QLabel       *lblImages      = nullptr;
    QLabel       *lblAudio       = nullptr;
    QLabel       *lblVideos      = nullptr;
    QLabel       *lblArchives    = nullptr;
    QLabel       *lblCode        = nullptr;
    QLabel       *lblProjects    = nullptr;
    QLabel       *lblOther       = nullptr;

    // Right panel – log
    QTextEdit    *txtLog         = nullptr;

    QString      folderPath;
    QString      m_customImagePath;
    Logger       logger;
    Organizer    organizer;
    QThread     *workerThread    = nullptr;
    bool         m_busy          = false;
};

#endif // MAINWINDOW_H