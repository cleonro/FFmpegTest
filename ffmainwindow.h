#ifndef FFMAINWINDOW_H
#define FFMAINWINDOW_H

#include "fftest.h"

#include <QMainWindow>
#include <QSharedPointer>

namespace Ui {
class FFMainWindow;
}

void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);

class FFTest;
class QPlainTextEdit;

class FFMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit FFMainWindow(QWidget *parent = nullptr);
    ~FFMainWindow();

signals:
    void appendPlainText(const QString &text);

private slots:
    void on_actionOpen_Stream_triggered();
    void on_actionSend_to_audio_toggled(bool arg1);
    void on_actionEncode_toggled(bool arg1);
    void on_action_Stop_triggered();
    void on_action_Open_triggered();
    void on_action_Init_triggered();
    void on_clearOutput_clicked();

    void onFFTestState(FFTest::State state);
    void onAppendPlainText(const QString &text);

private:
    const char *m_space;
    Ui::FFMainWindow *ui;
    QString m_filePath;
    QSharedPointer<FFTest> m_fftest;
    QPlainTextEdit* m_output;
};

#endif // FFMAINWINDOW_H
