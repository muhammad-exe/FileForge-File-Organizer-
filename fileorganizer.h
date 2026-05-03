#ifndef FILEORGANIZER_H
#define FILEORGANIZER_H

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QTextStream>

struct PreviewItem {
    QString fileName;
    QString folder;
    QString oldPath;
    QString newPath;
    QString finalName;
    bool renamed = false;
};

struct StatsReport {
    int totalScanned = 0;
    int totalMoved = 0;
    int totalFailed = 0;

    int imageCount = 0;
    int documentCount = 0;
    int audioCount = 0;
    int videoCount = 0;
    int archiveCount = 0;
    int codeCount = 0;
    int projectCount = 0;
    int otherCount = 0;
};

class Rule {
private:
    QString toLowerCase(const QString &text) const {
        return text.toLower();
    }

public:
    QString getFolder(const QString &ext, const QString &filename) const {
        const QString lowerExt = toLowerCase(ext);
        const QString lowerName = toLowerCase(filename);

        if (lowerExt == ".jpg" || lowerExt == ".jpeg") return "Images/JPG";
        if (lowerExt == ".png") return "Images/PNG";
        if (lowerExt == ".gif") return "Images/GIF";
        if (lowerExt == ".bmp") return "Images/BMP";

        if (lowerExt == ".mp3") return "Audio/MP3";
        if (lowerExt == ".wav") return "Audio/WAV";

        if (lowerExt == ".mp4") return "Videos/MP4";
        if (lowerExt == ".mkv") return "Videos/MKV";
        if (lowerExt == ".avi") return "Videos/AVI";

        if (lowerExt == ".pdf") return "Documents/PDFs";
        if (lowerExt == ".docx" || lowerExt == ".doc") return "Documents/Word";
        if (lowerExt == ".txt") return "Documents/Text";
        if (lowerExt == ".xlsx" || lowerExt == ".xls") return "Documents/Excel";
        if (lowerExt == ".pptx" || lowerExt == ".ppt") return "Documents/PowerPoint";

        if (lowerExt == ".zip") return "Archives/ZIP";
        if (lowerExt == ".rar") return "Archives/RAR";

        if (lowerExt == ".cpp") return "Code/C++";
        if (lowerExt == ".py") return "Code/Python";
        if (lowerExt == ".java") return "Code/Java";
        if (lowerExt == ".html") return "Code/HTML";
        if (lowerExt == ".css") return "Code/CSS";
        if (lowerExt == ".js") return "Code/JavaScript";

        if (lowerName.contains("project")) return "Projects";

        return "Others";
    }
};

class Logger {
private:
    QString logsDirectory;
    QString logFileName;
    QString undoFileName;
    int sessionNumber = 0;

    bool isDigitsOnly(const QString &text) const {
        if (text.isEmpty()) return false;
        for (const QChar &ch : text) {
            if (!ch.isDigit()) return false;
        }
        return true;
    }

public:
    bool initializeSessionStorage(QString *errorMessage = nullptr) {
        QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        if (desktopPath.isEmpty()) desktopPath = QDir::homePath();

        QDir desktopDir(desktopPath);
        if (!desktopDir.exists()) {
            if (errorMessage) *errorMessage = "Desktop location is not available.";
            return false;
        }

        logsDirectory = desktopDir.filePath("logs");
        QDir logsDir(logsDirectory);
        if (!logsDir.exists() && !desktopDir.mkpath("logs")) {
            if (errorMessage) *errorMessage = "Desktop logs folder could not be created.";
            return false;
        }

        int highestLogNumber = 0;
        const QStringList fileNames = logsDir.entryList(QStringList() << "log*.txt", QDir::Files, QDir::Name);
        for (const QString &fileName : fileNames) {
            const QFileInfo info(fileName);
            const QString baseName = info.completeBaseName();
            if (!baseName.startsWith("log") || baseName.endsWith("_undo")) continue;
            const QString numberPart = baseName.mid(3);
            if (isDigitsOnly(numberPart)) {
                const int value = numberPart.toInt();
                if (value > highestLogNumber) highestLogNumber = value;
            }
        }

        sessionNumber = highestLogNumber + 1;
        logFileName = logsDir.filePath(QString("log%1.txt").arg(sessionNumber));
        undoFileName = logsDir.filePath(QString("log%1_undo.txt").arg(sessionNumber));

        QFile logFile(logFileName);
        if (!logFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            if (errorMessage) *errorMessage = "Session log file could not be created.";
            return false;
        }
        QTextStream logOut(&logFile);
        logOut << "===== FileForge Session =====\n";
        logOut << "Session: log" << sessionNumber << "\n\n";
        logFile.close();

        QFile undoFile(undoFileName);
        if (!undoFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            if (errorMessage) *errorMessage = "Session undo file could not be created.";
            return false;
        }
        undoFile.close();

        return true;
    }

