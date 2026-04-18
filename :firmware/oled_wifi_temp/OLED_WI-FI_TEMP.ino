#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =========================
// WIFI SETTINGS
// =========================
const char* WIFI_SSID = "SchmungusAmongus";
const char* WIFI_PASS = "95ModelT";

// =========================
// VERIFIED BOARD MAP
// =========================
const int LED_R   = 3;   // active-low
const int LED_G   = 4;   // active-low
const int LED_B   = 5;   // active-low
const int I2C_SDA = 6;
const int I2C_SCL = 7;
const int USR_SW  = 23;  // active-low button with INPUT_PULLUP

#define OLED_ADDR      0x3C
#define HDC302X_ADDR   0x44
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool oledOk = false;
bool wifiConnected = false;

float tempC = 0.0f;
float humidity = 0.0f;

unsigned long lastSensorRead = 0;
unsigned long lastStatusPrint = 0;
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

void updateStatusLed() {
  bool buttonPressed = (digitalRead(USR_SW) == LOW);

  allOff();

  if (buttonPressed) {
    setRed(true);
    return;
  }

  if (wifiConnected) {
    setGreen(true);
  } else {
    setBlue(true);
  }
}

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
// OLED
// =========================
void drawHeader(const __FlashStringHelper* title) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(title);
  display.drawLine(0, 9, 127, 9, SSD1306_WHITE);
}

void drawScreen0() {
  display.clearDisplay();
  drawHeader(F("ESP32-C6 DEMO"));

  display.setCursor(0, 14);
  display.print(F("TEMP: "));
  display.print(tempC, 1);
  display.println(F(" C"));

  display.setCursor(0, 26);
  display.print(F("HUM : "));
  display.print(humidity, 1);
  display.println(F(" %"));

  display.setCursor(0, 40);
  display.print(F("WIFI: "));
  display.println(wifiConnected ? F("OK") : F("FAIL"));

  display.setCursor(0, 52);
  display.print(F("BTN : "));
  display.println((digitalRead(USR_SW) == LOW) ? F("PRESSED") : F("RELEASED"));

  display.display();
}

void drawScreen1() {
  display.clearDisplay();
  drawHeader(F("WIFI STATUS"));

  display.setCursor(0, 14);
  display.print(F("SSID: "));
  if (wifiConnected) {
    display.println(WIFI_SSID);
  } else {
    display.println(F("NOT CONNECTED"));
  }

  display.setCursor(0, 28);
  display.print(F("IP: "));
  if (wifiConnected) {
    display.println(WiFi.localIP());
  } else {
    display.println(F("--"));
  }

  display.setCursor(0, 42);
  display.print(F("RSSI: "));
  if (wifiConnected) {
    display.print(WiFi.RSSI());
    display.println(F(" dBm"));
  } else {
    display.println(F("--"));
  }

  display.display();
}

void drawScreen2() {
  display.clearDisplay();
  drawHeader(F("BOARD MAP"));

  display.setCursor(0, 14);
  display.println(F("R=3  G=4  B=5"));
  display.setCursor(0, 26);
  display.println(F("BTN=23"));
  display.setCursor(0, 38);
  display.println(F("SDA=6  SCL=7"));
  display.setCursor(0, 50);
  display.println(F("OLED=3C HDC=44"));

  display.display();
}

void updateDisplay() {
  if (!oledOk) return;

  switch (currentScreen) {
    case 0: drawScreen0(); break;
    case 1: drawScreen1(); break;
    case 2: drawScreen2(); break;
    default: drawScreen0(); break;
  }
}

// =========================
// WIFI
// =========================
void initWiFi() {
  Serial.println(F("=== INIT WIFI ==="));
  Serial.print(F("SSID: "));
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  delay(500);

  WiFi.disconnect(true, true);
  delay(1000);

  Serial.println(F("CALLING WiFi.begin()"));
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    Serial.print(F("."));
    tries++;
  }
  Serial.println();

  wl_status_t st = WiFi.status();
  wifiConnected = (st == WL_CONNECTED);

  Serial.print(F("FINAL WIFI STATUS: "));
  Serial.println((int)st);

  if (wifiConnected) {
    Serial.println(F("WIFI CONNECTED"));
    Serial.print(F("IP: "));
    Serial.println(WiFi.localIP());
    Serial.print(F("RSSI: "));
    Serial.print(WiFi.RSSI());
    Serial.println(F(" dBm"));
  } else {
    Serial.println(F("WIFI FAILED"));
  }
}

// =========================
// BUTTON
// =========================
void handleButton() {
  int reading = digitalRead(USR_SW);

  if (reading != lastButtonState && millis() - lastButtonDebounce > 25) {
    lastButtonDebounce = millis();
    lastButtonState = reading;

    if (reading == LOW) {
      currentScreen++;
      if (currentScreen > 2) currentScreen = 0;

      Serial.print(F("SCREEN -> "));
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
  Serial.println(oledOk ? F("OLED OK") : F("OLED FAIL"));

  Serial.println();
  Serial.println(F("=== ESP32-C6 WIFI SENSOR DEMO ==="));

  // quick LED self test
  setRed(true);   delay(120); setRed(false);
  setGreen(true); delay(120); setGreen(false);
  setBlue(true);  delay(120); setBlue(false);

  initWiFi();

  if (readHDC302x(tempC, humidity)) {
    Serial.print(F("TEMP_C: "));
    Serial.print(tempC, 2);
    Serial.print(F(" RH_%: "));
    Serial.println(humidity, 2);
  } else {
    Serial.println(F("HDC READ FAIL"));
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

    Serial.print(F("TEMP_C: "));
    Serial.print(tempC, 2);
    Serial.print(F(" | RH_%: "));
    Serial.print(humidity, 2);
    Serial.print(F(" | WIFI: "));
    Serial.print(wifiConnected ? F("OK") : F("FAIL"));

    if (wifiConnected) {
      Serial.print(F(" | IP: "));
      Serial.print(WiFi.localIP());
      Serial.print(F(" | RSSI: "));
      Serial.print(WiFi.RSSI());
      Serial.print(F(" dBm"));
    }

    Serial.print(F(" | SCREEN: "));
    Serial.println(currentScreen);
  }
}