#ifndef XIBRIDGE_H
#define XIBRIDGE_H

#include <QObject>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>

namespace xi {
class Editor;
}

namespace xi {

class XiBridge : public QObject
{
    Q_OBJECT
public:
    explicit XiBridge(QObject *parent = nullptr);
    ~XiBridge();

    void spawn();
    void init(Editor* editor);

    void new_view(QString file = "");
    void delete_backward();
    void insert_newline();
    void move_up();
    void move_down();
    void move_left();
    void move_right();
    void delete_forward();
    void insert(QString text);

private:
    void sendNotification(QString method, const QJsonObject& params);
    void sendEditorCmd(QString method, const QJsonObject& params);
    void send(const QJsonObject& value);

signals:
    void jsonAvailable(const QJsonObject& data);

public slots:
    void dataAvailable();

private:
    Editor* m_editor;
    QProcess* m_process;
};
}

#endif // XIBRIDGE_H
