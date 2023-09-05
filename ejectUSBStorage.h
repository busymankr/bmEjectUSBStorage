#ifndef __EJECT_DEVICE_H__
#define __EJECT_DEVICE_H__

#ifdef __cplusplus
//extern "C" {
#endif

// returned errorlevels, nice to have when writing documentation later :-)
enum {
    ERRL_SUCCESS = 0,
    ERRL_INVALID_PARAM,
    ERRL_NO_VOLREAD,
    ERRL_NO_LOCK,
    ERRL_NO_EJECT
};

int ejectUSBStorage(char driveLetter);

#ifdef __cplusplus
//}
#endif

#endif /* __EJECT_DEVICE_H__ */
