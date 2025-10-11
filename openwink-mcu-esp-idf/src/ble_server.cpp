#include "ble_server.h"
#include "esp_log.h"

static const char *TAG = "BLE_SERVER";

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Server callbacks
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        ESP_LOGI(TAG, "Client connected: %s", connInfo.getAddress().toString().c_str());
        pServer->updateConnParams(connInfo.getConnHandle(), 24, 48, 0, 60);
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        ESP_LOGI(TAG, "Client disconnected, reason: %d", reason);
        ESP_LOGI(TAG, "Starting advertising again");
        NimBLEDevice::getAdvertising()->start();
    }
};

// Characteristic callbacks for read/write
class CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        ESP_LOGI(TAG, "Client read characteristic, value: %s", 
                 pCharacteristic->getValue().c_str());
    }

    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        std::string value = pCharacteristic->getValue();
        ESP_LOGI(TAG, "Client wrote to characteristic: %s", value.c_str());
        
        // TODO: This is where we'll call headlight control functions
        if (value == "OPEN") {
            ESP_LOGI(TAG, "Command: OPEN headlights");
        } else if (value == "CLOSE") {
            ESP_LOGI(TAG, "Command: CLOSE headlights");
        } else {
            ESP_LOGI(TAG, "Unknown command: %s", value.c_str());
        }
    }

    void onStatus(NimBLECharacteristic* pCharacteristic, int code) override {
        ESP_LOGI(TAG, "Notification/Indication status code: %d", code);
    }
};

void BLEServerManager::init(const char* deviceName) {
    ESP_LOGI(TAG, "Initializing BLE Server: %s", deviceName);
    
    // Initialize NimBLE
    NimBLEDevice::init(deviceName);
    
    // Create server
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    
    // Create service
    NimBLEService* pService = pServer->createService(SERVICE_UUID);
    
    // Create characteristic with READ, WRITE, and NOTIFY properties
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ | 
        NIMBLE_PROPERTY::WRITE | 
        NIMBLE_PROPERTY::NOTIFY
    );
    
    // Set callbacks for the characteristic
    pCharacteristic->setCallbacks(new CharacteristicCallbacks());
    
    // Set initial value
    pCharacteristic->setValue("READY");
    
    // Start the service
    pService->start();
    
    // Get advertising object
    pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setName(deviceName);   
    
    ESP_LOGI(TAG, "BLE Server initialized");
}

void BLEServerManager::startAdvertising() {
    ESP_LOGI(TAG, "Starting BLE advertising");
    pAdvertising->start();
}

bool BLEServerManager::isConnected() {
    return pServer->getConnectedCount() > 0;
}

void BLEServerManager::notifyValue(const char* value) {
    if (isConnected()) {
        pCharacteristic->setValue(value);
        pCharacteristic->notify();
        ESP_LOGI(TAG, "Notified value: %s", value);
    } else {
        ESP_LOGI(TAG, "Cannot notify - no client connected");
    }
}