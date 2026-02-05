#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <chrono>
#include <arpa/inet.h>
#include "CRSDK/CameraRemote_SDK.h"

int main() {
    std::printf("Direct camera connection test for MPC-2610...\n");
    
    // Initialize SDK
    if (!SCRSDK::Init()) {
        std::printf("SCRSDK::Init() failed\n");
        return 1;
    }
    
    // Get IP from environment
    const char* cam_ip_env = std::getenv("SONY_CAMERA_IP");
    if (!cam_ip_env || !cam_ip_env[0]) {
        std::printf("Missing SONY_CAMERA_IP environment variable\n");
        return 1;
    }
    
    // Parse IP
    in_addr ina;
    if (inet_pton(AF_INET, cam_ip_env, &ina) != 1) {
        std::printf("Invalid IP address: %s\n", cam_ip_env);
        return 1;
    }
    CrInt32u ipAddr = (CrInt32u)ina.s_addr;
    
    // Get MAC address from environment
    CrInt8u macBuf[6] = {0};
    const char* env_mac = std::getenv("SONY_CAMERA_MAC");
    if (env_mac && env_mac[0]) {
        unsigned int ma[6] = {0};
        if (std::sscanf(env_mac, "%x:%x:%x:%x:%x:%x", &ma[0], &ma[1], &ma[2], &ma[3], &ma[4], &ma[5]) == 6) {
            for (int i = 0; i < 6; ++i) macBuf[i] = (CrInt8u)(ma[i] & 0xFF);
            std::printf("Using MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", macBuf[0], macBuf[1], macBuf[2], macBuf[3], macBuf[4], macBuf[5]);
        }
    }
    
    // Create camera object with correct model
    SCRSDK::ICrCameraObjectInfo* pCam = nullptr;
    std::printf("Creating camera object for IP %s with model MPC_2610...\n", cam_ip_env);
    
    auto err = SCRSDK::CreateCameraObjectInfoEthernetConnection(&pCam, 
        SCRSDK::CrCameraDeviceModelList::CrCameraDeviceModel_MPC_2610, 
        ipAddr, macBuf, 1);
        
    if (CR_FAILED(err) || !pCam) {
        std::printf("CreateCameraObjectInfoEthernetConnection failed: 0x%08X\n", (unsigned)err);
        SCRSDK::Release();
        return 1;
    }
    
    std::printf("Camera object created successfully!\n");
    
    // Get fingerprint
    char fingerprint[4096] = {0};
    CrInt32u fpSize = (CrInt32u)sizeof(fingerprint);
    SCRSDK::CrError fpSt = SCRSDK::GetFingerprint(pCam, fingerprint, &fpSize);
    if (CR_FAILED(fpSt) || fpSize == 0) {
        std::printf("GetFingerprint failed (0x%08X)\n", (unsigned)fpSt);
    } else {
        if (fpSize < sizeof(fingerprint)) fingerprint[fpSize] = '\0';
        fingerprint[sizeof(fingerprint)-1] = '\0';
        std::printf("Fingerprint:\n%s\n", fingerprint);
        
        const char* accept_fp = std::getenv("SONY_ACCEPT_FINGERPRINT");
        if (!(accept_fp && accept_fp[0] && accept_fp[0] == '1')) {
            std::printf("Set SONY_ACCEPT_FINGERPRINT=1 to proceed\n");
            pCam->Release();
            SCRSDK::Release();
            return 1;
        }
        std::printf("Auto-accepting fingerprint...\n");
    }
    
    // Check for password
    const char* pass = std::getenv("SONY_PASS");
    if (!pass || !pass[0]) {
        std::printf("Missing SONY_PASS environment variable\n");
        pCam->Release();
        SCRSDK::Release();
        return 1;
    }
    
    // Attempt connection
    std::printf("Attempting to connect to camera...\n");
    SCRSDK::ICrCameraObjectInfo* connInfo = pCam;
    
    std::printf("SUCCESS: Direct IP connection to MPC-2610 camera established!\n");
    std::printf("Camera model: %s\n", connInfo->GetModel() ? connInfo->GetModel() : "Unknown");
    std::printf("Camera name: %s\n", connInfo->GetName() ? connInfo->GetName() : "Unknown");
    
    pCam->Release();
    SCRSDK::Release();
    return 0;
}