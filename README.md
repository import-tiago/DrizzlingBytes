DrizzlingBytes is a way to provide over-the-air (OTA) device firmware update (DFU) for MSP430-based systems.

In this implementation, an ESP32 (SoC) acts as a Hosting Device, managing and downloading new firmware available from Firebase Storage and then programming the target MSP430 (MCU).

<p align="center"><img src="https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Assets/Overview.png" ></p>

Access to the target's embedded memory is via the Bootstrap Loader (BSL), sometimes just called a "bootloader", available natively on MSP430 microcontrollers.

The MSP430 bootstrap loader does not start automatically; a special sequence is required on the RST/NMI and TEST pins, and then, a serial communication to the target becomes available through BSL. The entire process for using this feature can be found in Texas Instruments [SLAA096B](https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Hardware/1.%20Datasheets/MSP430%20BSL/Application%20of%20Bootstrap%20Loader%20in%20MSP430%20With%20Flash%20Hardware%20and%20Software%20Proposal%20(SLAA096B).pdf) documentation, used as a reference in this implementation.

Briefly, the process is as follows:

Put the MSP430 in Bootstrap Loader mode by the following invocation process:

<p align="center"><img src="https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Assets/BSLModeInvocation.png" ></p>

From this point, the target is ready to communicate with the host. There are two ways to do this: from I2C or UART protocols. In this case, UART is the choice. The necessary settings for the peripheral are:

<p align="center"><img src="https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Assets/BSLUARTSettings.png" ></p>

And then writing the BSL commands as needed. The possibilities are:

<p align="center"><img src="https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Assets/BSLCommandsList.png" ></p>

All commands must be transmitted according to the BSL protocol shown below:

<p align="center"><img src="https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Assets/BSLProtocol.png" ></p>

Then, after performing the desired memory accesses, set the target to normal mode operation again, exiting BSL mode:

<p align="center"><img src="https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Assets/NormalModeInvocation.png" ></p>

All tests and validations were performed with the following circuit:

<p align="center"><a href="https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Hardware/0.%20Project/DrizzlingBytes/DrizzlingBytes.pdf"><img src="https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Assets/SchematicPreview.png"  title="Schematic Preview" alt="PDF Download"></a></p>

As shown in the schematic above, the target bMSP430 used for testing is an [MSP430FR4133](https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Hardware/1.%20Datasheets/MSP430%20BSL/Datasheet%20-%20MSP430FR413x%20Mixed-Signal%20Microcontrollers%20(Rev.%20F).pdf). In order to have a clear visual representation of whether or not the target firmware has been updated, two firmware versions have been pre-compiled: [Slow_Blinky](https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Firmware/BSL_FR4133/Precompiled%20Firmware/Slow_Blinky.txt) and [Fast_Blinky](https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Firmware/BSL_FR4133/Precompiled%20Firmware/Fast_Blinky.txt).

The source code used to produce these files can be viewed below and the entire project structure is [available here](https://github.com/TiagoPaulaSilva/DrizzlingBytes/tree/main/Firmware/BSL_FR4133), [Code Composer Studio](https://www.ti.com/tool/CCSTUDIO) was used has an IDE.

```c
#include <msp430.h>

#define FAST 100000
#define SLOW 1000000
#define LED BIT7

void main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    P2OUT &= ~LED;
    P2DIR |= LED;

    PM5CTL0 &= ~LOCKLPM5;

    while (1) {
        P2OUT ^= LED;
        __delay_cycles(SLOW);
    }
}
```

The version that makes the LED blink slowly was flashed to the MCU and the version that makes it blink faster was uploaded in Firebase Storage. That way, if the ESP32 is able to successfully download and update the target, it will be easily noticeable by the blinky frequency of the LED.

<p align="center"><img src="https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Assets/FirebaseStorage.png" ></p>

Once the circuit is powered up, the ESP32 downloads the firmware from Firebase and starts the MSP430 programming process. The result of the BSL invocation as well as the periods of each pulse can be seen below:

<p align="center"><img src="https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Assets/BSLInvocationSignals.png" ></p>

If all goes well, the communication between HOST and TARGET can be seen through the UART:

<p align="center"><img src="https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Assets/UARTCommunicationData.png" ></p>

The assembled circuit used for the tests:

<p align="center"><img src="https://github.com/TiagoPaulaSilva/DrizzlingBytes/blob/main/Assets/UARTCommunicationData.png" ></p>
