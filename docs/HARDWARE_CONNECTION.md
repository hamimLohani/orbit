# ORBIT v2 — Hardware Wiring & Connection Guide
**Target Microcontroller: ESP32-S3 (Dual-Core LX7, 44-Pin / DevKitC-1)**

This guide provides the complete, pin-by-pin hardware schematic, wiring tables, circuit diagrams, and assembly instructions for building the ORBIT hardware prototype.

---

## 1. ST7735 1.8" TFT + SD Card Module (Exact Photo Match)

The board in your photo is the standard **Blue PCB 1.8" ST7735 SPI TFT with MicroSD Socket**. It features two pin headers:
- **Left 16-Pin Header**: Exposes both the TFT display and the SD Card slot.
- **Right 8-Pin Header**: Exposes the TFT display only.

### ⚠️ Crucial Power & Jumper J1 Configuration

Notice the silkscreen box next to regulator `U21`:
```
   Vin
   VCC = 5V   -> J1 OPEN
   VCC = 3.3V -> J1 SHORT
```
In your photo, **J1 is OPEN** (unbridged). You have two power options:
- **Option A (Recommended — No Soldering)**: Connect module **`VCC`** to the ESP32-S3 **`5V` / `VIN` pin** (leave J1 OPEN). The onboard regulator `U21` drops 5V down to a stable 3.3V for both the display and the SD card.
- **Option B**: If you connect **`VCC`** to the ESP32-S3 **`3.3V` pin**, you **MUST solder/bridge jumper J1** with a small solder blob. *(If J1 is left OPEN while feeding 3.3V, the onboard regulator will drop voltage to ~2.2V, causing a white screen or SD card mount failure!)*

---

### 1.1 Pin-by-Pin Wiring Table (16-Pin Header — Display + SD Card)

Use the 16-pin header on the left to connect both the display and SD card slot:

| Pin # | Module Silkscreen Label | Group | ESP32-S3 GPIO / Pin | Recommended Wire Color | Function & Notes |
| :---: | :--- | :--- | :--- | :--- | :--- |
| **1** | **GND** | Power | **GND** | ⚫ Black | System Ground |
| **2** | **VCC** | Power | **5V / VIN** (J1 OPEN) or **3.3V** (J1 SHORT) | 🔴 Red | Module Power (See J1 rule above) |
| **3** | **NC** | — | *Do Not Connect* | — | No Connection |
| **4** | **BL** | TFT | **GPIO 21** | 🟣 Purple | Backlight Dimming (LEDC PWM) |
| **5** | **NC** | — | *Do Not Connect* | — | No Connection |
| **6** | **RST** | TFT | **GPIO 14** | ⚪ White | Display Hardware Reset |
| **7** | **RS** | TFT | **GPIO 9** | 🟠 Orange | Register Select (Command / Data) |
| **8** | **SDA** | TFT | **GPIO 11** | 🟢 Green | SPI MOSI (Data from ESP32 to TFT) |
| **9** | **SCK** | TFT | **GPIO 12** | 🟡 Yellow | SPI Clock |
| **10** | **CS** | TFT | **GPIO 10** | 🔵 Blue | TFT Chip Select (Active LOW) |
| **11** | **CLK** | SDCard | **GPIO 12** | 🟡 Yellow | Shared SPI Clock (ties with Pin 9) |
| **12** | **MISO** | SDCard | **GPIO 13** | 🔘 Gray | SPI MISO (Data from SD to ESP32) |
| **13** | **MOSI** | SDCard | **GPIO 11** | 🟢 Green | Shared SPI MOSI (ties with Pin 8) |
| **14** | **CS** | SDCard | **GPIO 47** | 🟤 Brown | SD Card Chip Select (Active LOW) |
| **15** | **NC** | — | *Do Not Connect* | — | No Connection |
| **16** | **NC** | — | *Do Not Connect* | — | No Connection |

> [!TIP]
> **SPI Bus Sharing on Breadboard**:
> - Connect Pin 8 (`SDA`) and Pin 13 (`MOSI`) together on your breadboard rail, then run a single wire to **GPIO 11**.
> - Connect Pin 9 (`SCK`) and Pin 11 (`CLK`) together on your breadboard rail, then run a single wire to **GPIO 12**.

---

### 1.2 Alternative: 8-Pin Header (Display-Only Testing)

If you only want to test the TFT display without the SD card, you can use the 8-pin header on the right:

