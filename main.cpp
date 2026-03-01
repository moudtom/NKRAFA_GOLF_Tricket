#include <QApplication>
#include <QMessageBox>
#include "mainwindow.h"
#include "db.h"

static void myMsgHandler(QtMsgType, const QMessageLogContext&, const QString& msg)
{
    fprintf(stderr, "%s\n", msg.toLocal8Bit().constData());
    fflush(stderr);
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(myMsgHandler);
    QApplication a(argc, argv);

    if (!Db::open()) {
        QMessageBox::critical(nullptr, "DB", "Cannot open SQLite database.");
        return 1;
    }

    MainWindow w;
    w.show();
    return a.exec();
}
