#ifndef MSP430_BSL_H
#define MSP430_BSL_H

#include "Arduino.h"
#include <HardwareSerial.h>
#include <FS.h>
#include <SPIFFS.h>

extern fs::File file;

#define MAX_MSP430_MEMORY_SECTIONS 30
#define MAX_PROGRAM_FRAM_KB 	64			//MSP430FR2476 as reference
#define MAX_PROGRAM_FRAM_BYTES (1024 * 64)
#define MAX_INFORMATION_FRAM_BYTES 512
#define FRAM_SIZE MAX_PROGRAM_FRAM_BYTES + MAX_INFORMATION_FRAM_BYTES

extern unsigned long fram_address[MAX_MSP430_MEMORY_SECTIONS];
extern unsigned long fram_length_of_sections[MAX_MSP430_MEMORY_SECTIONS];
extern unsigned long fram_sections;




extern unsigned char firmware[FRAM_SIZE];
extern unsigned long section[MAX_MSP430_MEMORY_SECTIONS][3];
/*
section[n][0] --> INIT INDEX of section n
section[n][1] --> END INDEX of section n
section[n][2] --> LENGTH (in bytes) of section n
*/

/**
 * @brief Construct a new BSL object to control the Bootstrap Loader programming of the target MCU
 *
 * @param RESET_PIN Any GPIO with output mode capability to be connected to the RESET pin of the target MSP430
 * @param TEST_PIN Any GPIO with output mode capability to be connected to the TEST pin of the target MSP430
 * @param Serial_Port HardwareSerial object (TX and RX UART) from Arduino Framework (Serial, Serial1, Serial2...)
 */
class MSP430_BSL {

	private:
	byte TEST_PIN;
	byte RESET_PIN;
	HardwareSerial BSL_UART;

	unsigned int Checksum(char* data, unsigned int length);
	bool Send_Large_Data(unsigned long startAddress, unsigned long length, unsigned char* data);
	bool Write_Memory(unsigned long startAddress, char lenght, unsigned char* data);

	public:
	MSP430_BSL(byte RESET_PIN, byte TEST_PIN, HardwareSerial& Serial_Port) : BSL_UART(Serial_Port) {

		this->TEST_PIN = TEST_PIN;
		this->RESET_PIN = RESET_PIN;

		BSL_UART.begin(9600, SERIAL_8E1);
		BSL_UART.setTimeout(500);

		pinMode(TEST_PIN, OUTPUT);
		pinMode(RESET_PIN, OUTPUT);

		digitalWrite(TEST_PIN, LOW);
		digitalWrite(RESET_PIN, HIGH);
	}
	void Invoke_MSP_Normal_Mode_Operation();
	void Invoke_MSP_BSL_Mode_Operation();
	bool Write_Default_Password();
	bool Write_Firmware();
};

class MSP430_OTA {

	private:
	char ctoh(char data);

	public:
	MSP430_OTA() {
		memset(firmware, '\0', FRAM_SIZE);
	}
	void Download_Firmware();
};

#endif //MSP430_BSL_H