| Pin # | Silkscreen Label | ESP32-S3 GPIO / Pin | Function |
| :---: | :--- | :--- | :--- |
| **1** | **1-RST** | **GPIO 14** | Display Reset |
| **2** | **2-CS** | **GPIO 10** | TFT Chip Select |
| **3** | **3-D/C** | **GPIO 9** | Data / Command (RS) |
| **4** | **4-DIN** | **GPIO 11** | SPI MOSI (SDA) |
| **5** | **5-CLK** | **GPIO 12** | SPI Clock (SCK) |
| **6** | **6-VCC** | **5V / VIN** (J1 OPEN) | Power Input |
| **7** | **7-BL** | **GPIO 21** | Backlight Control |
| **8** | **8-GND** | **GND** | Ground |

---

## 2. Other Peripherals Pinout (Audio, Sensors, Buttons)

All GPIO assignments below match [`include/config.h`](../include/config.h) exactly:

| Subsystem / Module | Module Pin | ESP32-S3 GPIO | Wire Color (Rec.) | Operating Voltage | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **INMP441 I2S Mic**| **VDD** | **3.3V** | 🔴 Red | 3.3V | ⚠️ **Do NOT connect to 5V!** |
| | **GND** | **GND** | ⚫ Black | 0V | Common Ground |
| | **L/R** | **GND** | ⚫ Black | 0V | Pull to GND for Left Channel (mono sampling) |
| | **SCK / BCLK**| **GPIO 5** | 🟡 Yellow | 3.3V | I2S Bit Clock |
| | **WS / LRCLK**| **GPIO 4** | 🔵 Blue | 3.3V | I2S Word Select |
| | **SD / DOUT** | **GPIO 6** | 🟢 Green | 3.3V | I2S Serial Data Out from microphone |
| **DHT11 Sensor** | **VCC** | **3.3V** | 🔴 Red | 3.3V | 3.3V power |
| | **GND** | **GND** | ⚫ Black | 0V | Common Ground |
| | **DATA** | **GPIO 7** | 🟡 Yellow | 3.3V | Needs 4.7kΩ–10kΩ pull-up resistor to 3.3V |
| **Indicator LED** | **Anode (+)** | **GPIO 2** | 🔴 Red | 3.3V | Connect through **220Ω or 330Ω resistor** |
| | **Cathode (-)**| **GND** | ⚫ Black | 0V | Short leg to GND |
| **WS2812 RGB** | **Built-in** | **GPIO 48** | — | 3.3V | Built into ESP32-S3 DevKit boards |
| **5× Buttons** | **BTN_UP** | **GPIO 1** | 🔵 Blue | GND | Directional UP / Thrust (Active LOW to GND) |
| *(Active LOW)* | **BTN_DOWN** | **GPIO 3** | 🟢 Green | GND | Directional DOWN / Drift reverse (Active LOW to GND) |
| | **BTN_LEFT** | **GPIO 17** | 🟠 Orange | GND | Directional LEFT / Steer left / Dim (Active LOW to GND) |
| | **BTN_RIGHT**| **GPIO 18** | 🟡 Yellow | GND | Directional RIGHT / Steer right / Brighten (Active LOW to GND) |
| | **BTN_ENTER**| **GPIO 16** | 🟣 Purple | GND | Tap: Enter \| Hold 0.5s: Back \| Hold 2.0s: Home |

---

## 3. Visual Connection Diagram (Your Exact Display Module)

```
       +-------------------------------------------------------------+
       |   YOUR ST7735 1.8" TFT MODULE (16-Pin Left Header View)     |
       +-------------------------------------------------------------+
         16: NC   -------------> No Connection
         15: NC   -------------> No Connection
         14: CS   (SDCard) -----> ESP32 GPIO 47 (SD Card CS)
         13: MOSI (SDCard) --+
                             +--> ESP32 GPIO 11 (SPI MOSI)
          8: SDA  (TFT)    --+
         12: MISO (SDCard) -----> ESP32 GPIO 13 (SPI MISO)
         11: CLK  (SDCard) --+
                             +--> ESP32 GPIO 12 (SPI SCK)
          9: SCK  (TFT)    --+
         10: CS   (TFT)    -----> ESP32 GPIO 10 (TFT CS)
          7: RS   (TFT)    -----> ESP32 GPIO 9  (TFT DC / RS)
          6: RST  (TFT)    -----> ESP32 GPIO 14 (TFT Reset)
          5: NC   -------------> No Connection
          4: BL   (TFT)    -----> ESP32 GPIO 21 (PWM Backlight)
          3: NC   -------------> No Connection
          2: VCC  --------------> ESP32 5V / VIN pin (J1 is OPEN!)
          1: GND  --------------> ESP32 GND
```

> [!IMPORTANT]
> **SPI Bus Mutex in Firmware**: The firmware handles bus contention automatically using FreeRTOS mutexes (`RenderEngine::getSpiMutex()`), holding the inactive CS line `HIGH` during transfers so that image and audio file operations never corrupt screen pixels.

