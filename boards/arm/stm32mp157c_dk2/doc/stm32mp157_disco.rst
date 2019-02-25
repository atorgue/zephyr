.. _stm32mp157c_dk2_board:

ST STM32MP157C Discovery
########################

Overview
********

The STM32MP157 Discovery board leverages the capacities of the STM32MP157
multi-core processor,composed of a dual Cortex®-A7 and a single Cortex®-M4 core.
Zephyr OS is ported to run on the Cortex®-M4 core.

Common features:

- STM32MP157:
    - Arm®-based dual Cortex®-A7 32 bits 
    - Cortex®-M4 32 bits
    - embedded SRAM (448 Kbytes) for Cortex®-M4.
- ST PMIC STPMIC1A
- 4-Gbit DDR3L, 16 bits, 533 MHz
- 1-Gbps Ethernet (RGMII) compliant with IEEE-802.3ab
- USB OTG HS
- Audio CODEC, with a stereo headset jack, including analog microphone input
- 4 user LEDs
- 2 user and reset push-buttons, 1 wake-up button
- 5 V / 3 A USB Type-CTM power supply input (not provided)
- Board connectors:
    - Ethernet RJ45
    - 4 USB Host Type-A
    - USB Type-C
    - DRP MIPI DSI HDMI
    - Stereo headset jack including analog microphone input
    - microSD card
    - GPIO expansion connector (Raspberry Pi® shields capability)
    - ArduinoTM Uno V3 expansion connectors
    - On-board ST-LINK/V2-1 debugger/programmer with USB re-enumeration capability: Virtual COM port and debug port
- Board-specific features
    - 4" TFT 480×800 pixels with LED backlight, MIPI DSI interface, and capacitive touch panel
    - Wi-Fi® 802.11b/g/n
    - Bluetooth® Low Energy 4.1
    
.. image:: img/en.stm32mp157c-dk2.jpg
     :width: 900px
     :align: center
     :height: 788px
     :alt: STM32MP157C-DK2 Discovery

More information about the board can be found at the `STM32P157C Discovery website`_.

Hardware
********

The STM32MP157 SoC provides the following hardware capabilities:

- Core

  - 32-bit dual-core Arm® Cortex®-A7
    - L1 32-Kbyte I / 32-Kbyte D for each core
    - 256-Kbyte unified level 2 cache
    - Arm® NEON™ and Arm® TrustZone®
  - 32-bit Arm® Cortex®-M4 with FPU/MPU
    - Up to 209 MHz (Up to 703 CoreMark®)

- Memories

  - External DDR memory up to 1 Gbyte
    - up to LPDDR2/LPDDR3-1066 16/32-bit
    - up to DDR3/DDR3L-1066 16/32-bit
  - 708 Kbytes of internal SRAM: 256 KB of AXI SYSRAM + 384 KB of AHB SRAM + 64 KB of AHB SRAM in backup domain and 4 KB of SRAM in backup domain
  - Dual mode Quad-SPI memory interface
  - Flexible external memory controller with up to 16-bit data bus: parallel interface to connect external ICs and SLC NAND memories with up to 8-bit ECC

- Security/safety

  - Secure boot, TrustZone® peripherals, active tamper
  - Cortex®-M4 resources isolation

- Reset and power management

  - 1.71 V to 3.6 V I/Os supply (5 V-tolerant I/Os)
  - POR, PDR, PVD and BOR
  - On-chip LDOs (RETRAM, BKPSRAM, DSI 1.2 V, USB 1.8 V, 1.1 V)
  - Backup regulator (~0.9 V)
  - Internal temperature sensors
  - Low-power modes: Sleep, Stop and Standby
  - LPDDR2/3 retention in Standby mode
  - Controls for PMIC companion chip

- Clock management

  - Internal oscillators: 64 MHz HSI oscillator, 4 MHz CSI oscillator, 32 kHz LSI oscillator
  - External oscillators: 8-48 MHz HSE oscillator, 32.768 kHz LSE oscillator
  - 6 × PLLs with fractional mode

- General-purpose input/outputs

  - Up to 176 I/O ports with interrupt capability
    - Up to 8 secure I/Os
    - Up to 6 Wakeup, 3 Tamper, 1 Active-Tamper

- Interconnect matrix

- 3 DMA controllers to unload the CPU

- Communication peripherals

  - 6 × I2C FM+ (1 Mbit/s, SMBus/PMBus)
  - 4 × UART + 4 × USART (12.5 Mbit/s, ISO7816 interface, LIN, IrDA, SPI slave)
  - 6 × SPI (50 Mbit/s, including 3 with full duplex I2S audio class accuracy via internal audio PLL or external clock)
  - 4 × SAI (stereo audio: I2S, PDM, SPDIF Tx)
  - SPDIF Rx with 4 inputs
  - HDMI-CEC interface
  - MDIO Slave interface
  - 3 × SDMMC up to 8-bit (SD / e•MMC™ / SDIO)
  - 2 × CAN controllers supporting CAN FD protocol, out of which one supports time-triggered CAN (TTCAN)
  - 2 × USB 2.0 high-speed Host+ 1 × USB 2.0 full-speed OTG simultaneously
    - or 1 × USB 2.0 high-speed Host+ 1 × USB 2.0 high-speed OTG simultaneously
  - 10/100M or Gigabit Ethernet GMAC
    - IEEE 1588v2 hardware, MII/RMII/GMII/RGMII
  - 8- to 14-bit camera interface up to 140 Mbyte/s
  - 6 analog peripherals
  - 2 × ADCs with 16-bit max. resolution (12 bits 5 Msps, 14 bits 4.4 Msps, 16 bits 250 ksps)
  - 1 × temperature sensor
  - 2 × 12-bit D/A converters (1 MHz)
  - 1 × digital filters for sigma delta modulator (DFSDM) with 8 channels/6 filters
  - Internal or external ADC/DAC reference VREF+
  
