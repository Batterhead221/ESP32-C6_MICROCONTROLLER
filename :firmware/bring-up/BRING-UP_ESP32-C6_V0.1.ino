#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int LED_R = 3;
const int LED_G = 4;
const int LED_B = 5;
const int USR_SW = 23;

const int I2C_SDA = 6;
const int I2C_SCL = 7;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C
#define HDC302X_ADDR 0x44

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool oledOk = false;

void allOff() {
  digitalWrite(LED_R, HIGH);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, HIGH);
}

void setRed(bool on)   { digitalWrite(LED_R, on ? LOW : HIGH); }
void setGreen(bool on) { digitalWrite(LED_G, on ? LOW : HIGH); }
void setBlue(bool on)  { digitalWrite(LED_B, on ? LOW : HIGH); }

bool readHDC302x(float &tempC, float &rh) {
  uint8_t buf[6];

  // Trigger on-demand measurement: 0x24 0x00
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

  tempC = ((float)rawTemp / 65535.0f) * 175.0f - 45.0f;
  rh    = ((float)rawHum  / 65535.0f) * 100.0f;

  return true;
}

void showOLED(float tempC, float rh, bool pressed) {
  if (!oledOk) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ESP32-C6 TEMP DEMO");

  display.setTextSize(2);
  display.setCursor(0, 16);
  display.print(tempC, 1);
  display.println(" C");

  display.setCursor(0, 40);
  display.print(rh, 1);
  display.println(" %");

  display.setTextSize(1);
  display.setCursor(92, 0);
  display.print("BTN:");
  display.println(pressed ? "P" : "R");

  display.display();
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(USR_SW, INPUT_PULLUP);
  allOff();

  Wire.begin(I2C_SDA, I2C_SCL);

  oledOk = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

  if (oledOk) {
    setGreen(true);
    delay(150);
    setGreen(false);
  } else {
    setRed(true);
    delay(150);
    setRed(false);
  }

  Serial.println();
  Serial.println("ESP32-C6 HDC302X + OLED DEMO START");
}

void loop() {
  bool pressed = (digitalRead(USR_SW) == LOW);
  float tempC = 0.0f;
  float rh = 0.0f;

  if (readHDC302x(tempC, rh)) {
    Serial.print("TEMP_C: ");
    Serial.print(tempC, 2);
    Serial.print("  RH_%: ");
    Serial.println(rh, 2);

    showOLED(tempC, rh, pressed);

    if (pressed) {
      setRed(true);
    } else {
      setRed(false);
    }

    setBlue(true);
    delay(50);
    setBlue(false);
  } else {
    Serial.println("HDC302X READ FAILED");
    setRed(true);
    delay(100);
    setRed(false);
  }

  delay(1000);
}