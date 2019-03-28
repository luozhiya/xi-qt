#include "core_connection.h"

#include <QFuture>
#include <QThreadPool>
#include <QtConcurrent>
#include <QtGlobal>

namespace xi {

static const char *XI_CORE = "xi-core";

CoreConnection::CoreConnection(QObject *parent) : QObject(parent) {
    m_rpcIndex = 0;
}

CoreConnection::~CoreConnection() {
}

void CoreConnection::init() {
    m_recvStdoutBuf = std::make_unique<QBuffer>();
    m_recvStdoutBuf->open(QBuffer::ReadWrite);

    m_recvStderrBuf = std::make_unique<QBuffer>();
    m_recvStderrBuf->open(QBuffer::ReadWrite);

    //m_process.reset(new QProcess, [](QProcess *p) { p->close(); p->waitForFinished(); delete p; });
    m_process = std::make_shared<QProcess>();

    connect(m_process.get(), &QProcess::readyReadStandardOutput, this, &CoreConnection::stdoutReceivedHandler);
    connect(m_process.get(), &QProcess::readyReadStandardError, this, &CoreConnection::stderrReceivedHandler);

    m_process->start(XI_CORE);
    m_process->waitForStarted();
}

void CoreConnection::unint() {
    m_process->close();
    m_process->waitForFinished();
    m_process.reset();
    m_recvStdoutBuf.reset();
    m_recvStderrBuf.reset();
}

enum NotificationType {
    Notification_Update = 0,
    Notification_ScrollTo,
    Notification_DefStyle,
    Notification_PluginStarted,
    Notification_PluginStopped,
    Notification_AvailableThemes,
    Notification_ThemeChanged,
    Notification_AvailablePlugins,
    Notification_UpdateCmds,
    Notification_ConfigChanged,
    Notification_Alert,
    Notification_Unknown,
};

static QHash<QString, NotificationType> notificationMap;

static NotificationType to_notification(QString name) {
    if (notificationMap.size() == 0) {
        notificationMap["update"] = Notification_Update;
        notificationMap["scroll_to"] = Notification_ScrollTo;
        notificationMap["def_style"] = Notification_DefStyle;
        notificationMap["plugin_started"] = Notification_PluginStarted;
        notificationMap["plugin_stopped"] = Notification_PluginStopped;
        notificationMap["available_themes"] = Notification_AvailableThemes;
        notificationMap["theme_changed"] = Notification_ThemeChanged;
        notificationMap["available_plugins"] = Notification_AvailablePlugins;
        notificationMap["update_cmds"] = Notification_UpdateCmds;
        notificationMap["config_changed"] = Notification_ConfigChanged;
        notificationMap["alert"] = Notification_Alert;
    }
    return notificationMap.value(name, Notification_Unknown);
}

void CoreConnection::sendNotification(const QString &method, const QJsonObject &params) {
    QJsonObject object;
    object["method"] = method;
    object["params"] = params;
    sendJson(object);
}

void CoreConnection::sendRequest(const QString &method, const QJsonObject &params, const ResponseHandler &handler) {
    m_pending[m_rpcIndex] = handler;
    QJsonObject object;
    object["id"] = m_rpcIndex++;
    object["method"] = method;
    object["params"] = params;
    sendJson(object);
}

void CoreConnection::sendEdit(const QString &viewId, const QString &method, const QJsonObject &params) {
    QJsonObject object;
    object["method"] = method;
    object["view_id"] = viewId;
    object["params"] = params;
    sendNotification("edit", object);
}

void CoreConnection::sendEditArray(const QString &viewId, const QString &method, const QJsonArray &params) {
    QJsonObject object;
    object["method"] = method;
    object["view_id"] = viewId;
    object["params"] = params;
    sendNotification("edit", object);
}

void CoreConnection::sendEditRequest(
    const QString &viewId, const QString &method, const QJsonObject &params, const ResponseHandler &handler) {
    QJsonObject object;
    object["method"] = method;
    object["view_id"] = viewId;
    object["params"] = params;
    sendRequest("edit", object, handler);
}

void CoreConnection::sendClientStarted(const QString &configDir, const QString &clientExtrasDir) {
    QJsonObject object;
    object["config_dir"] = configDir;
    object["client_extras_dir"] = clientExtrasDir;
    sendNotification("client_started", object);
}

void CoreConnection::sendNewView(const QString &filePath, const ResponseHandler &handler) {
    QJsonObject object;
    if (!filePath.isEmpty()) {
        object["file_path"] = filePath;
    }
    sendRequest("new_view", object, handler);
}

void CoreConnection::sendCloseView(const QString &viewId) {
    QJsonObject object;
    object["view_id"] = viewId;
    sendNotification("close_view", object);
}

void CoreConnection::sendPaste(const QString &viewId, const QString &chars) {
    QJsonObject object;
    object["chars"] = chars;
    sendEdit(viewId, "paste", object);
}

void CoreConnection::sendInsert(const QString &viewId, const QString &chars) {
    QJsonObject object;
    object["chars"] = chars;
    sendEdit(viewId, "insert", object);
}

void CoreConnection::sendCopy(const QString &viewId, const ResponseHandler &handler) {
    QJsonObject object;
    sendEditRequest(viewId, "copy", object, handler);
}

void CoreConnection::sendCut(const QString &viewId, const ResponseHandler &handler) {
    QJsonObject object;
    sendEditRequest(viewId, "cut", object, handler);
}

void CoreConnection::sendSave(const QString &viewId, const QString &filePath) {
    QJsonObject object;
    object["view_id"] = viewId;
    object["file_path"] = filePath;
    sendNotification("save", object);
}

void CoreConnection::sendSetTheme(const QString &themeName) {
    QJsonObject object;
    object["theme_name"] = themeName;
    sendNotification("set_theme", object);
}

void CoreConnection::sendScroll(const QString &viewId, qint64 firstLine, qint64 lastLine) {
    QJsonArray object;
    object.append(firstLine);
    object.append(lastLine);
    sendEditArray(viewId, "scroll", object);
}

void CoreConnection::sendClick(const QString &viewId, qint64 line, qint64 column, qint64 modifiers, qint64 clickCount) {
    QJsonArray object;
    object.append(line);
    object.append(column);
    object.append(modifiers);
    object.append(clickCount);
    sendEditArray(viewId, "gesture", object);
}

void CoreConnection::sendDrag(const QString &viewId, qint64 line, qint64 column, qint64 modifiers) {
    QJsonArray object;
    object.append(line);
    object.append(column);
    object.append(modifiers);
    sendEditArray(viewId, "drag", object);
}

void CoreConnection::sendGesture(const QString &viewId, qint64 line, qint64 col, const QString &ty) {
    QJsonObject object;
    object["line"] = line;
    object["col"] = col;
    object["ty"] = ty;
    sendEdit(viewId, "gesture", object);
}

void CoreConnection::sendFind(const QString &viewId, const QString &chars, bool caseSensitive, const ResponseHandler &handler) {
    QJsonObject object;
    object["chars"] = chars;
    object["case_sensitive"] = caseSensitive;
    sendEditRequest(viewId, "find", object, handler);
}

void CoreConnection::sendFindNext(const QString &viewId, bool wrapAround, bool allowSame) {
    QJsonObject object;
    object["wrap_around"] = wrapAround;
    object["allow_same"] = allowSame;
    sendEdit(viewId, "find_next", object);
}

void CoreConnection::sendFindPrevious(const QString &viewId, bool wrapAround) {
    QJsonObject object;
    object["wrap_around"] = wrapAround;
    sendEdit(viewId, "find_previous", object);
}

QByteArrayList mergeBuffer(const std::shared_ptr<QBuffer> &buffer, const QByteArray &append) {
    auto &buf = buffer->buffer();
    buf.append(append);
    if (buf.size() == 0) {
        return QByteArrayList();
    }
    auto list = buf.split('\n');
    list.removeLast(); //empty
    if (list.isEmpty()) {
        return QByteArrayList();
    }
    if (!buf.endsWith('\n')) {
        buf = list.last();
        list.removeLast();
    } else {
        buf.clear();
    }
    return list;
}

void CoreConnection::stdoutReceivedHandler() {
    if (!m_recvStdoutBuf) {
        qFatal("stdout buffer invalid");
        return;
    }
    if (m_process->canReadLine()) {
        auto &buf = m_recvStdoutBuf->buffer();
        do {
            auto line = m_process->readLine();
            if (!line.endsWith('\n')) {
                buf.append(line);
            } else {
                if (buf.size() != 0) {
                    auto newLine = buf + line;
                    handleRaw(newLine);
                    buf.clear();
                } else {
                    handleRaw(line);
                }
            }
        } while (m_process->bytesAvailable());
    } else {
        auto list = mergeBuffer(m_recvStdoutBuf, m_process->readAllStandardOutput());
        foreach (auto &bytesline, list) {
            handleRaw(bytesline);
        }
    }
}

void CoreConnection::stderrReceivedHandler() {
    if (!m_recvStderrBuf) {
        qFatal("stderr buffer invalid");
        return;
    }
    auto list = mergeBuffer(m_recvStderrBuf, m_process->readAllStandardError());
    foreach (auto &bytesline, list) {
        qWarning() << "recv " << bytesline;
    }
}

void CoreConnection::handleRawInner(const QByteArray &bytes) {
    auto doc = QJsonDocument::fromJson(bytes);
    if (doc.isNull()) {
        qFatal("malformed json %s", bytes);
        return;
    }
    auto json = doc.object();
    qDebug() << "recv " << json;
    handleRpc(json);
}

void CoreConnection::handleRaw(const QByteArray &bytes) {
    handleRawInner(bytes);
}

void CoreConnection::handleRpc(const QJsonObject &json) {
    if (json["id"].isDouble()) { // number
        auto index = json["id"].toVariant().toLongLong();
        if (json["result"].isString()) { // is response
            QJsonObject result;
            result["result"] = json["result"]; // copy?
            auto it = m_pending.find(index);
            if (it != m_pending.end()) {
                auto handler = it.value();
                m_pending.erase(it);
                handler.invoke(result);
            }
        } else {
            handleRequest(json);
        }
    } else {
        handleNotification(json);
    }
}

void CoreConnection::handleRequest(const QJsonObject &json) {
    // there are currently no core -> client requests in the protocol
    qDebug() << "Unexpected RPC Request: " << json;
}

void CoreConnection::handleNotification(const QJsonObject &json) {
    if (!json.contains("method") || !json.contains("params")) {
        qDebug() << "unknown json from core:" << json;
        return;
    }

    auto method = json["method"].toString();
    auto params = json["params"].toObject();
    auto viewIdentifier = params["view_id"].toString();

    switch (to_notification(method)) {
    case Notification_Update: {
        auto update = params["update"].toObject();
        emit updateReceived(viewIdentifier, update);
    } break;
    case Notification_ScrollTo: {
        auto line = params["line"].toInt();
        auto column = params["col"].toInt();
        emit scrollReceived(viewIdentifier, line, column);
    } break;
    case Notification_DefStyle: {
        emit defineStyleReceived(params);
    } break;
    case Notification_PluginStarted: {
        auto name = params["plugin"].toString();
        emit pluginStartedReceived(viewIdentifier, name);
    } break;
    case Notification_PluginStopped: {
        auto name = params["plugin"].toString();
        emit pluginStoppedReceived(viewIdentifier, name);
    } break;
    case Notification_AvailableThemes: {
        QStringList themes;
        QJsonArray array = params["themes"].toArray();
        foreach (const QJsonValue &v, array) {
            themes.push_front(v.toString());
        }
        emit availableThemesReceived(themes);
    } break;
    case Notification_ThemeChanged: {
        auto name = params["name"].toString();
        auto theme = params["theme"].toObject();
        emit themeChangedReceived(name, theme);
    } break;
    case Notification_AvailablePlugins: {
        QJsonObject plugins = params["plugins"].toObject();
        emit availablePluginsReceived(viewIdentifier, plugins);
    } break;
    case Notification_UpdateCmds: {
        //auto plugin = params["plugin"].toString();
        //QList<QJsonObject> cmds;
        //QJsonArray array = params["cmds"].toArray();
        //foreach (const QJsonValue &v, array) {
        //    cmds.push_front(v.toObject());
        //}
        //emit updateCommandsReceived(viewIdentifier, line, column);
    } break;
    case Notification_ConfigChanged: {
        auto changes = params["changes"].toObject();
        emit configChangedReceived(viewIdentifier, changes);
    } break;
    case Notification_Alert: {
        auto message = params["msg"].toString();
        emit alertReceived(message);
    } break;
    case Notification_Unknown:
    default: {
        qDebug() << "unknown notification: " << method;
    } break;
    }
}

void CoreConnection::sendJson(const QJsonObject &json) {
    if (!m_process) {
        qDebug() << "process invalid, send failed " << json;
        return;
    }
    qDebug() << "send " << json;

    QJsonDocument doc(json);
    QString stream(doc.toJson(QJsonDocument::Compact) + '\n');
    if (-1 == m_process->write(stream.toUtf8())) {
        qFatal("process write error");
    } else {
        m_process->waitForBytesWritten();
    }
}

ResponseHandler::ResponseHandler(Callback callback /*= nullptr*/) {
    m_callback = std::move(callback);
}

ResponseHandler::ResponseHandler(const ResponseHandler &handler) {
    m_callback = std::move(handler.m_callback);
}

void ResponseHandler::invoke(const QJsonObject &json) {
    if (m_callback)
        m_callback(json);
}

ResponseHandler &ResponseHandler::operator=(const ResponseHandler &handler) {
    if (this != &handler)
        m_callback = std::move(handler.m_callback);
    return *this;
}

} // namespace xi
