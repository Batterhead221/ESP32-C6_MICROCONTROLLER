## esp32-c6_microcontroller

custom esp32-c6 development board with rgb status led, user input, i2c expansion, oled support, and onboard environmental sensing.

*initial board bring-up demo showing oled output, live temp/hum readout, uart programming setup, and rgb led status.*

## demo features

* esp32-c6 microcontroller core
* usb-c power input
* rgb status led
* user button
* i2c header
* oled display support
* onboard temp / hum sensor
* uart program header for bring-up and debug
## bring-up status

this board has been successfully brought up far enough to verify core power, programming, gpio, i2c, oled, and environmental sensor functionality.

### verified working

* usb-c power input
* 3.3v regulation
* esp32-c6 boot from flash
* uart programming through j3
* reset button
* user button
* rgb led
* i2c bus
* oled display at `0x3c`
* temp/hum sensor at `0x44`

### verified gpio map

* `gpio3` = red
* `gpio4` = green
* `gpio5` = blue
* `gpio23` = user switch
* `gpio6` = sda
* `gpio7` = scl

### rgb led notes

the rgb led is **active-low**:

* `low` = on
* `high` = off

### uart programming notes

uart programming through the **j3 program header** is working.

working connections:

* usb-uart `tx` -> board `esp_rxd`
* usb-uart `rx` -> board `esp_txd`
* usb-uart `gnd` -> board `gnd`

current working upload sequence:

1. hold **boot**
2. start upload
3. tap **reset** when the tool begins connecting
4. release **boot** after connection
5. press **reset** after upload to run the application

### i2c device detection

the following i2c devices were detected during bring-up:

* `0x3c` = oled
* `0x44` = temp/hum sensor

### sample verified sensor reading

* **29.2c**
* **35.0% rh**

### current status

the board has successfully run a combined demo with:

* rgb led control
* user button input
* i2c bus scan
* oled output
* live temperature / humidity readout

### open items

* native usb enumeration still needs separate debug / validation
* programming flow is currently manual through uart header


Designed & Engineered by Brandon Shelly
