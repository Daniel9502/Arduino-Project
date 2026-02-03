# UV-C Disinfection & Safety Control System

## Overview
This project is a safety and control system designed for UV-C disinfection applications. It automates the operation of three relay channels (typically for UV-C lamps) based on strict environmental and safety parameters. The system ensures that disinfection only occurs when the area is free of human presence and air quality is within safe limits.

## Hardware Configuration

### Microcontroller
*   Arduino (Uno/Nano/Mega compatible)

### Sensors
1.  **Motion Detection:** 4x PIR Sensors to cover the entire room.
2.  **Temperature & Humidity:** AHT20 (I2C).
3.  **Air Quality:** ScioSense ENS160 (I2C) for AQI, TVOC, and eCO2.
4.  **Ozone (O3):** ZE03-O3 Module (UART).
5.  **Multi-Gas:** DFRobot MICS-5524 (Analog) for CO, CH4, Ethanol, H2, NH3, NO2.
6.  **Specific Gas Sensors:**
    *   **MQ-02:** Smoke, LPG, CO.
    *   **MQ-05:** LPG, Natural Gas.
    *   **MQ-06:** Isobutane, Propane.

### Actuators
*   **Relays:** 3 channels (Active LOW).
*   **Buzzer:** For status and alarm sounds.

## Pin Mapping

| Component | Pin | Type | Description |
| :--- | :--- | :--- | :--- |
| **Relay 1** | D2 | Output | UV-C Lamp 1 Control |
| **Relay 2** | D4 | Output | UV-C Lamp 2 Control |
| **Relay 3** | D7 | Output | UV-C Lamp 3 Control |
| **Buzzer** | D10 | Output | Alarm Output |
| **PIR 1** | D3 | Input | Motion Sensor 1 |
| **PIR 2** | D6 | Input | Motion Sensor 2 |
| **PIR 3** | D9 | Input | Motion Sensor 3 |
| **PIR 4** | D11 | Input | Motion Sensor 4 |
| **ZE03 RX** | D8 | SoftwareSerial | Connect to Sensor TX |
| **ZE03 TX** | D12 | SoftwareSerial | Connect to Sensor RX |
| **MICS Power**| D5 | Output | Power control for MICS sensor |
| **MICS ADC** | A2 | Analog | MICS sensor data |
| **MQ-06** | A0 | Analog | Isobutane Sensor |
| **MQ-02** | A1 | Analog | Smoke/LPG Sensor |
| **MQ-05** | A3 | Analog | LPG Sensor |
| **I2C Bus** | SDA/SCL | I2C | AHT20 & ENS160 connection |

## System Logic

### 1. Initialization
*   On startup, the system verifies the connection of all sensors.
*   Status messages are printed to the Serial Monitor (9600 baud).
*   A startup tone (1000Hz) indicates the system is ready.

### 2. Motion Safety (PIR)
*   **Immediate Stop:** If any of the 4 PIR sensors detect motion, all relays are immediately turned **OFF** and a warning tone plays.
*   **Safety Lock:** If presence is detected continuously for **20 seconds**, the system enters a "Locked" state (`sistemOprit`).
    *   In this state, the system will **not** restart automatically even if motion stops.
    *   **Reset:** To unlock the system, the operator must send the code `115` via the Serial Monitor.

### 3. Air Quality & Gas Safety
The system continuously monitors air quality. If any threshold is exceeded, relays are turned OFF and an alarm sounds.

*   **Thresholds:**
    *   **MQ-02 / MQ-05 / MQ-06:** Value > 500.
    *   **ENS160:** eCO2 > 1000 ppm, AQI > 100, or TVOC > 800 ppb.
    *   **Ozone (ZE03):** Concentration > 0.1 ppm.
*   **Alarm Behavior:**
    *   General Gas: Relays OFF, Alarm sounds.
    *   MQ-05 (LPG): Enters a blocking loop with a specific alarm tone (2500Hz) until gas levels drop below the threshold.

### 4. Normal Operation
*   If **no motion** is detected AND **air quality** is safe:
    *   Relay 1 turns ON.
    *   (500ms delay)
    *   Relay 2 turns ON.
    *   (500ms delay)
    *   Relay 3 turns ON.

## Serial Commands
The system accepts commands via the Serial Monitor:
*   `stop` : Forces all relays to turn OFF immediately.
*   `115` : Resets the system if it is in the "Locked" state (after prolonged human presence).

## Libraries Required
Ensure the following libraries are installed in your Arduino IDE:
*   `Wire.h` (Standard)
*   `Adafruit_AHTX0.h`
*   `ScioSense_ENS160.h`
*   `DFRobot_MICS.h`
*   `SoftwareSerial.h` (Standard)

## Author / License
*   Project: CodTzUVC
```

<!--
[PROMPT_SUGGESTION]Cum pot adăuga un ecran LCD pentru a afișa starea sistemului?[/PROMPT_SUGGESTION]
[PROMPT_SUGGESTION]Explică cum funcționează logica de blocare la 20 de secunde.[/PROMPT_SUGGESTION]
