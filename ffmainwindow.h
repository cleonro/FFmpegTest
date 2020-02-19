#ifndef FFMAINWINDOW_H
#define FFMAINWINDOW_H

#include <QMainWindow>
#include <QSharedPointer>

namespace Ui {
class FFMainWindow;
}

void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);

class FFTest;

class FFMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit FFMainWindow(QWidget *parent = nullptr);
    ~FFMainWindow();

signals:
    void appendPlainText(const QString &text);

private slots:
    void on_action_Open_triggered();

private slots:
    void on_action_Init_triggered();

private slots:
    void on_clearOutput_clicked();

private:
    const char *m_space;
    Ui::FFMainWindow *ui;
    QString m_filePath;
    QSharedPointer<FFTest> m_fftest;
};

#endif // FFMAINWINDOW_H
