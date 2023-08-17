#include "MSP430.h"

#define UART_BUFFER_SIZE 300

char uart_tx_buf[UART_BUFFER_SIZE] = { 0 };
char uart_rx_buf[UART_BUFFER_SIZE] = { 0 };
uint32_t fram_address[MAX_MSP430_MEMORY_SECTIONS];
uint32_t fram_length_of_sections[MAX_MSP430_MEMORY_SECTIONS];
uint32_t fram_sections;
uint32_t section[MAX_MSP430_MEMORY_SECTIONS][3];
unsigned char* firmware;

//***************************************
// BSL COMMANDS
//***************************************

#define RX_DATA_BLOCK_RESP_CMD    0x3B
#define RX_DATA_BLOCK_RESP_NL     0x02
#define RX_DATA_BLOCK_RESP_NH     0x00

#define PASSWORD_LENGTH           32
#define RX_PASSWORD_NL            0x21
#define RX_PASSWORD_NH            0x00
#define RX_PASSWORD_RESP_CMD      0x3B
#define RX_PASSWORD_RESP_NL       0x02
#define RX_PASSWORD_RESP_NH       0x00

#define MASS_ERASE_NL             0x01
#define MASS_ERASE_NH             0x00
#define MASS_ERASE_RESP_CMD       0x3B
#define MASS_ERASE_RESP_NL        0x02
#define MASS_ERASE_RESP_NH        0x00

#define LOAD_PC_NL                0x04
#define LOAD_PC_NH                0x00

#define TX_DATA_BLOCK_NL          0x06
#define TX_DATA_BLOCK_NH          0x00
#define TX_DATA_RESP_CMD          0x3A

#define RX_PASSWORD               0x11
#define TX_DATA_BLOCK             0x18
#define RX_DATA_BLOCK             0x10
#define MASS_ERASE                0x15
#define LOAD_PC                   0x17

#define HEADER                    0x80

#define ACK                       0x00
#define HEADER_INCORRECT          0x51
#define CHECKSUM_INCORRECT        0x52
#define PACKET_SIZE_ZERO          0x53
#define PACKET_SIZE_EXCEEDS       0x54
#define UNKNOWN_ERROR             0x55
#define UNKNOWN_BAUDRATE          0x56
#define PACKET_SIZE_ERROR         0x57


#define GetCKL(cs)			(char)(cs)
#define GetCKH(cs)			(char)(cs >> 8)

#define MAX_UART_BSL_BUFFER_SIZE 200

void MSP::invoke_target_normal_mode_operation() {

	free(firmware);

	digitalWrite(TEST_PIN, LOW);
	digitalWrite(RESET_PIN, HIGH);
	delay(200);

	digitalWrite(RESET_PIN, LOW);
	delayMicroseconds(100);
	digitalWrite(RESET_PIN, HIGH);
	delayMicroseconds(100);
}

void MSP::invoke_target_bsl_mode_operation() {

	println_debug("Invoking BSL by hardware entry sequence...");

	digitalWrite(RESET_PIN, HIGH);
	digitalWrite(TEST_PIN, HIGH);
	delay(200);

	digitalWrite(TEST_PIN, LOW);
	digitalWrite(RESET_PIN, LOW);
	delay(100);

	digitalWrite(TEST_PIN, HIGH);
	delay(50);
	digitalWrite(TEST_PIN, LOW);
	delay(50);

	digitalWrite(TEST_PIN, HIGH);
	delay(50);
	digitalWrite(RESET_PIN, HIGH);
	delay(50);
	digitalWrite(TEST_PIN, LOW);

	delay(200);
}

