DrizzlingBytes is a way to provide over-the-air (OTA) device firmware update (DFU) for MSP430-based systems.

In this implementation, an ESP32 (SoC) acts as a Hosting Device, managing and downloading new firmware available from Firebase Storage and then programming the target MSP430 (MCU).

<p align="center"><img src="https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Assets/Overview.png" width="70%" height="70%"></p>

Access to the target's built-in memory is via the bootloader (BSL), sometimes just called a "bootloader", available natively on MSP430 microcontrollers.

The MSP430 bootstrap loader does not start automatically; a special sequence is required on the RST/NMI and TEST pins and later a serial communication under a certain communication protocol. The entire process for using BSL and its features can be found in Texas Instruments' [SLAA096B](https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Hardware/1.%20Datasheets/MSP430%20BSL/Application%20of%20Bootstrap%20Loader%20in%20MSP430%20With%20Flash%20Hardware%20and%20Software%20Proposal%20(SLAA096B).pdf) documentation, used as a reference in this implementation.

Briefly, the process is as follows:

Put the MSP430 in Bootstrap Loader mode by invoking BSL:

<p align="center"><img src="https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Assets/BSLModeInvocation.png" width="70%" height="70%"></p>

From this point, the target is ready to communicate with the host. There are two ways to do this: from I2C or UART protocols. In this case, UART is the choice. The necessary settings are:

<p align="center"><img src="https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Assets/BSLUARTSettings.png" width="25%" height="25%"></p>

And then writing the BSL commands as needed. The possibilities are:

<p align="center"><img src="https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Assets/BSLCommandsList.png" width="70%" height="70%"></p>

All commands must be transmitted according to the BSL protocol shown below:

<p align="center"><img src="https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Assets/BSLProtocol.png" width="80%" height="80%"></p>

And then, after performing the desired memory accesses, set the target to normal mode operation again, exiting BSL mode:


<p align="center"><img src="https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Assets/NormalModeInvocation.png" width="70%" height="70%"></p>
