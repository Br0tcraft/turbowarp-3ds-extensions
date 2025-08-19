#include <string>
#include <stdio.h>

#if defined(__3DS__)
  #include <3ds.h>

#elif defined(__WIIU__)
  #include <nn/nfp.h>
  #include <coreinit/memdefaultheap.h>
  #include <coreinit/title.h>
  #include <coreinit/thread.h>
  #include <coreinit/time.h>
#endif

extern "C" {

  static bool scanningStarted = false;

#if defined(__3DS__)
  // Previous NFC state on 3DS
  static NFC_TagState prevState = NFC_TagState_None;

#elif defined(__WIIU__)
  // Handle for the NFC device on Wii U
  static nn::nfp::DeviceHandle g_handle;
  static bool g_attached = false;

  // Thread for calling nn::nfp::Update() in background
  static OSThread nfpThread;
  static uint8_t nfpThreadStack[0x4000];
  static bool nfpThreadRunning = false;

  // Background thread loop for updating NFC events on Wii U
  static int nfpUpdateThread(int argc, char* argv[]) {
      while (nfpThreadRunning) {
          nn::nfp::Update();                           // Process NFC events
          OSSleepTicks(OSMillisecondsToTicks(50));     // Run every 50ms
      }
      return 0;
  }
#endif


  // Check if NFC is supported on this platform
  bool amiibo_supported() {
  #if defined(__3DS__) || defined(__WIIU__)
      return true;
  #else
      return false;
  #endif
  }


  // Start scanning for Amiibo
  void amiibo_start_scan() {
  #if defined(__3DS__)
      // Initialize NFC and start scanning for tags
      if (R_FAILED(nfcInit(NFC_OpType_NFCTag))) return;
      if (R_FAILED(nfcStartScanning(NFC_STARTSCAN_DEFAULTINPUT))) return;
      nfcGetTagState(&prevState);
      scanningStarted = true;

  #elif defined(__WIIU__)
      // Initialize NFC system
      if (nn::nfp::Initialize() < 0) return;
      if (nn::nfp::Mount() < 0) return;

      // Get first available NFC device
      if (nn::nfp::ListDevices(&g_handle, 1) <= 0) return;

      // Start NFC detection on that device
      if (nn::nfp::StartDetection(g_handle) < 0) return;

      scanningStarted = true;
      g_attached = true;

      // Start background thread to continuously call nn::nfp::Update()
      nfpThreadRunning = true;
      OSCreateThread(&nfpThread, nfpUpdateThread, 0, nullptr,
                     nfpThreadStack + sizeof(nfpThreadStack),
                     sizeof(nfpThreadStack), 16, 0);
      OSResumeThread(&nfpThread);
  #endif
  }


  // Return the UID of a detected Amiibo, or "" if none is new
  std::string amiibo_get_uid() {
  #if defined(__3DS__)
      if (!scanningStarted) return "";
      NFC_TagState curState;
      NFC_TagInfo info;

      // Get current NFC state
      if (R_FAILED(nfcGetTagState(&curState))) return "";

      // Only act if a new tag just entered range
      if (curState == NFC_TagState_InRange && curState != prevState) {
          prevState = curState;

          // Get info for the tag
          if (R_FAILED(nfcGetTagInfo(&info))) return "";

          // Convert 7-byte ID to hex string
          std::string uid;
          for (u32 i = 0; i < 7; i++) {
              char buf[3];
              snprintf(buf, sizeof(buf), "%02X", info.id[i]);
              uid += buf;
          }
          return uid;
      }

      // Update previous state even if tag left range
      if (curState != prevState) prevState = curState;

      return "";

  #elif defined(__WIIU__)
      if (!scanningStarted || !g_attached) return "";
      nn::nfp::TagInfo info;

      // Try to read NFC tag info
      if (nn::nfp::GetTagInfo(g_handle, &info) < 0) return "";

      // Convert UUID to hex string (usually 10 bytes on Wii U)
      std::string uid;
      for (int i = 0; i < sizeof(info.uuid); i++) {
          char buf[3];
          snprintf(buf, sizeof(buf), "%02X", info.uuid[i]);
          uid += buf;
      }
      return uid;

  #else
      return "";
  #endif
  }


  // Stop scanning and clean up resources
  void amiibo_stop_scan() {
  #if defined(__3DS__)
      scanningStarted = false;
      nfcStopScanning();
      nfcExit();

  #elif defined(__WIIU__)
      if (g_attached) {
          nn::nfp::StopDetection(g_handle);
          nn::nfp::Unmount();
          nn::nfp::Finalize();
          g_attached = false;
      }

      // Stop background thread
      if (nfpThreadRunning) {
          nfpThreadRunning = false;
          OSJoinThread(&nfpThread, nullptr);
      }
      scanningStarted = false;
  #endif
  }

}