    void logMove(const QString &filename, const QString &oldPath, const QString &newPath, const QString &folder) {
        if (logFileName.isEmpty()) return;
        QFile file(logFileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
            QTextStream out(&file);
            out << "Moved File: " << filename << "\n";
            out << "From: " << oldPath << "\n";
            out << "To: " << newPath << "\n";
            out << "Category: " << folder << "\n";
            out << "-----------------------------\n";
            file.close();
        }
    }

    void logUndo(const QString &filename, const QString &currentPath, const QString &restoredPath) {
        if (logFileName.isEmpty()) return;
        QFile file(logFileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
            QTextStream out(&file);
            out << "Undo File: " << filename << "\n";
            out << "From: " << currentPath << "\n";
            out << "To: " << restoredPath << "\n";
            out << "-----------------------------\n";
            file.close();
        }
    }

    void logError(const QString &filename, const QString &reason) {
        if (logFileName.isEmpty()) return;
        QFile file(logFileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
            QTextStream out(&file);
            out << "Error File: " << filename << "\n";
            out << "Reason: " << reason << "\n";
            out << "-----------------------------\n";
            file.close();
        }
    }

    QString readLogText() const {
        if (logFileName.isEmpty()) return "Session log is not available.";
        QFile file(logFileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return "Could not open the session log file.";
        QTextStream in(&file);
        const QString text = in.readAll();
        file.close();
        return text;
    }

    QString getLogFileName() const { return logFileName; }
    QString getUndoFileName() const { return undoFileName; }
    QString getLogsDirectory() const { return logsDirectory; }
    int getSessionNumber() const { return sessionNumber; }
    bool isReady() const { return !logFileName.isEmpty() && !undoFileName.isEmpty(); }
};

class Organizer {
private:
    Rule rule;
    Logger *logger = nullptr;
    StatsReport lastStats;

    void resetStats() { lastStats = StatsReport(); }

    void updateStats(const QString &folder) {
        if (folder.startsWith("Images")) lastStats.imageCount++;
        else if (folder.startsWith("Documents")) lastStats.documentCount++;
        else if (folder.startsWith("Audio")) lastStats.audioCount++;
        else if (folder.startsWith("Videos")) lastStats.videoCount++;
        else if (folder.startsWith("Archives")) lastStats.archiveCount++;
        else if (folder.startsWith("Code")) lastStats.codeCount++;
        else if (folder.startsWith("Projects")) lastStats.projectCount++;
        else lastStats.otherCount++;
    }

    QString getDuplicateName(const QString &folderPath, const QString &filename) const {
        const QString firstPath = QDir(folderPath).filePath(filename);
        if (!QFileInfo::exists(firstPath)) return firstPath;
        int count = 1;
        while (true) {
            const QString tempPath = QDir(folderPath).filePath(QString("copy%1_%2").arg(count).arg(filename));
            if (!QFileInfo::exists(tempPath)) return tempPath;
            count++;
        }
    }

    QString getUndoRestoreName(const QString &oldPath) const {
        if (!QFileInfo::exists(oldPath)) return oldPath;
        const QFileInfo info(oldPath);
        const QString folderPath = info.absolutePath();
        const QString filename = info.fileName();
        int count = 1;
        while (true) {
            const QString tempPath = QDir(folderPath).filePath(QString("restored%1_%2").arg(count).arg(filename));
            if (!QFileInfo::exists(tempPath)) return tempPath;
            count++;
        }
    }

    void clearUndoFile() const {
        if (!logger) return;
        const QString undoFileName = logger->getUndoFileName();
        if (undoFileName.isEmpty()) return;
        QFile file(undoFileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) file.close();
    }

    void saveUndoInfo(const QString &oldPath, const QString &newPath) const {
        if (!logger) return;
        const QString undoFileName = logger->getUndoFileName();
        if (undoFileName.isEmpty()) return;
        QFile file(undoFileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
            QTextStream out(&file);
            out << oldPath << "|" << newPath << "\n";
            file.close();
        }
    }

    QList<QFileInfo> collectSourceFiles(const QString &path, QString *errorMessage = nullptr) const {
        QList<QFileInfo> files;
        const QDir sourceDir(path);
        if (!sourceDir.exists()) {
            if (errorMessage) *errorMessage = "The selected folder is not available.";
            return files;
        }
        const QFileInfoList infoList = sourceDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo &info : infoList) {
            if (info.isFile()) files.append(info);
        }
        return files;
    }

public:
    explicit Organizer(Logger *logRef = nullptr) : logger(logRef) { resetStats(); }

    void setLogger(Logger *logRef) { logger = logRef; }

