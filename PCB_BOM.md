# STM32 Industrial Edge Gateway - PCB BOM

## 1. Power Supply

### Power Input

- DC Power Jack (5.5 x 2.1mm)

### Protection

- Fuse
- TVS Diode: SMAF14ATR
- Reverse Polarity Protection: IRLML6402

### Voltage Regulation

- Buck Converter: MP2338GTL (Input → 5V)
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

- VDDA Ferrite Bead: BLM18AG601SN1D
- Decoupling Capacitors:
  - 100nF
  - 1uF

### Boot Configuration

- BOOT0 Jumper
- BOOT0 Pull-down Resistor

### Reset Circuit

- Reset Button
- Pull-up Resistor

### SWD Debug Interface

- SWD Header
  - SWDIO
  - SWCLK
  - NRST
  - GND
  - 3.3V

- ST-Link V2 Programmer

---

# 3. Ethernet Interface

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

- Ethernet Link/Activity LED

---

# 4. RS485 / Modbus RTU

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

### Indicator

- Modbus LED
---

# 5. microSD Storage

### Connector

- microSD Socket:
  - MCSP-Q1-08-A-SG-T/R

### Support Components

- Pull-up Resistors
- Decoupling Capacitor

### Indicator

- SD Activity LED
