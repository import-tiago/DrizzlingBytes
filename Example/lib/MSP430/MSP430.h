#pragma once
#define DRIZZLING_BYTES_VERSION "2.0"

/*
 *	v1.0 - Initial release.
 *	v1.1 - Flush RX UART buffer (MSP430 received bytes) before each transmission to allow multiple sequential OTA updates.
 *  v2.0 - Improvements in memory usage
 */

#include <Arduino.h>
#include <HardwareSerial.h>
#include <FS.h>
#include <SPIFFS.h>

#define SERIAL_DEBUG 0

#if SERIAL_DEBUG
#define print_debug(x); Serial.print(x);
#define println_debug(x); Serial.println(x);
#define print_debug_hex(x, base); Serial.print(x, HEX);
#define printf_debug_1arg(x, y); Serial.printf(x, y);
#define printf_debug_2arg(x, y, z); Serial.printf(x, y, z);
#else
#define print_debug(x);
#define println_debug(x);
#define print_debug_hex(x, base);
#define printf_debug_1arg(x, y);
#define printf_debug_2arg(x, y, z);
#endif

#define MAX_MSP430_MEMORY_SECTIONS 	30

 /**
  * @brief Construct a new BSL object to control the Bootstrap Loader programming of the target MCU.
  *
  * @param RESET_PIN Any GPIO with output mode capability to be connected to the RESET pin of the target MSP430.
  * @param TEST_PIN Any GPIO with output mode capability to be connected to the TEST pin of the target MSP430.
  * @param Serial_Port HardwareSerial object (TX and RX UART) from Arduino Framework (Serial, Serial1, Serial2...).
  * @param SPIFFS_Firmware_Address The local SPI Flash File System address where the BSL object must look for the firmware file in TI-TXT format.
  */
class MSP {

private:
	byte TEST_PIN;
	byte RESET_PIN;
	HardwareSerial BSL_UART;
	String SPIFFS_Firmware_Address;

	uint16_t calc_checksum(char* data, uint16_t length);
	bool send_large_data(uint32_t startAddress, uint32_t length, unsigned char* data);
	bool write_memory(uint32_t startAddress, char lenght, unsigned char* data);
	char ctoh(char data);
	void flush_bsl_uart_buffer();

public:
	MSP(byte RESET_PIN, byte TEST_PIN, HardwareSerial& Serial_Port, String SPIFFS_Firmware_Address) : BSL_UART(Serial_Port) {

		this->TEST_PIN = TEST_PIN;
		this->RESET_PIN = RESET_PIN;
		this->SPIFFS_Firmware_Address = SPIFFS_Firmware_Address;

		BSL_UART.begin(9600, SERIAL_8E1);
		BSL_UART.setTimeout(500);

		pinMode(TEST_PIN, OUTPUT);
		pinMode(RESET_PIN, OUTPUT);

		digitalWrite(TEST_PIN, LOW);
		digitalWrite(RESET_PIN, HIGH);
	}
	~MSP() {}

	void invoke_target_normal_mode_operation();
	void invoke_target_bsl_mode_operation();
	bool write_default_password();
	bool write_firmware();
	void load_firmware_from_spiffs();
};
