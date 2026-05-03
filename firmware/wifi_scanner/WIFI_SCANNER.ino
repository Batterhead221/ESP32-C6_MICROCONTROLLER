#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

const int LED_R = 3;
const int LED_G = 4;
const int LED_B = 5;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

void allOff() {
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, HIGH);
}

void setBlue(bool on) {
  digitalWrite(LED_B, on ? LOW : HIGH);   // active-low
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  allOff();

  Serial.println();
  Serial.println("=== BLE ADVERTISE TEST ===");

  BLEDevice::init("JBS-ESP32C6");

  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ
  );

  pCharacteristic->setValue("HELLO FROM ESP32-C6");
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();

  Serial.println("BLE ADVERTISING STARTED");
  Serial.println("LOOK FOR: JBS-ESP32C6");

  setBlue(true);
}

void loop() {
  delay(2000);
  Serial.println("BLE STILL ADVERTISING...");
}