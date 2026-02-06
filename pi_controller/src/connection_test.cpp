#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include "CRSDK/CameraRemote_SDK.h"

using namespace std;

int main() {
    cout << "🧪 Sony Camera SDK Connection Test" << endl;
    cout << "===================================" << endl;

    // Initialize SDK
    if (!SCRSDK::Init()) {
        cout << "❌ Failed to initialize Sony CRSDK" << endl;
        return 1;
    }
    cout << "✅ Sony CRSDK initialized successfully" << endl;

    // Connect to camera
    string ip = "192.168.1.110";
    in_addr ina;
    if (inet_pton(AF_INET, ip.c_str(), &ina) != 1) {
        cout << "❌ Invalid IP address: " << ip << endl;
        return 1;
    }
    CrInt32u ipAddr = (CrInt32u)ina.s_addr;
    
    CrInt8u macBuf[6] = {0};
    SCRSDK::ICrCameraObjectInfo* pCamera = nullptr;
    
    auto err = SCRSDK::CreateCameraObjectInfoEthernetConnection(&pCamera, 
        SCRSDK::CrCameraDeviceModelList::CrCameraDeviceModel_MPC_2610, 
        ipAddr, macBuf, 1);
    
    if (CR_FAILED(err) || !pCamera) {
        cout << "❌ Failed to create camera connection. Error: 0x" << hex << err << dec << endl;
        SCRSDK::Release();
        return 1;
    }
    
    cout << "✅ Camera object created successfully!" << endl;
    cout << "   Model: " << (pCamera->GetModel() ? pCamera->GetModel() : "Unknown") << endl;
    cout << "   Name: " << (pCamera->GetName() ? pCamera->GetName() : "Unknown") << endl;
    
    // Test fingerprint
    char fingerprint[4096] = {0};
    CrInt32u fpSize = (CrInt32u)sizeof(fingerprint);
    SCRSDK::CrError fpResult = SCRSDK::GetFingerprint(pCamera, fingerprint, &fpSize);
    
    if (CR_SUCCEEDED(fpResult) && fpSize > 0) {
        cout << "🔐 Fingerprint available (length: " << fpSize << ")" << endl;
    } else {
        cout << "⚠️  No fingerprint or fingerprint failed: 0x" << hex << fpResult << dec << endl;
    }
    
    // Try different connection approaches
    cout << "\n🔧 Testing Connection Methods:" << endl;
    
    SCRSDK::CrDeviceHandle deviceHandle = 0;
    
    // Method 1: Simple connect without auth
    cout << "1. Trying simple connection without authentication..." << endl;
    SCRSDK::CrError result1 = SCRSDK::Connect(pCamera, nullptr, &deviceHandle, 
        SCRSDK::CrSdkControlMode_Remote, SCRSDK::CrReconnecting_OFF, 
        "", "", "", 0);
    cout << "   Result: 0x" << hex << result1 << dec << " (deviceHandle: " << deviceHandle << ")" << endl;
    
    if (deviceHandle == 0) {
        // Method 2: With admin user
        cout << "2. Trying with admin user..." << endl;
        result1 = SCRSDK::Connect(pCamera, nullptr, &deviceHandle, 
            SCRSDK::CrSdkControlMode_Remote, SCRSDK::CrReconnecting_OFF, 
            "admin", "", "", 0);
        cout << "   Result: 0x" << hex << result1 << dec << " (deviceHandle: " << deviceHandle << ")" << endl;
    }
    
    if (deviceHandle == 0) {
        // Method 3: With password
        cout << "3. Trying with Password1..." << endl;
        result1 = SCRSDK::Connect(pCamera, nullptr, &deviceHandle, 
            SCRSDK::CrSdkControlMode_Remote, SCRSDK::CrReconnecting_OFF, 
            "admin", "Password1", "", 0);
        cout << "   Result: 0x" << hex << result1 << dec << " (deviceHandle: " << deviceHandle << ")" << endl;
    }
    
    if (deviceHandle == 0) {
        // Method 4: ContentsTransfer mode
        cout << "4. Trying ContentsTransfer mode..." << endl;
        result1 = SCRSDK::Connect(pCamera, nullptr, &deviceHandle, 
            SCRSDK::CrSdkControlMode_ContentsTransfer, SCRSDK::CrReconnecting_OFF, 
            "admin", "Password1", "", 0);
        cout << "   Result: 0x" << hex << result1 << dec << " (deviceHandle: " << deviceHandle << ")" << endl;
    }
    
    if (deviceHandle != 0) {
        cout << "\n✅ CONNECTION SUCCESS! Device handle: " << deviceHandle << endl;
        
        // Try sending a simple command
        cout << "\n📷 Testing camera commands..." << endl;
        SCRSDK::CrError cmdResult = SCRSDK::SendCommand(deviceHandle, 
            SCRSDK::CrCommandId::CrCommandId_MovieRecord, 
            SCRSDK::CrCommandParam::CrCommandParam_Down);
        cout << "Record start command result: 0x" << hex << cmdResult << dec << endl;
        
        // Cleanup
        SCRSDK::Disconnect(deviceHandle);
        SCRSDK::ReleaseDevice(deviceHandle);
    } else {
        cout << "\n❌ All connection methods failed" << endl;
        cout << "📝 Camera object works but authentication is required for commands" << endl;
    }
    
    // Cleanup
    pCamera->Release();
    SCRSDK::Release();
    
    return 0;
}