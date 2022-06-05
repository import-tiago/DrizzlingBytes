#include <Arduino.h>
#include "MSP430.h"
#include "Network.h"
#include "WiFi_Secrets.h"
#include "Firebase_Secrets.h"
#include "Cloud.h"
#include <SPIFFS.h>

File Read_File;

#define UART_BUFFER_SIZE 300

char sendBuffer[UART_BUFFER_SIZE] = { 0 };
char receiveBuffer[UART_BUFFER_SIZE] = { 0 };
unsigned long fram_address[MAX_MSP430_MEMORY_SECTIONS];
unsigned long fram_length_of_sections[MAX_MSP430_MEMORY_SECTIONS];
unsigned long fram_sections;
unsigned long section[MAX_MSP430_MEMORY_SECTIONS][3];
unsigned char firmware[FIRMWARE_SIZE];

//***************************************
// BSL COMMANDS
//***************************************

#define RX_DATA_BLOCK_RESP_CMD	0x3B
#define RX_DATA_BLOCK_RESP_NL	0x02
#define RX_DATA_BLOCK_RESP_NH	0x00

#define PASSWORD_LENGTH 	 	32
#define RX_PASSWORD_NL			0x21
#define RX_PASSWORD_NH			0x00
#define RX_PASSWORD_RESP_CMD	0x3B
#define RX_PASSWORD_RESP_NL		0x02
#define RX_PASSWORD_RESP_NH		0x00

#define MASS_ERASE_NL		0x01
#define MASS_ERASE_NH		0x00
#define MASS_ERASE_RESP_CMD	0x3B
#define MASS_ERASE_RESP_NL	0x02
#define MASS_ERASE_RESP_NH	0x00

#define LOAD_PC_NL			0x04
#define LOAD_PC_NH			0x00

#define TX_DATA_BLOCK_NL	0x06
#define TX_DATA_BLOCK_NH	0x00
#define TX_DATA_RESP_CMD	0x3A

#define RX_PASSWORD 		0x11
#define TX_DATA_BLOCK		0x18
#define RX_DATA_BLOCK		0x10
#define MASS_ERASE			0x15
#define LOAD_PC				0x17

#define HEADER				0x80

#define ACK 				0x00
#define HEADER_INCORRECT	0x51
#define CHECKSUM_INCORRECT	0x52
#define PACKET_SIZE_ZERO	0x53
#define PACKET_SIZE_EXCEEDS	0x54
#define UNKNOWN_ERROR		0x55
#define UNKNOWN_BAUDRATE	0x56
#define PACKET_SIZE_ERROR	0x57

#define GetCKL(cs)			(char)(cs)
#define GetCKH(cs)			(char)(cs >> 8)

#define MAX_UART_BSL_BUFFER_SIZE 200

void MSP430::Invoke_MSP_Normal_Mode_Operation() {

	digitalWrite(TEST_PIN, LOW);
	digitalWrite(RESET_PIN, HIGH);
	delay(200);

	digitalWrite(RESET_PIN, LOW);
	delayMicroseconds(100);
	digitalWrite(RESET_PIN, HIGH);
	delayMicroseconds(100);
}

void MSP430::Invoke_MSP_BSL_Mode_Operation() {

	Serial.println("Invoking BSL by hardware entry sequence...");

	digitalWrite(RESET_PIN, HIGH);
	digitalWrite(TEST_PIN, HIGH);
	delay(200);

	digitalWrite(TEST_PIN, LOW);
	digitalWrite(RESET_PIN, LOW);
	delay(78);

	digitalWrite(TEST_PIN, HIGH);
	delay(30);
	digitalWrite(TEST_PIN, LOW);
	delay(30);

	digitalWrite(TEST_PIN, HIGH);
	delay(30);
	digitalWrite(RESET_PIN, HIGH);
	delay(30);
	digitalWrite(TEST_PIN, LOW);

	delay(200);
}

