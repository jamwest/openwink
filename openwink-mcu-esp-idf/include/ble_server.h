#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include <NimBLEDevice.h>

class BLEServerManager {
public:
    // Initialize BLE, create server, services, characteristics
    void init(const char* deviceName);
    
    // Start advertising
    void startAdvertising();
    
    // Check if a client is connected
    bool isConnected();
    
    // Send notification to connected client
    void notifyValue(const char* value);

private:
    NimBLEServer* pServer;
    NimBLECharacteristic* pCharacteristic;
    NimBLEAdvertising* pAdvertising;
};

#endif // BLE_SERVER_H
