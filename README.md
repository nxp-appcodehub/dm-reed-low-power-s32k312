# NXP Application Code Hub
[<img src="https://mcuxpresso.nxp.com/static/icon/nxp-logo-color.svg" width="100"/>](https://www.nxp.com)

## Wake Up from Sleep Through Magnetic Detection
This example demonstrates how to wake up the S32K312 MCU from Standby mode through magnetic detection using the Reed Click board. The reed switch is configured as a WKPU wake-up source, allowing the device to remain in a low-power state while monitoring a magnetic contact. When the magnet is removed, the MCU wakes up, displays the event on an SSD1306 OLED display, and activates an LED alarm indication. The application illustrates a low-power door security monitoring use case using the NXP S32K3 RTD drivers and S32 Design Studio.

[<p align="center"><img src="images/FRDM-A-S32K312_Reed_Wakeup.png" width="600"/></p>](./images/FRDM-A-S32K312_Reed_Wakeup.png)

#### Boards: FRDM-A-S32K312
#### Categories: Low Power
#### Peripherals: I2C, GPIO
#### Toolchains: S32 Design Studio IDE

## Table of Contents
1. [Software and Tools](#step1)
2. [Hardware](#step2)
3. [Setup](#step3)
4. [Results](#step4)
5. [Support](#step5)
6. [Release Notes](#step6)

## 1. Software and Tools<a name="step1"></a>
This example was developed using the FRDM Automotive Bundle for S32K3. To download and install the complete software and tools ecosystem, use the following link:<br>
- [S32K3 FRDM Automotive Board Installation Package](https://www.nxp.com/app-autopackagemgr/automotive-software-package-manager:AUTO-SW-PACKAGE-MANAGER?currentTab=0&selectedDevices=S32K3&applicationVersionID=156)

## 2. Hardware<a name="step2"></a>
### 2.1 Required Hardware
- Personal Computer
- Type-C USB cable

| Boards | Images |
| ----------- | ------- |
| - [FRDM-A-S32K312](https://www.nxp.com/design/design-center/development-boards-and-designs/FRDM-A-S32K312) | <img src="https://www.nxp.com/assets/images/en/dev-board-image/FRDM-A-S32K312-TOP.jpg" width="600"> |
| - [FRDM K64 click shield](https://www.mikroe.com/frdm-k64-click-shield) | <p align="center"><img src="images/frdm-k64-click.jpg" width="400"/> |
| - [Reed Click](https://www.mikroe.com/reed-click) <br> - [OLED B Click](https://www.mikroe.com/oled-b-click) | <p align="center"><img src="images/reed_click.png" width="200"/> <img src="images/oled-b-click.png" width="200"/> |

### 2.2 Hardware Connections

| FRDM-A-S32K312   | Header Pin |I/O| FRDM Shield  | Click Board  | Click Pin | Description             |
|------------------|------------|---|--------------|--------------|-----------|-------------------------|
| PTB17 WKPU18     | J10 pin 5  | ← | D10          | Reed Click   | CS        | WKPU input              |
| GND              | JA3 pin 13 | ↔ | GND          | Reed Click   | GND       | Ground                  |
| VDD_PERH         | JA3 pin 7  | → | 3.3V         | Reed Click   | 3V3       | Power Supply            |
| PTA3 GPIO        | JA1 pin 11 | ← | D5           | OLED B Click | D/C       | I2C Slave Address Pin   |
| PTA13 GPIO       | J4 pin 5   | ← | A2           | OLED B Click | RST       | Reset Pin               |
| PTC10 GPIO       | J2 pin 3   | ← | D9           | OLED B Click | CS        | Communication Enable Pin|
| PTA14 FlexIO_D14 | J4 pin 11  | → | A4           | OLED B Click | SDA       | FlexIO I2C SDA Pin      |
| PTA9 FlexIO_D7   | J4 pin 9   | → | A5           | OLED B Click | SCL       | FlexIO I2C SCL Pin      |
| GND              | JA3 pin 11 | → | GND          | OLED B Click | GND       | Ground                  |
| VDD_PERH         | JA3 pin 7  | → | 3.3V         | OLED B Click | 3V3       | Power Supply            |

### 2.3 Debugger Connection
- Connect the Type-C USB cable to PC and FRDM-A-S32K312 board for power supply and debugging.

## 3. Setup<a name="step3"></a>

### 3.1 Import the Project into S32 Design Studio IDE
1. Open S32 Design Studio IDE, in the Dashboard Panel, choose **Import project from Application Code Hub**.
   [<p align="center"><img src="images/import_project_1.png" width="400"/></p>](./images/import_project_1.png)

2. Find the demo by searching: [dm-reed-low-power-s32k312](https://mcuxpresso.nxp.com/appcodehub?search=dm-reed-low-power-s32k312)
3. Open the project, click the **GitHub link**, S32 Design Studio IDE will automatically retrieve project attributes, then click **Next>**.
    [<p align="center"><img src="images/import_project_3.png" width="600"/></p>](./images/import_project_3.png)

4. Select **main** branch and then click **Next>**.

5. Select your local path for the repo in **Destination->Directory:** window. The S32 Design Studio IDE will clone the repo into this path, click **Next>**.

6. Select **Import existing Eclipse projects** then click **Next>**.

7. Select the project in this repo (only one project in this repo) then click **Finish**.

### 3.2 Generating, Building and Running the Example
1. In Project Explorer, right-click the project and select **Update Code and Build Project**. This will generate the configuration (Pins, Clocks, Peripherals), update the source code and build the project using the active configuration (e.g. Debug_FLASH).
Make sure the build completes successfully and the *.elf file is generated without errors.
[<p align="center"><img src="images/update_and_build.png" width="200"/></p>](./images/update_and_build.png)
Press **Yes** in the **SDK Component Management** pop-up window to continue.

2. Go to **Debug** and select **Debug Configurations**. There will be a debug configuration for this project:
[<p align="center"><img src="images/Debug_config.png" width="200"/></p>](./images/Debug_config.png)

        Configuration Name                  Description
        -------------------------------     -----------------------
        $(example)_debug_flash_pemicro      Debug the FLASH configuration using PEmicro probe

   Select the desired debug configuration and click on **Debug**. Now the perspective will change to the **Debug Perspective**.
   Use the controls to control the program flow.

## 4. Results<a name="step4"></a>
The demo combines a reed switch connected to WKPU18 (PTB17) with an SSD1306 OLED display (via FlexIO I2C) and a green LED indicator to deliver a low-power standby wake-up demonstration:

[<p align="center"><img src="images/FRDM-A-S32K312_Reed_Wakeup_Result.gif"/></p>](./images/FRDM-A-S32K312_Reed_Wakeup_Result.gif)

- **Startup:** On cold start, the SSD1306 OLED display shows the current reed switch state.

- **Reed Switch Monitoring:** The main loop continuously polls the WKPU18 input state register to read the logic level on PTB17. When the state changes, the OLED display is updated:
  - `State: CLOSED` - when the magnet is near (door closed / secure).
  - `State: OPEN` - when the magnet is removed (door open / alarm condition).

- **Standby Entry:** Pressing SW2 updates the OLED to show `Standby: ARMED`, displays an "Entering Standby..." screen, turns off the green LED, arms WKPU18 as the wake-up source, and puts the MCU into standby mode. The OLED is then blanked to save power.

- **Wake-up Detection:** When the reed switch is triggered (magnet removed / magnetic field no longer detected on PTB17 / WKPU18), the MCU wakes up:
  - On **FAST** wake-up, the MCU resets and restarts from the top of `main()`, the OLED shows a "Woke up! / WKPU18 event" screen, and the LED turns back on as an alarm indicator.
  - On **NORMAL** wake-up, execution resumes after the standby call, the OLED is re-initialized, shows the wake-up screen, and the LED is turned back on.

## 5. Support<a name="step5"></a>
For general technical questions related to NXP microcontrollers, please use the *NXP Community Forum*.

#### Project Metadata

<!----- Boards ----->
[![Board badge](https://img.shields.io/badge/Board-FRDM&ndash;A&ndash;S32K312-blue)](https://mcuxpresso.nxp.com/appcodehub?hwBoard=FRDM-A-S32K312)

<!----- Categories ----->
[![Category badge](https://img.shields.io/badge/Category-LOW%20POWER-yellowgreen)](https://mcuxpresso.nxp.com/appcodehub?category=low_power)

<!----- Peripherals ----->
[![Peripheral badge](https://img.shields.io/badge/Peripheral-I2C-yellow)](https://mcuxpresso.nxp.com/appcodehub?peripheral=i2c)
[![Peripheral badge](https://img.shields.io/badge/Peripheral-GPIO-yellow)](https://mcuxpresso.nxp.com/appcodehub?peripheral=gpio)

<!----- Toolchains ----->
[![Toolchain badge](https://img.shields.io/badge/Toolchain-S32%20DESIGN%20STUDIO%20IDE-orange)](https://mcuxpresso.nxp.com/appcodehub?toolchain=s32_design_studio_ide)

Questions regarding the content/correctness of this example can be entered as Issues within this GitHub repository.

>**Note**: For more general technical questions regarding NXP Microcontrollers and the difference in expected functionality, enter your questions on the [NXP Community Forum](https://community.nxp.com/)

[![Follow us on Youtube](https://img.shields.io/badge/Youtube-Follow%20us%20on%20Youtube-red.svg)](https://www.youtube.com/NXP_Semiconductors)
[![Follow us on LinkedIn](https://img.shields.io/badge/LinkedIn-Follow%20us%20on%20LinkedIn-blue.svg)](https://www.linkedin.com/company/nxp-semiconductors)
[![Follow us on Facebook](https://img.shields.io/badge/Facebook-Follow%20us%20on%20Facebook-blue.svg)](https://www.facebook.com/nxpsemi/)
[![Follow us on Twitter](https://img.shields.io/badge/X-Follow%20us%20on%20X-black.svg)](https://x.com/NXP)

## 6. Release Notes<a name="step6"></a>
| Version | Description / Update                           | Date                        |
|:-------:|------------------------------------------------|----------------------------:|
| 1.0     | Initial release on Application Code Hub        | August 6<sup>th</sup> 2026  |
