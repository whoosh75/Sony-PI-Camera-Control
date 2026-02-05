#include <cstdio>
#include <cstdlib>
#include "CRSDK/CameraRemote_SDK.h"
#include <arpa/inet.h>

#define MSEARCH_ENB

struct DeviceCallbackImpl : public SCRSDK::IDeviceCallback {
  virtual void OnConnected(SCRSDK::DeviceConnectionVersioin version) override {
    printf("[Callback] OnConnected(version=%d)\n", (int)version);
  }
  virtual void OnDisconnected(CrInt32u error) override {
    printf("[Callback] OnDisconnected(error=0x%08X)\n", (unsigned)error);
  }
  virtual void OnPropertyChanged() override { printf("[Callback] OnPropertyChanged\n"); }
  virtual void OnLvPropertyChanged() override { printf("[Callback] OnLvPropertyChanged\n"); }
  virtual void OnCompleteDownload(CrChar* filename, CrInt32u type) override { printf("[Callback] OnCompleteDownload\n"); }
  virtual void OnWarning(CrInt32u warning) override { printf("[Callback] OnWarning\n"); }
  virtual void OnWarningExt(CrInt32u warning, CrInt32 param1, CrInt32 param2, CrInt32 param3) override { printf("[Callback] OnWarningExt\n"); }
  virtual void OnError(CrInt32u error) override { printf("[Callback] OnError(0x%08X)\n", (unsigned)error); }
  virtual void OnPropertyChangedCodes(CrInt32u num, CrInt32u* codes) override { printf("[Callback] OnPropertyChangedCodes\n"); }
  virtual void OnLvPropertyChangedCodes(CrInt32u num, CrInt32u* codes) override { printf("[Callback] OnLvPropertyChangedCodes\n"); }
  virtual void OnNotifyContentsTransfer(CrInt32u notify, SCRSDK::CrContentHandle contentHandle, CrChar* filename) override { printf("[Callback] OnNotifyContentsTransfer\n"); }
  virtual void OnNotifyFTPTransferResult(CrInt32u notify, CrInt32u numOfSuccess, CrInt32u numOfFail) override { printf("[Callback] OnNotifyFTPTransferResult\n"); }
  virtual void OnNotifyRemoteTransferResult(CrInt32u notify, CrInt32u per, CrChar* filename) override { printf("[Callback] OnNotifyRemoteTransferResult\n"); }
  virtual void OnNotifyRemoteTransferResult(CrInt32u notify, CrInt32u per, CrInt8u* data, CrInt64u size) override { printf("[Callback] OnNotifyRemoteTransferResultData\n"); }
  virtual void OnNotifyRemoteTransferContentsListChanged(CrInt32u notify, CrInt32u slotNumber, CrInt32u addSize) override { printf("[Callback] OnNotifyRemoteTransferContentsListChanged\n"); }
  virtual void OnReceivePlaybackTimeCode(CrInt32u timeCode) override { printf("[Callback] OnReceivePlaybackTimeCode\n"); }
  virtual void OnReceivePlaybackData(CrInt8u mediaType, CrInt32 dataSize, CrInt8u* data, CrInt64 pts, CrInt64 dts, CrInt32 param1, CrInt32 param2) override { printf("[Callback] OnReceivePlaybackData\n"); }
  virtual void OnNotifyRemoteFirmwareUpdateResult(CrInt32u notify, const void* param) override { printf("[Callback] OnNotifyRemoteFirmwareUpdateResult\n"); }
};

int main() {
  printf("Direct connection test\n");
  
  if (!SCRSDK::Init()) {
    printf("Init() failed\n");
    return 1;
  }
  
  // Try direct IP connection - camera IP from RemoteCli discovery  
  const char* camera_ip = "192.168.33.94"; // Found by scanning from eth0 interface
  
  SCRSDK::ICrCameraObjectInfo* pCam = nullptr;
  in_addr ina;
  if (inet_pton(AF_INET, camera_ip, &ina) != 1) {
    printf("Invalid IP: %s\n", camera_ip);
    return 1;
  }
  
  CrInt32u ipAddr = (CrInt32u)ina.s_addr;
  CrInt8u macBuf[6] = {0x50, 0x26, 0xEF, 0xB8, 0x3F, 0x2C}; // MAC from RemoteCli
  
  printf("Creating camera object for IP %s\n", camera_ip);
  auto err = SCRSDK::CreateCameraObjectInfoEthernetConnection(
    &pCam, 
    SCRSDK::CrCameraDeviceModelList::CrCameraDeviceModel_ILME_FX6, 
    ipAddr, 
    macBuf, 
    1  // SSH support
  );
  
  if (CR_FAILED(err) || !pCam) {
    printf("CreateCameraObjectInfoEthernetConnection failed (0x%08X)\n", (unsigned)err);
    return 1;
  }
  
  printf("Camera object created successfully\n");
  
  // Get fingerprint
  char fingerprint[4096] = {0};
  CrInt32u fpSize = (CrInt32u)sizeof(fingerprint);
  SCRSDK::CrError fpSt = SCRSDK::GetFingerprint(pCam, fingerprint, &fpSize);
  if (CR_FAILED(fpSt)) {
    printf("GetFingerprint failed (0x%08X)\n", (unsigned)fpSt);
  } else {
    fingerprint[fpSize] = '\0';
    printf("Fingerprint:\n%s\n", fingerprint);
  }
  
  // Try connect
  DeviceCallbackImpl callback;
  SCRSDK::CrDeviceHandle handle = 0;
  
  const char* password = std::getenv("SONY_PASS");
  if (!password) password = "Password1";
  
  printf("Attempting connection with password...\n");
  SCRSDK::CrError connectSt = SCRSDK::Connect(
    pCam,
    &callback,
    &handle,
    SCRSDK::CrSdkControlMode_Remote,
    SCRSDK::CrReconnecting_ON,
    nullptr,  // username 
    password,
    fingerprint,
    fpSize
  );
  
  if (CR_FAILED(connectSt)) {
    printf("Connect failed (0x%08X)\n", (unsigned)connectSt);
    pCam->Release();
    return 1;
  }
  
  printf("✅ Connected successfully! Handle: %p\n", (void*)handle);
  
  // Disconnect
  SCRSDK::Disconnect(handle);
  SCRSDK::ReleaseDevice(handle);
  pCam->Release();
  
  printf("✅ Connection test successful!\n");
  return 0;
}