bool MSP::write_default_password() {

	bool result = false;
	uint16_t checksum = 0;
	char password[PASSWORD_LENGTH] = { 0 };

	memset(uart_tx_buf, '\0', UART_BUFFER_SIZE);
	memset(uart_rx_buf, '\0', UART_BUFFER_SIZE);

	for (uint16_t i = 0; i < PASSWORD_LENGTH; i++)
		password[i] = 0xFF;

	uart_tx_buf[0] = HEADER;
	uart_tx_buf[1] = RX_PASSWORD_NL;
	uart_tx_buf[2] = RX_PASSWORD_NH;
	uart_tx_buf[3] = RX_PASSWORD;

	memcpy(&uart_tx_buf[4], password, PASSWORD_LENGTH);

	checksum = calc_checksum(&uart_tx_buf[3], (uint16_t)(PASSWORD_LENGTH + 1));
	uart_tx_buf[PASSWORD_LENGTH + 4] = GetCKL(checksum);
	uart_tx_buf[PASSWORD_LENGTH + 5] = GetCKH(checksum);

	println_debug("Sending BSL password...");

	flush_bsl_uart_buffer();

	BSL_UART.write(uart_tx_buf, PASSWORD_LENGTH + 6);

	while (BSL_UART.available() <= 0) {
		;
	}

	BSL_UART.readBytes(uart_rx_buf, 8);

	println_debug("BSL password unlock response: ");
	print_debug_hex(uart_rx_buf[0], HEX);
	print_debug(" ");
	print_debug_hex(uart_rx_buf[1], HEX);
	print_debug(" ");
	print_debug_hex(uart_rx_buf[2], HEX);
	print_debug(" ");
	print_debug_hex(uart_rx_buf[3], HEX);
	print_debug(" ");
	print_debug_hex(uart_rx_buf[4], HEX);
	print_debug(" ");
	print_debug_hex(uart_rx_buf[5], HEX);
	println_debug(" ");

	if ((uart_rx_buf[0] == ACK)
		&& (uart_rx_buf[1] == HEADER)
		&& (uart_rx_buf[2] == RX_PASSWORD_RESP_NL)
		&& (uart_rx_buf[3] == RX_PASSWORD_RESP_NH)
		&& (uart_rx_buf[4] == RX_PASSWORD_RESP_CMD)
		&& (uart_rx_buf[5] == 0x00)) {

		println_debug("BSL password is correct!");
		result = true;
	}
	else {
		println_debug("BSL password is wrong!");
		result = false;
	}

	delay(50);

	return result;
}

uint16_t MSP::calc_checksum(char* data, uint16_t length) {

	char x;
	uint16_t crc = 0xFFFF;

	while (length--) {
		x = crc >> 8 ^ *data++;
		x ^= x >> 4;
		crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ ((uint16_t)x);
	}

	return crc;
}

bool MSP::write_firmware() {

	bool numberOfErrors = 0;
	bool result = true;
	uint16_t region = 0;
	unsigned char* framStartOfData = 0;

	for (region = 0; region < fram_sections; region++) {

		framStartOfData = &firmware[section[region][0]];

		result = send_large_data(fram_address[region], fram_length_of_sections[region], framStartOfData);

		if (!result) {
			print_debug("write_memory Large Data To Memory failed\r\n");
			return false;
		}
		else
			print_debug("write_memory Large Data To Memory successful\r\n");
	}

	if (numberOfErrors != 0)
		return false;

	return true;
}

