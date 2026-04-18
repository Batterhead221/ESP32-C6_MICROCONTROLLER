#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// =========================
// USER SETTINGS
// =========================
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

// =========================
// VERIFIED BOARD MAP
// =========================
const int LED_R  = 3;   // active-low
const int LED_G  = 4;   // active-low
const int LED_B  = 5;   // active-low
const int I2C_SDA = 6;
const int I2C_SCL = 7;
const int USR_SW  = 23; // active-low button with INPUT_PULLUP

#define OLED_ADDR     0x3C
#define HDC302X_ADDR  0x44
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

#define BLE_SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define BLE_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// =========================
// GLOBAL STATE
// =========================
bool oledOk = false;
bool bleConnected = false;
bool wifiConnected = false;

float tempC = 0.0f;
float humidity = 0.0f;

unsigned long lastSensorRead = 0;
unsigned long lastStatusPrint = 0;
unsigned long lastBleBlink = 0;
unsigned long lastButtonDebounce = 0;

int currentScreen = 0;
int lastButtonState = HIGH;

// =========================
// LED HELPERS (ACTIVE-LOW)
// =========================
void allOff() {
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, HIGH);
}

void setRed(bool on)   { digitalWrite(LED_R, on ? LOW : HIGH); }
void setGreen(bool on) { digitalWrite(LED_G, on ? LOW : HIGH); }
void setBlue(bool on)  { digitalWrite(LED_B, on ? LOW : HIGH); }

// Priority:
// red = button/error
// green = Wi-Fi connected
// blue = BLE advertising or BLE connected indicator handled separately
void updateStatusLed() {
  bool buttonPressed = (digitalRead(USR_SW) == LOW);

  allOff();

  if (buttonPressed) {
    setRed(true);
    return;
  }

  if (wifiConnected) {
    setGreen(true);
  }

  // BLE connected overlays blue with periodic blink handled in loop
}

// =========================
// BLE CALLBACKS
// =========================
class DemoServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    bleConnected = true;
    Serial.println("BLE CLIENT CONNECTED");
  }

  void onDisconnect(BLEServer* pServer) override {
    bleConnected = false;
    Serial.println("BLE CLIENT DISCONNECTED");
    BLEDevice::startAdvertising();
    Serial.println("BLE ADVERTISING RESTARTED");
  }
};

// =========================
// HDC302X READ
// =========================
bool readHDC302x(float &tC, float &rh) {
  uint8_t buf[6];

  Wire.beginTransmission(HDC302X_ADDR);
  Wire.write(0x24);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) {
    return false;
  }

  delay(25);

  if (Wire.requestFrom(HDC302X_ADDR, 6) != 6) {
    return false;
  }

  for (int i = 0; i < 6; i++) {
    buf[i] = Wire.read();
  }

  uint16_t rawTemp = ((uint16_t)buf[0] << 8) | buf[1];
  uint16_t rawHum  = ((uint16_t)buf[3] << 8) | buf[4];

  tC = ((float)rawTemp / 65535.0f) * 175.0f - 45.0f;
  rh = ((float)rawHum  / 65535.0f) * 100.0f;

  return true;
}

// =========================
// OLED SCREENS
// =========================
void drawHeader(const char* title) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(title);
  display.drawLine(0, 9, 127, 9, SSD1306_WHITE);
}

void drawScreen0() {
  display.clearDisplay();
  drawHeader("JBS ESP32-C6 DEMO");

  display.setCursor(0, 14);
  display.print("TEMP: ");
  display.print(tempC, 1);
  display.println(" C");

  display.setCursor(0, 26);
  display.print("HUM : ");
  display.print(humidity, 1);
  display.println(" %");

  display.setCursor(0, 40);
  display.print("WIFI: ");
  display.println(wifiConnected ? "OK" : "FAIL");

  display.setCursor(0, 52);
  display.print("BLE : ");
  display.println(bleConnected ? "CONNECTED" : "ADV");

  display.display();
}

void drawScreen1() {
  display.clearDisplay();
  drawHeader("WIFI STATUS");

  display.setCursor(0, 14);
  display.print("SSID: ");
  if (wifiConnected) {
    display.println(WIFI_SSID);
  } else {
    display.println("NOT CONNECTED");
  }

  display.setCursor(0, 28);
  display.print("IP: ");
  if (wifiConnected) {
    display.println(WiFi.localIP());
  } else {
    display.println("--");
  }

  display.setCursor(0, 42);
  display.print("RSSI: ");
  if (wifiConnected) {
    display.print(WiFi.RSSI());
    display.println(" dBm");
  } else {
    display.println("--");
  }

  display.display();
}

void drawScreen2() {
  display.clearDisplay();
  drawHeader("BLE / INPUT");

  display.setCursor(0, 14);
  display.print("BLE NAME:");
  display.setCursor(0, 26);
  display.println("JBS-ESP32C6");

  display.setCursor(0, 40);
  display.print("BLE: ");
  display.println(bleConnected ? "CONNECTED" : "ADVERTISING");

  display.setCursor(0, 52);
  display.print("BTN: ");
  display.println((digitalRead(USR_SW) == LOW) ? "PRESSED" : "RELEASED");

  display.display();
}

