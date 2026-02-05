#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include "CRSDK/CameraRemote_SDK.h"

int main() {
  std::printf("ccu_diag: Initializing CRSDK...\n");
  if (!SCRSDK::Init(0)) {
    std::printf("ccu_diag: SCRSDK::Init() failed\n");
    return 1;
  }

  SCRSDK::ICrEnumCameraObjectInfo* enumInfo = nullptr;
  const int maxAttempts = 3;
  for (int i = 0; i < maxAttempts; ++i) {
    SCRSDK::CrError st = SCRSDK::EnumCameraObjects(&enumInfo, 3);
    if (!CR_FAILED(st) && enumInfo) {
      std::printf("ccu_diag: EnumCameraObjects succeeded\n");
      break;
    }
    std::printf("ccu_diag: EnumCameraObjects failed (0x%08X) attempt %d/%d\n", (unsigned)st, i+1, maxAttempts);
    if (i+1 < maxAttempts) std::this_thread::sleep_for(std::chrono::seconds(1 << i));
  }

  if (!enumInfo) {
    std::printf("ccu_diag: No enumInfo returned after retries\n");
    // Print network interfaces to help debug
    std::printf("ccu_diag: Dumping 'ip -brief addr' for debugging:\n");
    system("ip -brief addr 2>/dev/null || ip addr 2>/dev/null || echo 'ip command not available'");
    return 2;
  }

  const auto n = enumInfo->GetCount();
  std::printf("ccu_diag: Found %u camera(s)\n", (unsigned)n);
  for (unsigned idx = 0; idx < n; ++idx) {
    const SCRSDK::ICrCameraObjectInfo* info = enumInfo->GetCameraObjectInfo(idx);
    if (!info) continue;
    std::printf("--- Camera %u ---\n", idx);
    std::printf("Name: %s\n", info->GetName());
    std::printf("Model: %s\n", info->GetModel());
    std::printf("ConnType: %s\n", info->GetConnectionTypeName());
    std::printf("Adaptor: %s\n", info->GetAdaptorName());
    std::printf("SSH support: %u\n", (unsigned)info->GetSSHsupport());

    char fpBuf[4096] = {0};
    CrInt32u fpSize = (CrInt32u)sizeof(fpBuf);
    SCRSDK::CrError fpSt = SCRSDK::GetFingerprint(const_cast<SCRSDK::ICrCameraObjectInfo*>(info), fpBuf, &fpSize);
    if (!CR_FAILED(fpSt) && fpSize > 0) {
      if (fpSize < sizeof(fpBuf)) fpBuf[fpSize] = '\0';
      fpBuf[sizeof(fpBuf)-1] = '\0';
      std::printf("Fingerprint (size=%u):\n%s\n", (unsigned)fpSize, fpBuf);
    } else {
      std::printf("Fingerprint not available (0x%08X)\n", (unsigned)fpSt);
    }
  }

  enumInfo->Release();
  return 0;
}
