#ifndef ORGANIZERWORKER_H
#define ORGANIZERWORKER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include "fileorganizer.h"

// Runs Organizer::organize() on a background thread.
// All signals are emitted and queued to the main thread automatically.
class OrganizerWorker : public QObject
{
    Q_OBJECT

public:
    explicit OrganizerWorker(Organizer *organizer, const QString &path, QObject *parent = nullptr)
        : QObject(parent), m_organizer(organizer), m_path(path) {}

public slots:
    void run()
    {
        const QStringList messages = m_organizer->organize(m_path);
        emit finished(messages, m_organizer->getStats());
    }

signals:
    void finished(const QStringList &messages, const StatsReport &stats);

private:
    Organizer *m_organizer;
    QString    m_path;
};

// Runs Organizer::undoLastOrganization() on a background thread.
class UndoWorker : public QObject
{
    Q_OBJECT

public:
    explicit UndoWorker(Organizer *organizer, QObject *parent = nullptr)
        : QObject(parent), m_organizer(organizer) {}

public slots:
    void run()
    {
        const QStringList messages = m_organizer->undoLastOrganization();
        emit finished(messages);
    }

signals:
    void finished(const QStringList &messages);

private:
    Organizer *m_organizer;
};

#endif // ORGANIZERWORKER_H