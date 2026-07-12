# STM32 Industrial Edge Gateway

**An Ethernet-Based Environmental Monitoring System for Kunak AIR
Sensors**

An embedded industrial edge gateway built on the **STM32F767ZI** that
acquires environmental measurements from a **Kunak AIR** sensor using
**Modbus RTU over RS485**, processes the collected data, calculates the
**US Air Quality Index (AQI)**, stores historical records on an SD card,
and serves a real-time web dashboard over Ethernet.

The system operates entirely on a local network without requiring cloud
services, external APIs, or internet connectivity, making it suitable
for industrial monitoring and edge computing applications.

------------------------------------------------------------------------

# Features

-   Real-time environmental monitoring
-   Modbus RTU Master implementation over RS485
-   Ethernet-based embedded web server (LwIP HTTPD)
-   Dynamic web pages using SSI (Server Side Includes)
-   Local data logging to SD card (FATFS)
-   CSV export for historical measurements
-   US Air Quality Index (AQI) calculation
-   Live browser-based dashboard
-   Bilingual user interface (English / Arabic)
-   Automatic language switching
-   Sensor communication monitoring
-   Industrial status indication using LEDs
-   Fully offline operation (No Cloud Required)

------------------------------------------------------------------------

# System Architecture

``` text
                  +----------------------+
                  |      Kunak AIR       |
                  | Environmental Sensor |
                  +----------+-----------+
                             |
                     Modbus RTU (RS485)
                             |
                STM32 Industrial Edge Gateway
                             |
        +--------------------+--------------------+
        |                                         |
   SD Card Logging                       Embedded Web Server
        |                                         |
      FATFS                                  LwIP HTTPD
        |                                         |
        +--------------------+--------------------+
                             |
                      Ethernet Network
                             |
                     Web Browser Dashboard
```

------------------------------------------------------------------------

# System Workflow

1.  Poll the Kunak AIR sensor every **10 seconds** using Modbus RTU.
2.  Read environmental measurements.
3.  Convert raw register values into engineering units.
4.  Calculate the US AQI from PM2.5 and PM10.
5.  Store measurements on the SD card as CSV records.
6.  Update the embedded web dashboard.
7.  Serve the dashboard over the local Ethernet network.

------------------------------------------------------------------------

# Measured Parameters

-   Temperature
-   Humidity
-   Pressure
-   PM1
-   PM2.5
-   PM10
-   US AQI

------------------------------------------------------------------------

# Dashboard Features

-   Real-time environmental data
-   AQI visualization
-   Automatic data refresh
-   Local Ethernet access
-   Responsive design
-   English and Arabic interface
-   AQI status indication using EPA color categories

------------------------------------------------------------------------

# Data Logging

Example CSV record:

``` text
Timestamp,Temperature,Humidity,Pressure,PM1,PM2.5,PM10,AQI
2026-06-14 12:00:00,31.20,46.00,1012.0,12.5,18.3,25.7,72
```

------------------------------------------------------------------------

# Reliability Features

-   Modbus CRC16 verification
-   Communication timeout detection
-   Sensor offline monitoring
-   Runtime status indication using LEDs
-   Watchdog-based recovery support
-   Local data storage without internet dependency

------------------------------------------------------------------------

# Technologies Used

-   Embedded Systems
-   Industrial IoT (IIoT)
-   Edge Computing
-   STM32
-   Modbus RTU
-   RS485
-   Ethernet
-   LwIP
-   FATFS
-   Embedded Web Server
-   Data Logging

------------------------------------------------------------------------

<br/>

**Keep building. Keep learning.**