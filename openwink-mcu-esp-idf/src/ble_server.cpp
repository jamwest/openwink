#include "ble_server.h"
#include "esp_log.h"

static const char *TAG = "BLE_SERVER";

// Service UUIDs
#define WINK_SERVICE_UUID "a144c6b0-5e1a-4460-bb92-3674b2f51520"
#define OTA_SERVICE_UUID "e24c13d7-d7c7-4301-903a-7750b09fc935"
#define MODULE_SETTINGS_SERVICE_UUID "cb5f7a1f-59f2-418e-b9d1-d6fc5c85a749"

// Characteristic UUIDs (minimal set)
#define HEADLIGHT_CHAR_UUID "034a383c-d3e4-4501-b7a5-1c950db4f3c7"
#define BUSY_CHAR_UUID "8d2b7b9f-c6a3-4f56-9f4f-2dc7d7873c18"
#define LEFT_STATUS_UUID "c4907f4a-fb0c-440c-bbf1-4836b0636478"
#define RIGHT_STATUS_UUID "784dd553-d837-4027-9143-280cb035163a"

/** Time in milliseconds to advertise */
static uint32_t advTime = 5000;

/** Time to sleep between advertisements */
static uint32_t sleepSeconds = 20;

/** Primary PHY used for advertising, can be one of BLE_HCI_LE_PHY_1M or BLE_HCI_LE_PHY_CODED */
static uint8_t primaryPhy = BLE_HCI_LE_PHY_1M;

/**
 *  Secondary PHY used for advertising and connecting,
 *  can be one of BLE_HCI_LE_PHY_1M, BLE_HCI_LE_PHY_2M or BLE_HCI_LE_PHY_CODED
 */
static uint8_t secondaryPhy = BLE_HCI_LE_PHY_2M;

// Server callbacks
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        ESP_LOGI(TAG, "Client connected: %s", connInfo.getAddress().toString().c_str());
        pServer->updateConnParams(connInfo.getConnHandle(), 24, 48, 0, 60);
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        ESP_LOGI(TAG, "Client disconnected, reason: %d", reason);
        ESP_LOGI(TAG, "Restarting advertising");
        NimBLEDevice::getAdvertising()->start(0);
    }
};

// Wink characteristic callback - handles headlight commands
class WinkCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        std::string value = pCharacteristic->getValue();
        ESP_LOGI(TAG, "Headlight command received: %s", value.c_str());
        
        // TODO: Parse command and call headlight control functions
        // For now, just log it so we can see commands coming through
    }
};

void BLEServerManager::init(const char* deviceName) {
    ESP_LOGI(TAG, "Initializing BLE Server: %s", deviceName);
    
    leftStatus = 0;
    rightStatus = 0;
    
    // Initialize NimBLE
    NimBLEDevice::init(deviceName);
    NimBLEDevice::setMTU(512);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    
    initServer();
    initServices();
    initCharacteristics();
    initAdvertising(deviceName);
    
    ESP_LOGI(TAG, "BLE Server initialized");
}

void BLEServerManager::initServer() {
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
}

void BLEServerManager::initServices() {
    // Create all three services (even though we're only using winkService for now)
    pWinkService = pServer->createService(WINK_SERVICE_UUID);
    pOtaService = pServer->createService(OTA_SERVICE_UUID);
    pSettingsService = pServer->createService(MODULE_SETTINGS_SERVICE_UUID);
}

void BLEServerManager::initCharacteristics() {
    // Wink command characteristic - receives commands from app
    pWinkChar = pWinkService->createCharacteristic(
        HEADLIGHT_CHAR_UUID,
        NIMBLE_PROPERTY::WRITE
    );
    pWinkChar->setCallbacks(new WinkCharacteristicCallbacks());
    pWinkChar->setValue("0");
    
    // Busy indicator - tells app when headlights are moving
    pBusyChar = pWinkService->createCharacteristic(
        BUSY_CHAR_UUID,
        NIMBLE_PROPERTY::NOTIFY
    );
    pBusyChar->setValue("0");
    
    // Left headlight status
    pLeftStatusChar = pWinkService->createCharacteristic(
        LEFT_STATUS_UUID,
        NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ
    );
    pLeftStatusChar->setValue("0");
    
    // Right headlight status
    pRightStatusChar = pWinkService->createCharacteristic(
        RIGHT_STATUS_UUID,
        NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ
    );
    pRightStatusChar->setValue("0");
    
    // Start all services
    pWinkService->start();
    pOtaService->start();
    pSettingsService->start();
}

void BLEServerManager::initAdvertising(const char* deviceName) {
    // Create extended advertisement
    NimBLEExtAdvertisement advertisement(primaryPhy, secondaryPhy);
    advertisement.setConnectable(true);
    advertisement.setScannable(false);
    
    advertisement.setName(deviceName);
    
    // Add all three service UUIDs so app recognizes the device
    advertisement.addServiceUUID(NimBLEUUID(WINK_SERVICE_UUID));
    advertisement.addServiceUUID(NimBLEUUID(OTA_SERVICE_UUID));
    advertisement.addServiceUUID(NimBLEUUID(MODULE_SETTINGS_SERVICE_UUID));
    
    advertisement.setMinInterval(0x20);  // 20ms
    advertisement.setMaxInterval(0x40);  // 40ms
    advertisement.setTxPower(ESP_PWR_LVL_P9);
    
    // Get advertising object and configure
    pAdvertising = NimBLEDevice::getAdvertising();

    if (!pAdvertising->setInstanceData(0, advertisement)) {
        ESP_LOGE(TAG, "Failed to set advertisement data");
    }
}

void BLEServerManager::startAdvertising() {
    ESP_LOGI(TAG, "Starting BLE advertising");
    
    if (pAdvertising->start(0)) {
        ESP_LOGI(TAG, "Advertising started successfully");
    } else {
        ESP_LOGE(TAG, "Failed to start advertising");
    }
}

bool BLEServerManager::isConnected() {
    return pServer->getConnectedCount() > 0;
}

void BLEServerManager::notifyLeftStatus(int status) {
    leftStatus = status;
    pLeftStatusChar->setValue(std::to_string(status));
    pLeftStatusChar->notify();
    ESP_LOGI(TAG, "Left status notified: %d", status);
}

void BLEServerManager::notifyRightStatus(int status) {
    rightStatus = status;
    pRightStatusChar->setValue(std::to_string(status));
    pRightStatusChar->notify();
    ESP_LOGI(TAG, "Right status notified: %d", status);
}

void BLEServerManager::notifyBusy(bool busy) {
    pBusyChar->setValue(busy ? "1" : "0");
    pBusyChar->notify();
    ESP_LOGI(TAG, "Busy status: %s", busy ? "true" : "false");
}

int BLEServerManager::getLeftStatus() {
    return leftStatus;
}

int BLEServerManager::getRightStatus() {
    return rightStatus;
}