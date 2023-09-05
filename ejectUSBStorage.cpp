//
// EjectMediaByLetter.cpp by Uwe Sieber - www.uwe-sieber.de
// Simple demonstration how to flush, lock and dismount a volume and eject a media from a drive
// Works under W2K, XP, W2K3, Vista, Win7, Win8, not tested under Win9x
// Console application - expects the drive letter of the drive to eject as parameter
// you are free to use this code in your projects
//

#include "mainwindow.h"
#include "ejectUSBStorage.h"

//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
// we expect a single drive letter as param, e.g.
// ejectmediabyletter X
//-------------------------------------------------------------------------------
int ejectUSBStorage(char driveLetter)
{
    bool ForceEject = false;  // dismount and ejecting even we got no lock

    driveLetter &= ~0x20;            // make uppercase

    if ( driveLetter < 'A' || driveLetter > 'Z' ) {
        return ERRL_INVALID_PARAM;
    }

    char szRootPath[] = "X:\\";   // "X:\"  -> for GetDriveType
    szRootPath[0] = driveLetter;

    char szVolumeAccessPath[] = "\\\\.\\X:";   // "\\.\X:"  -> to open the volume
    szVolumeAccessPath[4] = driveLetter;

    int res;
    DWORD dwRet;
    DWORD dwDriveType = GetDriveType(szRootPath);

    fprintf(stderr, "driveLetter : %c\n", driveLetter);

    // if it is no CD drive then we try to flush the volume
    if ( dwDriveType != DRIVE_CDROM ) {
        // try to flush, write access required which only admins will get
        HANDLE hVolWrite = CreateFile(szVolumeAccessPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if ( hVolWrite != INVALID_HANDLE_VALUE ) {
            fprintf(stderr, "Flushing cache...");
            res = FlushFileBuffers(hVolWrite);
            if ( res ) {
                fprintf(stderr, " OK\n");
            } else {
                fprintf(stderr, " failed  err=%u\n", GetLastError());
            }
            CloseHandle(hVolWrite);
        }
    }

    HANDLE hVolRead = CreateFile(szVolumeAccessPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if ( hVolRead == INVALID_HANDLE_VALUE ) {
        fprintf(stderr, "error %u opening the volume for read access -> abort\n", GetLastError());
        return ERRL_NO_VOLREAD;
    }


    // allowing (unlocking) eject, usually for CD/DVD only, but does not hurt (and returns TRUE) for other drives
    fprintf(stderr, "Allowing eject...");
    PREVENT_MEDIA_REMOVAL pmr = {0}; // pmr.PreventMediaRemoval = FALSE;
    res = DeviceIoControl(hVolRead, IOCTL_STORAGE_MEDIA_REMOVAL, &pmr, sizeof(pmr), NULL, 0, &dwRet, NULL);
    if ( res  ) {
        fprintf(stderr, " OK\n");
    } else {
        fprintf(stderr, " failed  err=%u\n", GetLastError());
    }


    // try to lock the volume, seems to flush too, maybe even with read access...
    fprintf(stderr, "Locking volume...");
    int Locked = DeviceIoControl(hVolRead, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &dwRet, NULL);
    if ( Locked  ) {
        fprintf(stderr, " OK\n");
    } else {
        fprintf(stderr, " failed  err=%u\n", GetLastError());
    }

    if ( !Locked && !ForceEject ) {
        return ERRL_NO_LOCK;
    }

    // dismount the file system if either we got a lock or we want to force it
    fprintf(stderr, "Dismounting volume...");
    res = DeviceIoControl(hVolRead, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &dwRet, NULL);
    if ( res ) {
        fprintf(stderr, " OK\n");
    } else {
        fprintf(stderr, " failed  err=%u\n", GetLastError());
    }

    printf("Ejecting media...");
    res = DeviceIoControl(hVolRead, IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &dwRet, NULL);
    if ( res ) {
        fprintf(stderr, " OK\n");
    } else {
        fprintf(stderr, " failed  err=%u\n", GetLastError());
    }

    if ( Locked ) {
        DeviceIoControl(hVolRead, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &dwRet, NULL);
    }
    fprintf(stderr, "Unlocking volume...\n");

    CloseHandle(hVolRead);

    if ( res ) {
        HWND h = (HWND) MainWindow::getInstance()->winId();
        SendMessage(h, WM_USER, 0x0B, 0xB0);
        return ERRL_SUCCESS;
    }

    return ERRL_NO_EJECT;
}
