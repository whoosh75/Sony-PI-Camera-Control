#pragma once
#include "CRSDK/CameraRemote_SDK.h"
#include "CRSDK/IDeviceCallback.h"
#include <string>
#include <vector>
#include <map>

enum class CameraCommand {
    RECORD_START,
    RECORD_STOP,
    PHOTO_CAPTURE,
    SET_ISO,
    SET_WHITE_BALANCE,
    SET_FRAME_RATE,
    SET_APERTURE,
    SET_SHUTTER_SPEED,
    GET_STATUS
};

enum class RecordingState {
    STOPPED,
    RECORDING,
    PAUSED,
    ERROR,
    UNKNOWN
};

struct CameraStatus {
    RecordingState recording_state;
    std::string iso_value;
    std::string white_balance;
    std::string frame_rate;
    std::string aperture;
    std::string shutter_speed;
    int battery_level;
    std::string storage_remaining;
    bool camera_ready;
};

class CameraController {
private:
    struct DeviceCallback : public SCRSDK::IDeviceCallback {
        void OnConnected(SCRSDK::DeviceConnectionVersioin) override {}
        void OnWarning(CrInt32u) override {}
    };

    SCRSDK::ICrCameraObjectInfo* camera_info;
    SCRSDK::CrDeviceHandle device_handle;
    bool is_connected;
    std::string camera_name;
    DeviceCallback callback;

public:
    CameraController(const std::string& name = "Camera");
    ~CameraController();

    // Connection management (using our proven direct IP method)
    bool connectViaIP(const std::string& ip, const std::string& mac, 
                      SCRSDK::CrCameraDeviceModelList model);
    bool connectViaUSB();
    void disconnect();
    
    // Recording controls
    bool startRecording();
    bool stopRecording();
    bool capturePhoto();
    RecordingState getRecordingState();
    
    // Camera settings
    bool setISO(int iso_value);                    // 100, 200, 400, 800, 1600, 3200, 6400, etc.
    bool setWhiteBalance(const std::string& wb);   // "Auto", "Daylight", "Cloudy", "Tungsten", etc.
    bool setFrameRate(const std::string& fps);     // "24p", "30p", "60p", "120p", etc.
    bool setAperture(float f_stop);               // f/1.4, f/2.8, f/5.6, etc.
    bool setShutterSpeed(const std::string& speed); // "1/60", "1/120", "1/250", etc.
    
    // Status and monitoring
    CameraStatus getCameraStatus();
    bool isCameraReady();
    std::string getLastError() const { return last_error; }
    
    // Batch operations
    static bool syncRecordStart(const std::vector<CameraController*>& cameras);
    static bool syncRecordStop(const std::vector<CameraController*>& cameras);
    static bool setAllISO(const std::vector<CameraController*>& cameras, int iso);

private:
    std::string last_error;
    bool sendCameraCommand(CrInt32u command, CrInt32u param = 0);
    bool setCameraProperty(CrInt32u property, const void* value, CrInt32u size, SCRSDK::CrDataType type);
    bool getCameraProperty(CrInt32u property, void* value, CrInt32u* size);
    bool ensurePropertySettable(CrInt32u property, const char* label);
    
    // Sony CRSDK property mappings
    CrInt32u mapISOValue(int iso);
    CrInt32u mapWhiteBalance(const std::string& wb);
    CrInt32u mapFrameRate(const std::string& fps);
    CrInt16u mapAperture(float f_stop);
    CrInt32u mapShutterSpeed(const std::string& speed);
};