bool MSP430::Write_Default_Password() {

	bool result = false;
	unsigned int checksum = 0;
	char password[PASSWORD_LENGTH] = { 0 };

	memset(sendBuffer, '\0', UART_BUFFER_SIZE);
	memset(receiveBuffer, '\0', UART_BUFFER_SIZE);

	for (unsigned int i = 0; i < PASSWORD_LENGTH; i++)
		password[i] = 0xFF;

	sendBuffer[0] = HEADER;
	sendBuffer[1] = RX_PASSWORD_NL;
	sendBuffer[2] = RX_PASSWORD_NH;
	sendBuffer[3] = RX_PASSWORD;

	memcpy(&sendBuffer[4], password, PASSWORD_LENGTH);

	checksum = Checksum(&sendBuffer[3], (unsigned int)(PASSWORD_LENGTH + 1));
	sendBuffer[PASSWORD_LENGTH + 4] = GetCKL(checksum);
	sendBuffer[PASSWORD_LENGTH + 5] = GetCKH(checksum);

	Serial.println("Sending BSL password...");

	BSL_UART.write(sendBuffer, PASSWORD_LENGTH + 6);
	BSL_UART.readBytes(receiveBuffer, 8);

	Serial.println("BSL password unlock response: ");
	Serial.print(receiveBuffer[0], HEX);
	Serial.print(" ");
	Serial.print(receiveBuffer[1], HEX);
	Serial.print(" ");
	Serial.print(receiveBuffer[2], HEX);
	Serial.print(" ");
	Serial.print(receiveBuffer[3], HEX);
	Serial.print(" ");
	Serial.print(receiveBuffer[4], HEX);
	Serial.print(" ");
	Serial.print(receiveBuffer[5], HEX);
	Serial.println(" ");

	if ((receiveBuffer[0] == ACK)
		&& (receiveBuffer[1] == HEADER)
		&& (receiveBuffer[2] == RX_PASSWORD_RESP_NL)
		&& (receiveBuffer[3] == RX_PASSWORD_RESP_NH)
		&& (receiveBuffer[4] == RX_PASSWORD_RESP_CMD)
		&& (receiveBuffer[5] == 0x00)) {

		Serial.println("BSL password is correct!");
		result = true;
	}
	else {
		Serial.println("BSL password is wrong!");
		result = false;
	}

	return result;
}

unsigned int MSP430::Checksum(char* data, unsigned int length) {

	char x;
	unsigned int crc = 0xFFFF;

	while (length--) {
		x = crc >> 8 ^ *data++;
		x ^= x >> 4;
		crc = (crc << 8) ^ ((unsigned int)(x << 12)) ^ ((unsigned int)(x << 5)) ^ ((unsigned int)x);
	}

	return crc;
}

bool MSP430::Write_Firmware() {

	bool numberOfErrors = 0;
	bool result = true;
	unsigned int region = 0;
	unsigned char* framStartOfData = 0;

	for (region = 0; region < fram_sections; region++) {

		framStartOfData = &firmware[section[region][0]];

		result = Send_Large_Data(fram_address[region], fram_length_of_sections[region], framStartOfData);

		if (!result) {
			Serial.print("Write_Memory Large Data To Memory failed\r\n");
			return false;
		}
		else
			Serial.print("Write_Memory Large Data To Memory successful\r\n");
	}

	if (numberOfErrors != 0)
		return false;

	return true;
}

bool MSP430::Send_Large_Data(unsigned long startAddress, unsigned long length, unsigned char* data) {

	unsigned long currentAddress = startAddress;
	unsigned long currentLength = length;
	unsigned char* currentData = data;
	bool done = false;
	bool result = true;

	while (!done) {
		if (currentLength <= 0) {
			done = true;
		}
		else if (currentLength < MAX_UART_BSL_BUFFER_SIZE) {

			result = Write_Memory(currentAddress, currentLength, currentData);
			if (!result) {
				return result;
			}
			done = true;
		}
		else {
			result = Write_Memory(currentAddress, MAX_UART_BSL_BUFFER_SIZE, currentData);
			if (!result) {
				return result;
			}
			currentAddress += MAX_UART_BSL_BUFFER_SIZE;
			currentData += MAX_UART_BSL_BUFFER_SIZE;
			currentLength -= MAX_UART_BSL_BUFFER_SIZE;
		}
	}

	return true;
}