bool MSP::send_large_data(uint32_t startAddress, uint32_t length, unsigned char* data) {

	uint32_t currentAddress = startAddress;
	uint32_t currentLength = length;
	unsigned char* currentData = data;
	bool done = false;
	bool result = true;

	while (!done) {
		if (currentLength <= 0) {
			done = true;
		}
		else if (currentLength < MAX_UART_BSL_BUFFER_SIZE) {

			result = write_memory(currentAddress, currentLength, currentData);
			if (!result) {
				return result;
			}
			done = true;
		}
		else {
			result = write_memory(currentAddress, MAX_UART_BSL_BUFFER_SIZE, currentData);
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

bool MSP::write_memory(uint32_t startAddress, char lenght, unsigned char* data) {

	uint16_t checksum = 0;

	memset(uart_tx_buf, '\0', UART_BUFFER_SIZE);
	memset(uart_rx_buf, '\0', UART_BUFFER_SIZE);

	uart_tx_buf[0] = HEADER;
	uart_tx_buf[1] = (char)((lenght + 4) & 0x00ff);
	uart_tx_buf[2] = (char)(((lenght + 4) >> 8) & 0x00ff);
	uart_tx_buf[3] = RX_DATA_BLOCK;

	uart_tx_buf[4] = (char)(startAddress & 0x00ff);
	uart_tx_buf[5] = (char)((startAddress >> 8) & 0x00ff);
	uart_tx_buf[6] = (char)((startAddress >> 16) & 0x00ff);

	memcpy(&uart_tx_buf[7], data, lenght);

	checksum = calc_checksum(&uart_tx_buf[3], lenght + 4);
	uart_tx_buf[lenght + 7] = (char)(checksum);
	uart_tx_buf[lenght + 8] = (char)(checksum >> 8);

	println_debug("Writing in memory...");

	flush_bsl_uart_buffer();

	BSL_UART.write(uart_tx_buf, lenght + 9);

	while (BSL_UART.available() <= 0) {
		;
	}

	BSL_UART.readBytes(uart_rx_buf, 8);

	println_debug("Memory write response:");
	print_debug_hex(uart_rx_buf[0], HEX);
	print_debug(" ");
	print_debug_hex(uart_rx_buf[1], HEX);
	print_debug(" ");
	print_debug_hex(uart_rx_buf[2], HEX);
	print_debug(" ");
	print_debug_hex(uart_rx_buf[3], HEX);
	print_debug(" ");
	print_debug_hex(uart_rx_buf[4], HEX);
	print_debug(" ");
	print_debug_hex(uart_rx_buf[5], HEX);
	println_debug(" ");

	if ((uart_rx_buf[0] == ACK)
		&& (uart_rx_buf[1] == HEADER)
		&& (uart_rx_buf[2] == RX_DATA_BLOCK_RESP_NL)
		&& (uart_rx_buf[3] == RX_DATA_BLOCK_RESP_NH)
		&& (uart_rx_buf[4] == RX_DATA_BLOCK_RESP_CMD)
		&& (uart_rx_buf[5] == 0x00)) {
		return true;
	}

	return false;
}

char  MSP::ctoh(char data) {
	if (data > '9') {
		data += 9;
	}
	return (data &= 0x0F);
}

void MSP::flush_bsl_uart_buffer() {
	while (BSL_UART.available() > 0) {
		BSL_UART.read();
	}
}

void MSP::load_firmware_from_spiffs() {

	File file = SPIFFS.open(SPIFFS_Firmware_Address, "r");

	if (!file) {
		println_debug("Failed to open file in SPIFFS");
		return;
	}

	size_t fileSize = file.size();
	print_debug("File size: ");
	println_debug(fileSize);

	const uint16_t MAX_MEMORY_LENGTH = 4;
	uint16_t sections_count = 0;
	uint32_t memory_address[MAX_MSP430_MEMORY_SECTIONS];
	char address[MAX_MEMORY_LENGTH + 1] = { 0 };
	uint32_t i = 0;
	unsigned char _byte = 0;

	firmware = (unsigned char*)malloc(fileSize + 32);

	while (file.available()) {

		_byte = (char)file.read();

		if (_byte != ' ' && _byte != '\r' && _byte != '\n') {

			if (_byte == '@') {

				address[0] = (char)file.read();
				address[1] = (char)file.read();
				address[2] = (char)file.read();
				address[3] = (char)file.read();
				address[4] = '\0';

				memory_address[sections_count] = strtol(address, NULL, 16);

				section[sections_count][0] = i; // INIT INDEX of section

				if (sections_count > 0) {
					section[sections_count - 1][1] = section[sections_count][0] - 1; // END INDEX of section
					section[sections_count - 1][2] = section[sections_count - 1][1] - section[sections_count - 1][0] + 1; // LENGTH (in bytes) of section
				}
				sections_count++;
			}
			else {
				char msb = ctoh(_byte);
				char lsb = ctoh((char)file.read());
				firmware[i++] = (unsigned char)((msb << 4) | lsb);
			}
		}
	}

	file.close();

	int16_t z = 0;
	for (z = 0; z < 150; z++)
		print_debug_hex(firmware[z], HEX);
	println_debug("----------------");

	printf_debug_1arg("exit: %d\r\n", sections_count);
	println_debug(i);
	section[sections_count - 1][1] = i - 2; //END INDEX of section
	section[sections_count - 1][2] = section[sections_count - 1][1] - section[sections_count - 1][0] + 1; //LENGTH (in bytes) of section

	for (int16_t _i = 0; _i < sections_count; _i++) {
		printf_debug_2arg("Section %d: %#lX\r\n", _i, memory_address[_i]);
		fram_address[_i] = memory_address[_i];
	}

	printf_debug_1arg("Total memory section: %d\r\n", sections_count);
	fram_sections = sections_count;

	print_debug("\r\nINIT: ");
	println_debug(section[0][0]);
	print_debug("END: ");
	println_debug(section[0][1]);
	print_debug("LENGTH: ");
	println_debug(section[0][2]);

	print_debug("INIT: ");
	println_debug(section[1][0]);
	print_debug("END: ");
	println_debug(section[1][1]);
	print_debug("LENGTH: ");
	println_debug(section[1][2]);

	print_debug("INIT: ");
	println_debug(section[2][0]);
	print_debug("END: ");
	println_debug(section[2][1]);
	print_debug("LENGTH: ");
	println_debug(section[2][2]);

	for (int16_t _i = 0; _i < sections_count; _i++)
		fram_length_of_sections[_i] = section[_i][2];
}