void drawScreen3() {
  display.clearDisplay();
  drawHeader("BOARD MAP");

  display.setCursor(0, 14);
  display.println("R=GPIO3  G=GPIO4");
  display.setCursor(0, 26);
  display.println("B=GPIO5  BTN=23");
  display.setCursor(0, 38);
  display.println("SDA=6  SCL=7");
  display.setCursor(0, 50);
  display.println("OLED 3C  HDC 44");

  display.display();
}

void updateDisplay() {
  if (!oledOk) return;

  switch (currentScreen) {
    case 0: drawScreen0(); break;
    case 1: drawScreen1(); break;
    case 2: drawScreen2(); break;
    case 3: drawScreen3(); break;
    default: drawScreen0(); break;
  }
}

// =========================
// WIFI / BLE INIT
// =========================
void initWiFi() {
  Serial.println("INIT WIFI...");
  WiFi.mode(WIFI_STA);
  delay(300);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    Serial.print(".");
    tries++;
  }
  Serial.println();

  wifiConnected = (WiFi.status() == WL_CONNECTED);

  if (wifiConnected) {
    Serial.println("WIFI CONNECTED");
    Serial.print("IP ADDRESS: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("WIFI CONNECTION FAILED");
    Serial.print("STATUS: ");
    Serial.println(WiFi.status());
  }
}

void initBLE() {
  Serial.println("INIT BLE...");
  BLEDevice::init("JBS-ESP32C6");

  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new DemoServerCallbacks());

  BLEService *service = server->createService(BLE_SERVICE_UUID);

  BLECharacteristic *characteristic = service->createCharacteristic(
    BLE_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ
  );

  characteristic->setValue("HELLO FROM JBS ESP32-C6");
  service->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(BLE_SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->start();

  Serial.println("BLE ADVERTISING STARTED");
  Serial.println("LOOK FOR: JBS-ESP32C6");
}

// =========================
// BUTTON HANDLING
// =========================
void handleButton() {
  int reading = digitalRead(USR_SW);

  if (reading != lastButtonState && millis() - lastButtonDebounce > 25) {
    lastButtonDebounce = millis();
    lastButtonState = reading;

    if (reading == LOW) {
      currentScreen++;
      if (currentScreen > 3) currentScreen = 0;

      Serial.print("BUTTON PRESSED -> SCREEN ");
      Serial.println(currentScreen);
      updateDisplay();
    }
  }
}

// =========================
// SETUP / LOOP
// =========================
void setup() {
  Serial.begin(115200);
  delay(1500);

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(USR_SW, INPUT_PULLUP);
  allOff();

  Wire.begin(I2C_SDA, I2C_SCL);

  oledOk = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  if (oledOk) {
    Serial.println("OLED INIT OK");
  } else {
    Serial.println("OLED INIT FAILED");
  }

  Serial.println();
  Serial.println("=== JBS ESP32-C6 FINAL DEMO ===");

  // quick LED self test
  setRed(true);   delay(150); setRed(false);
  setGreen(true); delay(150); setGreen(false);
  setBlue(true);  delay(150); setBlue(false);

  initWiFi();
  initBLE();

  if (readHDC302x(tempC, humidity)) {
    Serial.print("TEMP_C: ");
    Serial.print(tempC, 2);
    Serial.print("  RH_%: ");
    Serial.println(humidity, 2);
  } else {
    Serial.println("HDC302X READ FAILED");
  }

  updateStatusLed();
  updateDisplay();
}

void loop() {
  handleButton();

  if (millis() - lastSensorRead >= 1000) {
    lastSensorRead = millis();

    float newTemp = 0.0f;
    float newHum = 0.0f;
    if (readHDC302x(newTemp, newHum)) {
      tempC = newTemp;
      humidity = newHum;
    }

    wifiConnected = (WiFi.status() == WL_CONNECTED);
    updateStatusLed();
    updateDisplay();
  }

  if (millis() - lastStatusPrint >= 3000) {
    lastStatusPrint = millis();

    Serial.print("TEMP_C: ");
    Serial.print(tempC, 2);
    Serial.print(" | RH_%: ");
    Serial.print(humidity, 2);

    Serial.print(" | WIFI: ");
    Serial.print(wifiConnected ? "OK" : "FAIL");

    if (wifiConnected) {
      Serial.print(" | IP: ");
      Serial.print(WiFi.localIP());
      Serial.print(" | RSSI: ");
      Serial.print(WiFi.RSSI());
      Serial.print(" dBm");
    }

    Serial.print(" | BLE: ");
    Serial.print(bleConnected ? "CONNECTED" : "ADVERTISING");

    Serial.print(" | SCREEN: ");
    Serial.println(currentScreen);
  }

  // BLE state indicator pulse
  if (bleConnected) {
    if (millis() - lastBleBlink >= 1200) {
      lastBleBlink = millis();
      setBlue(true);
      delay(40);
      updateStatusLed();
    }
  } else {
    if (millis() - lastBleBlink >= 1800) {
      lastBleBlink = millis();
      setBlue(true);
      delay(40);
      updateStatusLed();
    }
  }
}