- Graphics

  - 3D GPU: Vivante® - OpenGL® ES 2.0
    - Up to 26 Mtriangle/s, 133 Mpixel/s

  - LCD-TFT controller, up to 24-bit // RGB888
    - up to WXGA (1366 × 768) @60 fps
    - Two layers with programmable colour LUT
  - MIPI® DSI 2 data lanes up to 1 GHz each
  
- Timers

  - 2 × 32-bit timers with up to 4 IC/OC/PWM or pulse counter and quadrature (incremental) encoder input
  - 2 × 16-bit advanced motor control timers
  - 10 × 16-bit general-purpose timers (including 2 basic timers without PWM)
  - 5 × 16-bit low-power timers
  - RTC with sub-second accuracy and hardware calendar
  - 2 × 4 Cortex®-A7 system timers (secure, non-secure, virtual, hypervisor)
  - 1 × SysTick Cortex®-M4 timer
 
- Hardware acceleration

  - AES 128, 192, 256, TDES
  - HASH (MD5, SHA-1, SHA224, SHA256), HMAC
  - 2 × true random number generator (3 oscillators each)
  - 2 × CRC calculation unit

- Debug mode

  - Arm® CoreSight™ trace and debug: SWD and JTAG interfaces
  - 8-Kbyte embedded trace buffer
  - 3072-bit fuses including 96-bit unique ID, up to 1184-bit available for user

More information about STM32P157C can be found here:
       - `STM32MP157C on www.st.com`_
       - `STM32MP157C reference manual`_

Supported Features
==================

The Zephyr stm32mp157c_dk2 board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+

Other hardware features are not yet supported on this Zephyr port.

The default configuration can be found in the defconfig file:

	``boards/arm/stm32mp157c_dk2/stm32mp157c_dk2_defconfig``


Connections and IOs
===================

STM32MP157C Discovery Board schematic is available here: `STM32MP157C Discovery board schematics`_.


System Clock
------------

The Cortex®-M4 Core is configured to run at a 209 MHz clock speed.

Serial Port
-----------

The STM32MP157C Discovery board has 8 U(S)ARTs. 
The Zephyr console output is assigned by default to the ram console to be dumped
by the Linux Remoteproc Framework on Cortex®-A7 core. The UART 4 can enabled as
Cortex®-M4 console.

Programming and Debugging
*************************
The STM32MP157C doesn't have QSPI flash for the Cortex®-M4  and it needs to be 
started by the Cortex®-A7 core. The Cortex®-A7 core is responsible to load the 
Cortex®-M4 binary application into the RAM, get the Cortex®-M4 out of reset.
The Cortex®-A7 can perform these steps at bootloader level or after the Linux 
system has booted.

The Cortex®-M4 can use up to 2 different RAMs. The program pointer starts at 
address 0x00000000 (RETRAM), the vector table should be loaded at this address 
These are the memory mapping for Cortex®-A7 and Cortex®-M4:

+------------+-----------------------+------------------------+----------------------+
| Region     | Cortex®-A7            | Cortex®-M4             | Size                 |
+============+=======================+========================+======================+
| RETRAM     | 0x38000000-0x3800FFFF | 0x00000000-0x0000FFFF  | 64KB                 |
+------------+-----------------------+------------------------+----------------------+
| MCUSRAM    | 0x10000000-0x1005FFFF | 0x10000000-0x1005FFFF  | 384KB                |
+------------+-----------------------+------------------------+----------------------+
| DDR        | 0xC0000000-0xFFFFFFFF |                        | up to 1 GB           |
+------------+-----------------------+------------------------+----------------------+

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32mp157_dk2
   :maybe-skip-config:
   :goals: debug

.. _STM32P157C Discovery website:
   https://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-mpu-eval-tools/stm32-mcu-mpu-eval-tools/stm32-discovery-kits/stm32mp157c-dk2.html

.. _STM32MP157C Discovery board User Manual:
   https://www.st.com/resource/en/user_manual/dm00591354.pdf

.. _STM32MP157C Discovery board schematics:
   https://www.st.com/resource/en/schematic_pack/mb1272-dk2-c01_schematic.pdf

.. _STM32MP157C on www.st.com:
   https://www.st.com/content/st_com/en/products/microcontrollers-microprocessors/stm32-arm-cortex-mpus/stm32mp1-series/stm32mp157/stm32mp157c.html

.. _STM32MP157C reference manual:
   https://www.st.com/resource/en/reference_manual/DM00327659.pdf

