# DrizzlingBytes

DrizzlingBytes is a way to provide over-the-air (OTA) device firmware update (DFU) for MSP430-based systems.

In this implementation, an ESP32 (SoC) acts as a Host Device, managing and downloading new firmware available from Firebase Storage and then programming the target MSP430 (MCU).

<p align="center"><img src="https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Assets/Overview.png" width="70%" height="70%"></p>

Access to the target's embedded memory is done through the bootstrap loader (BSL), sometimes just called the "bootloader", available natively on MSP430 microcontrollers.
