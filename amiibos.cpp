#include "cast.hpp"
#include <scratch-3ds.hpp>
#include <stdio.h>
#include <string>

#if defined(__3DS__)
  #include <3ds.h>
#elif defined(__WIIU__)
  #include <coreinit/thread.h>
  #include <nfc/nfc.h>
  static OSThread nfcThread;
  static bool threadRunning = false;
  static std::string latestUID;
#endif

extern "C" {

// -------- shared state --------
static bool scanningStarted = false;

#if defined(__3DS__)
static NFC_TagState prevState;
#endif

// -------- start scan --------
void Amiibo_startScan(const std::map<std::string, std::any> &arguments, ExtensionData data) {
#if defined(__3DS__)
    Result ret = nfcInit(NFC_OpType_NFCTag);
    if (R_FAILED(ret)) return;

    ret = nfcStartScanning(NFC_STARTSCAN_DEFAULTINPUT);
    if (R_FAILED(ret)) return;

    scanningStarted = true;
    nfcGetTagState(&prevState);

#elif defined(__WIIU__)
    if (scanningStarted) return;
    scanningStarted = true;
    threadRunning = true;

    // Start a thread to poll NFC
    OSCreateThread(&nfcThread, [](int argc, void* arg) -> int {
        NFCInit();
        while (threadRunning) {
            NFCStatus status;
            NFCGetStatus(&status);

            if (status == NFC_STATUS_TAG_IN_RANGE) {
                NFCTagInfo info;
                if (NFCReadTagInfo(&info) == NFC_OK) {
                    char buf[32];
                    latestUID.clear();
                    for (u32 i = 0; i < info.uuid_length; i++) {
                        snprintf(buf, sizeof(buf), "%02X", info.uuid[i]);
                        latestUID += buf;
                    }
                }
            }
            OSSleepTicks(OSMillisecondsToTicks(50));
        }
        NFCClose();
        return 0;
    }, 0, nullptr, nullptr, 0x4000, 16, 0);
    OSResumeThread(&nfcThread);
#endif
}

// -------- get UID --------
void Amiibo_getUID(const std::map<std::string, std::any> &arguments, std::string *ret, ExtensionData data) {
#if defined(__3DS__)
    if (!scanningStarted) {
        *ret = "";
        return;
    }

    NFC_TagState curState;
    NFC_TagInfo tagInfo;

    Result r = nfcGetTagState(&curState);
    if (R_FAILED(r)) {
        *ret = "";
        return;
    }

    if (curState == NFC_TagState_InRange && curState != prevState) {
        prevState = curState;
        r = nfcGetTagInfo(&tagInfo);
        if (R_FAILED(r)) {
            *ret = "";
            return;
        }

        ret->clear();
        for (u32 i = 0; i < 7; i++) {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02X", tagInfo.id[i]);
            *ret += buf;
        }
        return;
    } else if (curState != prevState) {
        prevState = curState;
    }

    *ret = "";

#elif defined(__WIIU__)
    if (!scanningStarted) {
        *ret = "";
        return;
    }
    *ret = latestUID;

#else
    *ret = "";
#endif
}

// -------- stop scan --------
void Amiibo_stopScan(const std::map<std::string, std::any> &arguments, ExtensionData data) {
#if defined(__3DS__)
    if (scanningStarted) {
        nfcStopScanning();
        nfcExit();
        scanningStarted = false;
    }

#elif defined(__WIIU__)
    if (scanningStarted) {
        threadRunning = false;
        OSJoinThread(&nfcThread, nullptr);
        scanningStarted = false;
    }
#endif
}

// -------- supported check --------
void Amiibo_isSupported(const std::map<std::string, std::any> &arguments, std::string *ret, ExtensionData data) {
#if defined(__3DS__) || defined(__WIIU__)
    *ret = "true";
#else
    *ret = "false";
#endif
}

} // extern "C"