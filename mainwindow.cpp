#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <dbt.h>

#include "disk.h"
#include "ejectUSBStorage.h"

MainWindow* MainWindow::instance = NULL;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setFixedSize(geometry().width(), geometry().height());

    connect(ui->btnEject, SIGNAL(clicked()), this, SLOT(slotClickEject()));

    getLogicalDrives();
}

void MainWindow::setTargetDrive(QString drive)
{
    if (targetDrive != drive) {
        targetDrive = drive;
    }
}

QString MainWindow::getTargetDrive()
{
    return targetDrive;
}

void MainWindow::slotClickEject()
{
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    QString drive = ui->cbUSBStorages->currentText();
    if (drive.isEmpty() == false) {
        drive.replace(QRegExp("[\\[\\]]"), "");
        setTargetDrive(drive);
    }

    if (getTargetDrive().isEmpty()) {
        return;
    }

    ejectUSBStorage(getTargetDrive().at(0).toLatin1());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    Q_UNUSED(event);
}

void MainWindow::getLogicalDrives()
{
    // GetLogicalDrives returns 0 on failure, or a bitmask representing
    // the drives available on the system (bit 0 = A:, bit 1 = B:, etc)
    unsigned long driveMask = GetLogicalDrives();
    int i = 0;
    ULONG pID;

    ui->cbUSBStorages->clear();

    while (driveMask != 0) {
        if (driveMask & 1) {
            // the "A" in drivename will get incremented by the # of bits
            // we've shifted
            char drivename[] = "\\\\.\\A:\\";
            drivename[4] += i;
            if (checkDriveType(drivename, &pID)) {
                ui->cbUSBStorages->addItem(QString("[%1:\\]").arg(drivename[4]), (qulonglong)pID);
            }
        }
        driveMask >>= 1;
        ui->cbUSBStorages->setCurrentIndex(0);
        ++i;
    }
}

// support routine for winEvent - returns the drive letter for a given mask
//   taken from http://support.microsoft.com/kb/163503
char FirstDriveFromMask (ULONG unitmask)
{
    char i;

    for (i = 0; i < 26; ++i) {
        if (unitmask & 0x1) {
            break;
        }
        unitmask = unitmask >> 1;
    }

    return (i + 'A');
}

// register to receive notifications when USB devices are inserted or removed
// adapted from http://www.known-issues.net/qt/qt-detect-event-windows.html
bool MainWindow::nativeEvent(const QByteArray &type, void *vMsg, long *result)
{
    Q_UNUSED(type);
    MSG *msg = (MSG*)vMsg;
    if (msg->message == WM_DEVICECHANGE)
    {
        PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)msg->lParam;
        switch(msg->wParam)
        {
        case DBT_DEVICEARRIVAL:
            if (lpdb -> dbch_devicetype == DBT_DEVTYP_VOLUME) {
                PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
                if (DBTF_NET) {
                    char ALET = FirstDriveFromMask(lpdbv->dbcv_unitmask);
                    // add device to combo box (after sanity check that
                    // it's not already there, which it shouldn't be)
                    QString qs = QString("[%1:\\]").arg(ALET);
                    if (ui->cbUSBStorages->findText(qs) == -1) {
                        ULONG pID;
                        char longname[] = "\\\\.\\A:\\";
                        longname[4] = ALET;
                        // checkDriveType gets the physicalID
                        if (checkDriveType(longname, &pID)) {
                            ui->cbUSBStorages->addItem(qs, (qulonglong)pID);
                        }
                    }
                }
            }
            break;
        case DBT_DEVICEREMOVECOMPLETE:
            if (lpdb -> dbch_devicetype == DBT_DEVTYP_VOLUME)
            {
                PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
                if (DBTF_NET) {
                    char ALET = FirstDriveFromMask(lpdbv->dbcv_unitmask);
                    //  find the device that was removed in the combo box,
                    //  and remove it from there....
                    //  "removeItem" ignores the request if the index is
                    //  out of range, and findText returns -1 if the item isn't found.
                    ui->cbUSBStorages->removeItem(ui->cbUSBStorages->findText(QString("[%1:\\]").arg(ALET)));
                }
            }
            break;
        } // skip the rest
    }
    else if (msg->message == WM_USER)
    {
        if (0x0B == msg->wParam && 0xB0 == msg->lParam) {
            getLogicalDrives();
        }
    }
    // end of if msg->message
    *result = 0; //get rid of obnoxious compiler warning

    return false; // let qt handle the rest
}
