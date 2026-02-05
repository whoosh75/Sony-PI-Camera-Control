#pragma once
#include "CRSDK/CameraRemote_SDK.h"
#include <string>
#include <vector>
#include <map>

struct CameraConfig {
    std::string name;
    std::string ip_address;
    std::string mac_address;
    std::string password;
    SCRSDK::CrCameraDeviceModelList model;
    int camera_id;           // 1-8 for your setup
    bool enabled;            // Allow selective enable/disable
    std::string role;        // "main", "bcam", "close", "wide", etc.
};

class MultiCameraManager {
public:
    // 8-camera production setup template
    static std::vector<CameraConfig> get8CameraSetup() {
        return {
            {
                "CAM1-MAIN-FX6", 
                "192.168.33.91", 
                "",  // Auto-detect MAC
                "Password1",
                SCRSDK::CrCameraDeviceModelList::CrCameraDeviceModel_ILME_FX6,
                1, true, "main"
            },
            {
                "CAM2-BCAM-FX6", 
                "192.168.33.92", 
                "",
                "Password1",
                SCRSDK::CrCameraDeviceModelList::CrCameraDeviceModel_ILME_FX6,
                2, true, "bcam"
            },
            {
                "CAM3-WIDE-FX3", 
                "192.168.33.93", 
                "",
                "Password1",
                SCRSDK::CrCameraDeviceModelList::CrCameraDeviceModel_ILME_FX3,
                3, true, "wide"
            },
            {
                "CAM4-CLOSE-A74", 
                "192.168.33.94", 
                "",
                "Password1",
                SCRSDK::CrCameraDeviceModelList::CrCameraDeviceModel_ILCE_7M4,
                4, true, "close"
            },
            {
                "CAM5-STAGE-FX3", 
                "192.168.33.95", 
                "",
                "Password1",
                SCRSDK::CrCameraDeviceModelList::CrCameraDeviceModel_ILME_FX3,
                5, true, "stage"
            },
            {
                "CAM6-OVERHEAD-A74", 
                "192.168.33.96", 
                "",
                "Password1",
                SCRSDK::CrCameraDeviceModelList::CrCameraDeviceModel_ILCE_7M4,
                6, true, "overhead"
            },
            {
                "CAM7-SIDE-FX6", 
                "192.168.33.97", 
                "",
                "Password1",
                SCRSDK::CrCameraDeviceModelList::CrCameraDeviceModel_ILME_FX6,
                7, true, "side"
            },
            {
                "CAM8-DETAIL-A74", 
                "192.168.33.98", 
                "",
                "Password1",
                SCRSDK::CrCameraDeviceModelList::CrCameraDeviceModel_ILCE_7M4,
                8, true, "detail"
            }
        };
    }

    // Environment variable support for 8 cameras
    static CameraConfig fromEnvironment(int cameraNum);
    static std::vector<CameraConfig> loadAll8FromEnvironment();
    
    // Connection management
    static bool connectToCamera(const CameraConfig& config);
    static bool connectAllCameras(const std::vector<CameraConfig>& cameras);
    static bool disconnectAllCameras();
    
    // Camera control operations
    static bool synchronizedRecord(const std::vector<int>& cameraIds);
    static bool setAllISO(int isoValue);
    static bool setAllAperture(float fStop);
};