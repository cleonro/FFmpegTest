#include "ffmainwindow.h"
#include "ui_ffmainwindow.h"
#include "fftest.h"

#include <QQueue>
#include <QFileDialog>
#include <QDebug>

static QPlainTextEdit *textOut = nullptr;

static QQueue<QString> msgBuffer;

void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context)
//    enum QtMsgType { QtDebugMsg, QtWarningMsg, QtCriticalMsg, QtFatalMsg, QtInfoMsg, QtSystemMsg = QtCriticalMsg };
    switch (type)
    {
    case QtDebugMsg:
    {
        QString ms = msg;//.mid(1, msg.size() - 2);
        if(textOut == nullptr)
        {
            msgBuffer.enqueue(ms);
        }
        else
        {
            textOut->appendPlainText(ms);
        }
    }
        break;
    default:
        break;
    }
}

void emptyMsgBuffer()
{
    if(textOut == nullptr)
    {
        return;
    }
    while(!msgBuffer.isEmpty())
    {
        textOut->appendPlainText(msgBuffer.dequeue());
    }
}

//////////////////////////////////////////////////////////////////////////////////////////

FFMainWindow::FFMainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_space("    "),
    ui(new Ui::FFMainWindow)
{
    ui->setupUi(this);

    textOut = ui->output;
    emptyMsgBuffer();

    m_fftest = new FFTest(this);
}

FFMainWindow::~FFMainWindow()
{
    delete ui;
}

void FFMainWindow::on_clearOutput_clicked()
{
    ui->output->clear();
}

void FFMainWindow::on_action_Init_triggered()
{
    m_fftest->init();
}

void FFMainWindow::on_action_Open_triggered()
{
    qDebug() << Q_FUNC_INFO;
    m_filePath = QFileDialog::getOpenFileName(this, "Open AAC file", "", tr("AAC files (*.m4a *.mp4)"));

    qDebug () << m_space << m_filePath;

    m_fftest->open(m_filePath.toStdString().c_str());
}
