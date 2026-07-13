# STM32 Industrial Edge Gateway - PCB BOM

## 1. Power Supply

### Power Input

- DC Power Jack (5.5 x 2.1mm)

### Protection

- Fuse
- Reverse Polarity Protection: IRLML6402
- TVS Diode: SMAF14ATR

### Voltage Regulation

- Buck Converter: MP1584EN (Input → 5V)
- LDO: TLV76733DRVR (5V → 3.3V)

### Indicator

- Power LED + Current Limiting Resistor

---

# 2. STM32 MCU Core

### Microcontroller

- STM32F407VGT6

### Clock

- HSE Crystal: ECS-80-10-33-CHN-TR3 (8MHz)
- Crystal Load Capacitors

### Power Filtering

- VDDA Ferrite Bead: 490-1014-1-ND
- Decoupling Capacitors:
  - 100nF
  - 1uF
  - 4.7uF
  - 10uF

### Boot Configuration

- BOOT0 Jumper
- BOOT0 Pull-down Resistor

### Reset Circuit

- Reset Button
- Pull-up Resistor
- Reset Capacitor

---

# 3. SWD Debug Interface

- SWD Header
  - SWDIO
  - SWCLK
  - NRST
  - 3.3V
  - GND

- ST-Link V2 Programmer

---

# 4. Ethernet Interface

### Ethernet PHY

- LAN8742A-CZ-CT-ND

### PHY Clock

- 25MHz Crystal:
  - SXT32420AA48-25.000M
- Crystal Load Capacitors

### Ethernet Connector

- RJ45 with Integrated Magnetics:
  - 277-1663966-ND

### Protection

- Ethernet TVS Protection:
  - SRV05-4HTG

### Indicators

- Ethernet Link LED
- Ethernet Activity LED

---

# 5. RS485 / Modbus RTU

### Transceiver

- MAX3485ECSA+CT-ND

### Connector

- 2-Pin Terminal Block
  - A
  - B

### Protection

- RS485 TVS:
  - SM712.TCT

### Bus Components

- 120Ω Termination Resistor
- Termination Jumper
- Bias Resistors

---

# 6. microSD Storage

### Connector

- microSD Socket:
  - MCSP-Q1-08-A-SG-T/R

### Support Components

- Pull-up Resistors
- Decoupling Capacitor

### Indicator

- SD Activity LED

---

# 7. Status Indicators

### LEDs

- Power LED
- Modbus RX LED
- Modbus TX LED
- SD Activity LED
- Ethernet Link LED
- Ethernet Activity LED

### Each LED Requires

- Current Limiting Resistor

---

# 8. Mechanical & PCB

- Mounting Holes ×4
- Test Points
- PCB Fiducials
- Board Label / Revision

---

# 9. Common Components

## Resistors

- Pull-up Resistors
- Pull-down Resistors
- LED Current Limiting Resistors
- Ethernet Bias Resistors
- RS485 Bias Resistors
- 120Ω Termination Resistor

## Capacitors

- 100nF Ceramic Capacitors
- 1uF Capacitors
- 4.7uF Capacitors
- 10uF Capacitors
- Crystal Load Capacitors
