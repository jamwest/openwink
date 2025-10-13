#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include <NimBLEDevice.h>
#include <string>

class BLEServerManager {
public:
    void init(const char* deviceName);
    void startAdvertising();
    bool isConnected();
    
    // Notify methods for updating the app
    void notifyLeftStatus(int status);
    void notifyRightStatus(int status);
    void notifyBusy(bool busy);
    
    // Get current status
    int getLeftStatus();
    int getRightStatus();

private:
    void initServer();
    void initServices();
    void initCharacteristics();
    void initAdvertising(const char* deviceName);
    
    NimBLEServer* pServer;
    NimBLEExtAdvertising* pAdvertising;
    
    // Services
    NimBLEService* pWinkService;
    NimBLEService* pOtaService;
    NimBLEService* pSettingsService;
    
    // Minimal characteristics for POC
    NimBLECharacteristic* pWinkChar;           // Receives commands
    NimBLECharacteristic* pBusyChar;           // Notifies busy status
    NimBLECharacteristic* pLeftStatusChar;     // Left headlight position
    NimBLECharacteristic* pRightStatusChar;    // Right headlight position
    
    // Status tracking
    int leftStatus;
    int rightStatus;
};

#endif // BLE_SERVER_H