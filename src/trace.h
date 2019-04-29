#ifndef TRACE_H
#define TRACE_H

#include <QCoreApplication>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QThread>

#include <chrono>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "unfair_lock.h"

namespace xi {

enum class TraceCategory {
    main,
    rpc,
};

enum class TracePhase {
    B, //begin
    E, //end
    I, //instant
};

QString to_string(TraceCategory tc);
QString to_string(TracePhase tp);

struct TraceEntry {
    QString name;
    TraceCategory cat;
    TracePhase ph;
    qint64 abstime;
    qint64 tid;
    QString thread_name;
};

class Trace {
public:
    static Trace *shared();

    bool isEnabled();
    void setEnabled(bool enabled);
    void trace(const QString &name, TraceCategory cat, TracePhase ph);
    QJsonDocument json();
    QJsonDocument snapshot();

private:
    QMutex m_mutex;
    const int BUF_SIZE = 100'000;
    std::unique_ptr<std::vector<TraceEntry>> m_buf;
    int m_n_entries = 0;
    bool m_enabled = false;

    Trace();
};

} // namespace xi

#endif // TRACE_H