    QList<PreviewItem> preview(const QString &path, QString *errorMessage = nullptr) const {
        QList<PreviewItem> items;
        const QList<QFileInfo> sourceFiles = collectSourceFiles(path, errorMessage);
        for (const QFileInfo &info : sourceFiles) {
            const QString filename = info.fileName();
            const QString ext = info.suffix().isEmpty() ? QString() : QString(".") + info.suffix();
            const QString folder = rule.getFolder(ext, filename);
            const QString newFolderPath = QDir(path).filePath(folder);
            const QString newPath = getDuplicateName(newFolderPath, filename);
            PreviewItem item;
            item.fileName = filename;
            item.folder = folder;
            item.oldPath = info.absoluteFilePath();
            item.newPath = newPath;
            item.finalName = QFileInfo(newPath).fileName();
            item.renamed = (item.finalName != filename);
            items.append(item);
        }
        return items;
    }

    QStringList organize(const QString &path) {
        QStringList messages;
        resetStats();
        clearUndoFile();

        QString scanError;
        const QList<QFileInfo> sourceFiles = collectSourceFiles(path, &scanError);
        if (!scanError.isEmpty()) { messages << scanError; return messages; }

        for (const QFileInfo &info : sourceFiles) {
            lastStats.totalScanned++;
            const QString filename = info.fileName();
            const QString ext = info.suffix().isEmpty() ? QString() : QString(".") + info.suffix();
            const QString folder = rule.getFolder(ext, filename);
            const QString newFolderPath = QDir(path).filePath(folder);

            QDir dir;
            if (!dir.mkpath(newFolderPath)) {
                const QString reason = "Destination folder could not be created.";
                messages << QString("Error moving file: %1").arg(filename);
                if (logger) logger->logError(filename, reason);
                lastStats.totalFailed++;
                continue;
            }

            const QString oldPath = info.absoluteFilePath();
            const QString newPath = getDuplicateName(newFolderPath, filename);
            const QString finalName = QFileInfo(newPath).fileName();

            QFile file(oldPath);
            if (!file.rename(newPath)) {
                const QString reason = file.errorString().isEmpty() ? "Move operation failed." : file.errorString();
                messages << QString("Error moving file: %1").arg(filename);
                if (logger) logger->logError(filename, reason);
                lastStats.totalFailed++;
                continue;
            }

            saveUndoInfo(oldPath, newPath);
            if (logger) logger->logMove(filename, oldPath, newPath, folder);

            QString message = QString("Moved: %1 -> %2").arg(filename, folder);
            if (finalName != filename) message += QString(" (saved as %1)").arg(finalName);
            messages << message;
            lastStats.totalMoved++;
            updateStats(folder);
        }
        return messages;
    }

    QStringList undoLastOrganization() {
        QStringList messages;
        if (!logger || logger->getUndoFileName().isEmpty()) {
            messages << "Undo data is not available for this session.";
            return messages;
        }

        QFile file(logger->getUndoFileName());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            messages << "Undo data is not available for this session.";
            return messages;
        }

        QStringList lines;
        QTextStream in(&file);
        while (!in.atEnd()) {
            const QString line = in.readLine().trimmed();
            if (!line.isEmpty()) lines << line;
        }
        file.close();

        if (lines.isEmpty()) { messages << "No files available to restore."; return messages; }

        int restoredCount = 0;
        for (int i = lines.size() - 1; i >= 0; --i) {
            const QString line = lines[i];
            const int splitPos = line.indexOf('|');
            if (splitPos == -1) continue;

            const QString oldPath = line.left(splitPos);
            const QString currentPath = line.mid(splitPos + 1);
            if (!QFileInfo::exists(currentPath)) continue;

            const QFileInfo oldInfo(oldPath);
            QDir dir;
            if (!dir.mkpath(oldInfo.absolutePath())) {
                messages << QString("Could not restore file: %1").arg(QFileInfo(currentPath).fileName());
                continue;
            }

            const QString restoredPath = getUndoRestoreName(oldPath);
            QFile currentFile(currentPath);
            if (!currentFile.rename(restoredPath)) {
                messages << QString("Could not restore file: %1").arg(QFileInfo(currentPath).fileName());
                continue;
            }

            const QString fileName = QFileInfo(currentPath).fileName();
            if (logger) logger->logUndo(fileName, currentPath, restoredPath);

            QString message = QString("Restored: %1").arg(fileName);
            if (restoredPath != oldPath) message += QString(" (saved as %1)").arg(QFileInfo(restoredPath).fileName());
            messages << message;
            restoredCount++;
        }

        clearUndoFile();
        if (restoredCount > 0) messages.prepend(QString("Undo completed. Restored %1 file(s).").arg(restoredCount));
        return messages;
    }

    bool canUndo() const {
        if (!logger || logger->getUndoFileName().isEmpty()) return false;
        QFile file(logger->getUndoFileName());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
        QTextStream in(&file);
        while (!in.atEnd()) {
            if (!in.readLine().trimmed().isEmpty()) { file.close(); return true; }
        }
        file.close();
        return false;
    }

    StatsReport getStats() const { return lastStats; }
};

#endif // FILEORGANIZER_H