#include <QDebug>

#include "xibridge.h"
#include "editor.h"

namespace xi {

XiBridge::XiBridge(QObject *parent) :
    QObject(parent),
    m_editor(nullptr),
    m_process(nullptr)
{

}

XiBridge::~XiBridge()
{
    m_process->close();
    delete m_process;
}

void XiBridge::spawn()
{
    m_process = new QProcess;
    m_process->start("xi-core");
    connect(m_process, SIGNAL(readyRead()), this, SLOT(dataAvailable()));
    m_process->waitForStarted();
}

void XiBridge::init(Editor *editor)
{
    m_editor = editor;
}

void XiBridge::new_view(QString file)
{
    QJsonObject object;
    QJsonObject params;
    if (!file.isEmpty()) {
        params["file_path"] = file;
    }
    object["method"] = "new_view";
    object["params"] = params;
    object["id"] = 0;
    send(object);
}

void XiBridge::delete_backward()
{
    QJsonObject object;
    sendEditorCmd("delete_backward", object);
}

void XiBridge::insert_newline()
{
    QJsonObject object;
    sendEditorCmd("insert_newline", object);
}

void XiBridge::move_up()
{
    QJsonObject object;
    sendEditorCmd("move_up", object);
}

void XiBridge::move_down()
{
    QJsonObject object;
    sendEditorCmd("move_down", object);
}

void XiBridge::move_left()
{
    QJsonObject object;
    sendEditorCmd("move_left", object);
}

void XiBridge::move_right()
{
    QJsonObject object;
    sendEditorCmd("move_right", object);
}

void XiBridge::delete_forward()
{
    QJsonObject object;
    sendEditorCmd("delete_forward", object);
}

void XiBridge::insert(QString text)
{
    QJsonObject object;
    object["chars"] = text;
    sendEditorCmd("insert", object);
}

void XiBridge::sendNotification(QString method, const QJsonObject& params)
{
    QJsonObject object;
    object["method"] = method;
    object["params"] = params;
    send(object);
}

void XiBridge::sendEditorCmd(QString method, const QJsonObject& params)
{
    QString view_id = m_editor->getViewId();
    QJsonObject object;
    object["method"] = method;
    object["params"] = params;
    object["view_id"] = view_id;
    sendNotification("edit", object);
}

void XiBridge::send(const QJsonObject &value)
{
    QJsonDocument doc(value);
    QString json(doc.toJson(QJsonDocument::Compact) +'\n');
    qDebug() << "sendJson";
    qDebug() << json;
    m_process->write(json.toUtf8());
    m_process->waitForBytesWritten();
}

void XiBridge::dataAvailable()
{
    qDebug() << "dataAvailable";
    QString line;
    do {
        if (m_process->canReadLine()) {
            QByteArray bytes = m_process->readLine();
            line = QString::fromUtf8(bytes);
        }else {
            line = m_process->readAll();
        }
        if (line.isEmpty()) break;

        qDebug() << line;
        QJsonObject json = QJsonDocument::fromJson(line.toUtf8()).object();
        emit jsonAvailable(json);
    }
    while (!line.isEmpty());
}

}
