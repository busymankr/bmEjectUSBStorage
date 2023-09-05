#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <windows.h>
#include "ui_mainwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow, public Ui::MainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    static MainWindow* getInstance() {
        // !NOT thread safe  - first call from main only
        if (!instance)
            instance = new MainWindow();
        return instance;
    }

    void setTargetDrive(QString drive);
    QString getTargetDrive();

    bool nativeEvent(const QByteArray &type, void *vMsg, long *result);

private:
    void getLogicalDrives();

private slots:
    void slotClickEject();
    void closeEvent(QCloseEvent* event);

private:
    Ui::MainWindow *ui;
    QString targetDrive;
    static MainWindow* instance;
};
#endif // MAINWINDOW_H