---

## 4. Detailed Wiring Schematics (Audio, Sensors, Buttons)

### 4.1 INMP441 I2S Digital MEMS Microphone

The INMP441 delivers 24-bit studio-grade digital audio straight to the ESP32-S3 over I2S DMA.

```
       +-----------------------+
       |   INMP441 MEMS MIC    |
       +-----------------------+
         VDD   GND   SD   WS  SCK  L/R
          |     |     |    |   |    |
          |     |     |    |   |    +-------+ (Tie L/R to GND for Left Channel)
          |     |     |    |   |            |
          |     +-----+----+---+------------+---> ESP32 GND
          |           |    |   |
          |           |    |   +----------------> ESP32 GPIO 5 (I2S SCK / BCLK)
          |           |    +--------------------> ESP32 GPIO 4 (I2S WS / LRCLK)
          |           +-------------------------> ESP32 GPIO 6 (I2S SD / DOUT)
          +-------------------------------------> ESP32 3.3V (NEVER CONNECT TO 5V!)
```

> [!CAUTION]
> The INMP441 is a **3.3V strictly rated device**. Applying 5V will destroy the MEMS microphone element permanently. Always power from the ESP32-S3 `3V3` pin.

---

### 4.2 DHT11 Environmental Sensor

```
          +------------------+
          |   DHT11 SENSOR   |
          +------------------+
            VCC   DATA   GND
             |     |      |
             |     +--[ 4.7kΩ - 10kΩ Resistor ]--+
             |     |                             |
             +-----+-----------------------------+---> ESP32 3.3V
             |     |
             |     +---------------------------------> ESP32 GPIO 7 (Data)
             |
             +---------------------------------------> ESP32 GND
```

> [!NOTE]
> If using a 3-pin DHT11 module mounted on a small PCB (blue breakout board with an SMD resistor), the pull-up resistor is already included on the module. If using a bare 4-pin DHT11, install a 4.7kΩ to 10kΩ resistor between `VCC` and `DATA`.

---

### 4.3 Dedicated Indicator Alert LED

```
 ESP32 GPIO 2 ----[ 220Ω Resistor ]----(>| Anode) LED (Cathode |)---- ESP32 GND
```
- **Long Leg (Anode +)**: Connects to 220Ω resistor from GPIO 2.
- **Short Leg / Flat Side (Cathode -)**: Connects to Ground rail.

---

### 4.4 5-Button Navigation Matrix (Active LOW)

All five push buttons utilize the ESP32-S3's internal pull-up resistors (`INPUT_PULLUP`). **No external pull-up resistors are required!** Simply wire one terminal of each switch to the designated GPIO pin and the opposite terminal to Common Ground (`GND`).

#### Visual D-Pad & Control Layout Preview
```
                   +-------------------+
                   |    BTN_UP (G1)    |
                   +---------+---------+
                             |
+-------------------+        |        +-------------------+
|   BTN_LEFT (G17)  | <------+------> |  BTN_RIGHT (G18)  |
+-------------------+        |        +-------------------+
                             |
                   +---------+---------+
                   |   BTN_DOWN (G3)   |
                   +-------------------+

                   +-------------------+
                   |   BTN_ENTER(G16)  |
                   +-------------------+
            • Short Tap (<1.0s): Select / Open / Action
            • Hold 1.0s        : Back to Previous / Close
            • Hold 2.5s        : Return Home Dashboard
```

#### Electrical Wiring Schematic (Per Button)
```
   ESP32-S3 Pin                     Tactile Switch (Active LOW)
   +------------+                        o     o
   |   GPIO 1   | (UP)    ---------------|  \  |----+
   |   GPIO 3   | (DOWN)  ---------------|  \  |----+
   |   GPIO 17  | (LEFT)  ---------------|  \  |----+
   |   GPIO 18  | (RIGHT) ---------------|  \  |----+
   |   GPIO 16  | (ENTER) ---------------|  \  |----+
   |            |                                   |
   |    GND     | ----------------------------------+ (Common GND Rail)
   +------------+
```