bool MSP430::Write_Memory(unsigned long startAddress, char lenght, unsigned char* data) {

	uint16_t checksum = 0;

	memset(sendBuffer, '\0', UART_BUFFER_SIZE);
	memset(receiveBuffer, '\0', UART_BUFFER_SIZE);

	sendBuffer[0] = HEADER;
	sendBuffer[1] = (char)((lenght + 4) & 0x00ff);
	sendBuffer[2] = (char)(((lenght + 4) >> 8) & 0x00ff);
	sendBuffer[3] = RX_DATA_BLOCK;

	sendBuffer[4] = (char)(startAddress & 0x00ff);
	sendBuffer[5] = (char)((startAddress >> 8) & 0x00ff);
	sendBuffer[6] = (char)((startAddress >> 16) & 0x00ff);

	memcpy(&sendBuffer[7], data, lenght);

	checksum = Checksum(&sendBuffer[3], lenght + 4);
	sendBuffer[lenght + 7] = (char)(checksum);
	sendBuffer[lenght + 8] = (char)(checksum >> 8);

	Serial.println("Writing in memory...");

	BSL_UART.write(sendBuffer, lenght + 9);
	BSL_UART.readBytes(receiveBuffer, 8);

	Serial.println("Memory write response:");
	Serial.print(receiveBuffer[0], HEX);
	Serial.print(" ");
	Serial.print(receiveBuffer[1], HEX);
	Serial.print(" ");
	Serial.print(receiveBuffer[2], HEX);
	Serial.print(" ");
	Serial.print(receiveBuffer[3], HEX);
	Serial.print(" ");
	Serial.print(receiveBuffer[4], HEX);
	Serial.print(" ");
	Serial.print(receiveBuffer[5], HEX);
	Serial.println(" ");

	if ((receiveBuffer[0] == ACK)
		&& (receiveBuffer[1] == HEADER)
		&& (receiveBuffer[2] == RX_DATA_BLOCK_RESP_NL)
		&& (receiveBuffer[3] == RX_DATA_BLOCK_RESP_NH)
		&& (receiveBuffer[4] == RX_DATA_BLOCK_RESP_CMD)
		&& (receiveBuffer[5] == 0x00)) {
		return true;
	}

	return false;
}

char  MSP430::ctoh(char data) {
	if (data > '9') {
		data += 9;
	}
	return (data &= 0x0F);
}

void MSP430::Load_from_SPIFFS_and_Store_in_RAM() {
	Read_File = SPIFFS.open(SPIFFS_Firmware_Address, "r");

#define MAX_MEMORY_LENGTH 4
	unsigned int sections_count = 0;
	unsigned long memory_address[MAX_MSP430_MEMORY_SECTIONS];
	char address[MAX_MEMORY_LENGTH + 1] = { 0 };
	unsigned long i = 0;
	unsigned char _byte = 0;

	while (Read_File.available()) {

		_byte = (char)Read_File.read();

		if (_byte != ' ' && _byte != '\r' && _byte != '\n') {

			if (_byte == '@') {

				address[0] = (char)Read_File.read();
				address[1] = (char)Read_File.read();
				address[2] = (char)Read_File.read();
				address[3] = (char)Read_File.read();
				address[4] = '\0';

				memory_address[sections_count] = strtol(address, NULL, 16);

				section[sections_count][0] = i; //INIT INDEX of section

				if (sections_count > 0) {
					section[sections_count - 1][1] = section[sections_count][0] - 1; //END INDEX of section
					section[sections_count - 1][2] = section[sections_count - 1][1] - section[sections_count - 1][0] + 1; //LENGTH (in bytes) of section
				}
				sections_count++;
			}
			else {
				char msb = ctoh(_byte);
				char lsb = ctoh((char)Read_File.read());
				firmware[i++] = (unsigned char)((msb << 4) | lsb);
			}
		}
	}

	Read_File.close();

	for (int z = 0; z < 150; z++) {
		Serial.println(firmware[z], HEX);
	}
	Serial.println("----------------");

	Serial.printf("exit: %d\r\n", sections_count);
	Serial.println(i);
	section[sections_count - 1][1] = i - 2; //END INDEX of section
	section[sections_count - 1][2] = section[sections_count - 1][1] - section[sections_count - 1][0] + 1; //LENGTH (in bytes) of section

	for (int _i = 0; _i < sections_count; _i++) {
		Serial.printf("Section %d: %#lX\r\n", _i, memory_address[_i]);
		fram_address[_i] = memory_address[_i];
	}

	Serial.printf("Total memory section: %d\r\n", sections_count);
	fram_sections = sections_count;

	Serial.print("\r\nINIT: ");
	Serial.println(section[0][0]);
	Serial.print("END: ");
	Serial.println(section[0][1]);
	Serial.print("LENGTH: ");
	Serial.println(section[0][2]);

	Serial.print("INIT: ");
	Serial.println(section[1][0]);
	Serial.print("END: ");
	Serial.println(section[1][1]);
	Serial.print("LENGTH: ");
	Serial.println(section[1][2]);

	Serial.print("INIT: ");
	Serial.println(section[2][0]);
	Serial.print("END: ");
	Serial.println(section[2][1]);
	Serial.print("LENGTH: ");
	Serial.println(section[2][2]);

	for (int _i = 0; _i < sections_count; _i++)
		fram_length_of_sections[_i] = section[_i][2];
}