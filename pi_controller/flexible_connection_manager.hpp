#pragma once
#include "CRSDK/CameraRemote_SDK.h"
#include <string>
#include <vector>
#include <map>

enum class ConnectionType {
    ETHERNET,
    WIFI,
    USB,
    AUTO_DETECT
};

enum class ConnectionStatus {
    DISCONNECTED,
    CONNECTING, 
    CONNECTED,
    ERROR,
    FALLBACK_ACTIVE
};

struct CameraConnection {
    std::string name;
    int camera_id;                    // 1-8
    SCRSDK::CrCameraDeviceModelList model;
    
    // Multiple connection options
    std::string ethernet_ip;
    std::string ethernet_mac;
    std::string wifi_ip;
    std::string wifi_mac;
    std::string usb_device_path;
    
    // Connection preferences (ordered by priority)
    std::vector<ConnectionType> connection_priority;
    
    // Current status
    ConnectionType active_connection;
    ConnectionStatus status;
    std::string last_error;
    
    // Authentication
    std::string ssh_password;
    bool auto_accept_fingerprint;
};

class FlexibleConnectionManager {
public:
    // Scalable 1-8 camera configurations
    static std::vector<CameraConnection> createProductionSetup(int num_cameras);
    
    // Connection methods with fallback
    static bool connectCamera(CameraConnection& camera);
    static bool connectViaEthernet(CameraConnection& camera);
    static bool connectViaWiFi(CameraConnection& camera);
    static bool connectViaUSB(CameraConnection& camera);
    
    // Auto-detection and fallback
    static bool autoDetectAndConnect(CameraConnection& camera);
    static bool attemptFallback(CameraConnection& camera);
    
    // Connection monitoring and recovery
    static bool monitorConnections(std::vector<CameraConnection>& cameras);
    static bool reconnectFailedCameras(std::vector<CameraConnection>& cameras);
    
    // USB specific functions
    static std::vector<std::string> scanUSBCameras();
    static bool isUSBCameraAvailable(const std::string& device_path);
    
    // Network diagnostics
    static bool testEthernetConnection(const std::string& ip);
    static bool testWiFiConnection(const std::string& ip);
    static std::string getDeviceMAC(const std::string& ip);
};