#### Button Action Map:
| Button | GPIO Pin | Instant Press-Down | Short Click (< 1.0s) | Hold 1.0s (Back) | Long Hold 2.5s (Home) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **`ENTER`** | **GPIO 16** | Debounced state timing starts | **Select / Enter / Open Carousel / Play-Pause / Toggle** | **Back to previous screen / Dismiss launcher** | **Instant return to Home Dashboard from anywhere** |
| **`LEFT`** | **GPIO 17** | **Instant Steer Left / Prev App / Dim** | — | Directional auto-repeat (120ms) | — |
| **`RIGHT`** | **GPIO 18** | **Instant Steer Right / Next App / Brighten** | — | Directional auto-repeat (120ms) | — |
| **`UP`** | **GPIO 1** | **Instant Thruster / Steer Up / Value Up** | — | Directional auto-repeat (120ms) | — |
| **`DOWN`** | **GPIO 3** | **Instant Drift Invert / Steer Down / Value Down** | — | Directional auto-repeat (120ms) | — |

---

## 5. Physical Breadboard Layout Diagram

```
           +---------------------[ ESP32-S3 ]---------------------+
           |                                                      |
           | [GND] ----------------- Common Ground Rail --------- |
           | [3V3] ----------------- 3.3V Power Rail ------------ |
           |                                                      |
           | [GPIO 1]  ------------> BTN_UP ----------------(GND) |
           | [GPIO 2]  ------------> [220Ω] -> Alert LED ---(GND) |
           | [GPIO 3]  ------------> BTN_DOWN --------------(GND) |
           | [GPIO 4]  ------------> INMP441 WS                   |
           | [GPIO 5]  ------------> INMP441 SCK                  |
           | [GPIO 6]  ------------> INMP441 SD                   |
           | [GPIO 7]  ------------> DHT11 Data                   |
           | [GPIO 9]  ------------> ST7735 DC                    |
           | [GPIO 10] ------------> ST7735 CS                    |
           | [GPIO 11] ------------> SPI MOSI (TFT + SD Card)     |
           | [GPIO 12] ------------> SPI SCK  (TFT + SD Card)     |
           | [GPIO 13] ------------> SPI MISO (SD Card only)      |
           | [GPIO 14] ------------> ST7735 RESET                 |
           | [GPIO 16] ------------> BTN_ENTER -------------(GND) |
           | [GPIO 17] ------------> BTN_LEFT --------------(GND) |
           | [GPIO 18] ------------> BTN_RIGHT -------------(GND) |
           | [GPIO 21] ------------> ST7735 BLK (Backlight)       |
           | [GPIO 47] ------------> MicroSD CS                   |
           | [GPIO 48] ------------> Built-in WS2812 RGB LED      |
           +------------------------------------------------------+
           * BTN_ENTER (GPIO 16): Tap = Enter | 0.5s Hold = Back | 2.0s Hold = Home
```

---

## 6. Hardware Assembly Checklist & Sanity Testing

Before plugging in the USB cable, perform this 5-point safety check:

1. [ ] **Ground Continuity**: Verify all `GND` pins from the ST7735, SD card, INMP441, DHT11, LED, and buttons tie back to the ESP32-S3 `GND` pin.
2. [ ] **No 5V to Mic**: Double-check that the INMP441 `VDD` is connected to **3.3V**, never 5V.
3. [ ] **INMP441 L/R Pin**: Confirm the `L/R` pin on the microphone is tied to **GND** (selects Left audio channel).
4. [ ] **MicroSD Card Format**: Format the microSD card as **FAT32** (or FAT16) with 32KB allocation size.
5. [ ] **Backlight Connection**: Ensure `PIN_TFT_BL` (GPIO 21) is connected. The ST7735 display will appear completely black if the backlight pin is left floating!

---

## 7. Troubleshooting Hardware Issues

| Symptom | Probable Cause | Quick Fix |
| :--- | :--- | :--- |
| **All White Screen** | Missing SPI clock/MOSI or wrong CS | Verify GPIO 11 (MOSI), GPIO 12 (SCK), and GPIO 10 (CS). Ensure display `VCC` is receiving 3.3V/5V. |
| **Completely Black Screen** | Backlight LED pin not powered | Ensure `PIN_TFT_BL` (GPIO 21) is connected; test by tying TFT `BLK` directly to 3.3V temporarily. |
| **SD Card Initialization Failed** | Missing SD CS pin or slow bus | Ensure GPIO 47 is wired to `SD_CS`. Verify SD card is formatted FAT32. |
| **Audio Spectrum Flat / Zero dB** | INMP441 L/R floating or swapped clock | Check that `L/R` is firmly tied to GND. Verify GPIO 5 is SCK (BCLK) and GPIO 4 is WS (LRCLK). |
| **DHT11 Shows NaN or 0°C** | Missing pull-up resistor on DATA | Add a 4.7kΩ–10kΩ pull-up resistor between GPIO 7 (DATA) and 3.3V. |
| **Ghost / Unresponsive Buttons** | Floating button input | Buttons must switch to GND; firmware enables internal pull-ups on GPIO 1, 3, 16, 17, 18. |
