# STM32 Industrial Edge Gateway - PCB BOM

## 1. Power Supply

### Power Input

- DC Barrel Jack (5.5 × 2.1mm)

### Protection

- 1A Fuse
- TVS Diode: SMAJ15A
- Reverse Polarity Protection: AO3407A

### Voltage Regulation

- Buck Converter: MP2338GTL (Input → 5V)
- LDO: TLV76733DRVR (5V → 3.3V)

### Indicator

- Power LED

---

## 2. STM32 MCU Core

### Microcontroller

- STM32F407VGT6

### Clock

- HSE Crystal: SMD3225 (8MHz)
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
  - 3.3V
  - SWDIO
  - SWCLK
  - NRST
  - GND

- ST-Link V2 Programmer

---

## 3. Ethernet Interface

### Ethernet PHY

- LAN8742A

### PHY Clock

- 25MHz Crystal:
  - Abracon ABM8G
- Crystal Load Capacitors

### Ethernet Connector

- RJ45 with Integrated Magnetics:
  - HanRun HR911105A

### Protection

- Ethernet TVS Protection:
  - SRV05-4

### Indicators

- Ethernet Link LED
- Ethernet Activity LED

---

## 4. RS485 / Modbus RTU

### Transceiver

- MAX3485ESA

### Connector

- 2-Pin Terminal Block
  - A
  - B

### Protection

- RS485 TVS:
  - SM712

### Bus Components

- 120Ω Termination Resistor
- Bias Resistors

### Indicator

- Modbus LED
---

## 5. microSD Storage

### Connector

- microSD Socket:
  - Hirose DM3AT

### Support Components

- Pull-up Resistors
- Decoupling Capacitor

### Indicator

- SD Activity LED
