#ifndef MYCLASS_H
#define MYCLASS_H
#define DRIZZLING_BYTES_VERSION "2.1"

/*
 *	v1.0 - Initial release.
 *	v1.1 - Flush RX UART buffer (MSP430 received bytes) before each transmission to allow multiple sequential OTA updates.
 *  v2.0 - Improvements in memory usage
 */

#include <Arduino.h>
#include <HardwareSerial.h>
#include <FS.h>
#include <SPIFFS.h>

#define STRINGIZE(x) #x
#define EXPAND_AND_STRINGIZE(x) STRINGIZE(x)

#define ENABLE_SERIAL_DEBUG 1

#if ENABLE_SERIAL_DEBUG
#define DEBUG_PORT Serial

#define print_debug(x); Serial.print(x);
#define println_debug(x); Serial.println(x);
#define print_debug_hex(x); Serial.printf("0x%02X ", x);
#define printf_debug_1arg(x, y); Serial.printf(x, y);
#define printf_debug_2arg(x, y, z); Serial.printf(x, y, z);
#else
#define print_debug(x);
#define println_debug(x);
#define print_debug_hex(x);
#define printf_debug_1arg(x, y);
#define printf_debug_2arg(x, y, z);
#endif

#define MAX_MSP430_MEMORY_SECTIONS 	30

#define UART_BUFFER_SIZE 300

char uart_tx_buf[UART_BUFFER_SIZE] = { 0 };
char uart_rx_buf[UART_BUFFER_SIZE] = { 0 };
uint32_t fram_address[MAX_MSP430_MEMORY_SECTIONS];
uint32_t fram_length_of_sections[MAX_MSP430_MEMORY_SECTIONS];
uint32_t fram_sections;
uint32_t section[MAX_MSP430_MEMORY_SECTIONS][3];
unsigned char* firmware_bytes;

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

 /**
  * @brief Construct a new BSL object to control the Bootstrap Loader programming of the target MCU.
  *
  * @param reset_pin Any GPIO with output mode capability to be connected to the RESET pin of the target MSP430.
  * @param test_pin Any GPIO with output mode capability to be connected to the TEST pin of the target MSP430.
  * @param serial_port HardwareSerial object from Arduino Framework (Serial, Serial1, Serial2...).
  * @param spiffs_addr The local SPI Flash File System address where the BSL object must look for the firmware_bytes file in TI-TXT format.
  */




template <HardwareSerial& DebugPin>
class MSP {
public:
    //static_assert(!is_same_as_debug_port<decltype(DebugPin)>::value, "Warning: pin types are the same!");
    MSP(byte reset_pin, byte test_pin, const char* spiffs_addr) : setDebugPin(DebugPin) {

// Check if DebugPin matches DEBUG_PORT and generate a warning if not
#if (DebugPin != DEBUG_PORT)
    #warning "DebugPin doesn't match DEBUG_PORT"
#endif

            this->test_pin = test_pin;
        this->reset_pin = reset_pin;
        this->spiffs_addr = spiffs_addr;

        ///  target_uart.begin(9600, SERIAL_8E1);
         // target_uart.setTimeout(500);

        pinMode(test_pin, OUTPUT);
        pinMode(reset_pin, OUTPUT);

        digitalWrite(test_pin, LOW);
        digitalWrite(reset_pin, HIGH);
    }
    ~MSP() {}

    void invoke_target_normal_mode_operation() {

        free(firmware_bytes);

        digitalWrite(test_pin, LOW);
        digitalWrite(reset_pin, HIGH);
        delay(200);

        digitalWrite(reset_pin, LOW);
        delayMicroseconds(100);
        digitalWrite(reset_pin, HIGH);
        delayMicroseconds(100);
    }

    void invoke_target_bsl_mode_operation() {

        println_debug("Invoking target bootloader by hardware entry sequence...");

        digitalWrite(reset_pin, HIGH);
        digitalWrite(test_pin, HIGH);
        delay(200);

        digitalWrite(test_pin, LOW);
        digitalWrite(reset_pin, LOW);
        delay(100);

        digitalWrite(test_pin, HIGH);
        delay(50);
        digitalWrite(test_pin, LOW);
        delay(50);

        digitalWrite(test_pin, HIGH);
        delay(50);
        digitalWrite(reset_pin, HIGH);
        delay(50);
        digitalWrite(test_pin, LOW);

        delay(200);
    }

    bool write_default_password() {

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

        print_debug("Writing target bootloader password... ");

        flush_bsl_uart_buffer();

        /*     target_uart.write(uart_tx_buf, PASSWORD_LENGTH + 6);

            while (target_uart.available() <= 0) {
                ;
            }

            target_uart.readBytes(uart_rx_buf, 8); */

        print_debug("response: ");
        print_debug_hex(uart_rx_buf[0]);
        print_debug_hex(uart_rx_buf[1]);
        print_debug_hex(uart_rx_buf[2]);
        print_debug_hex(uart_rx_buf[3]);
        print_debug_hex(uart_rx_buf[4]);
        print_debug_hex(uart_rx_buf[5]);

        if ((uart_rx_buf[0] == ACK)
            && (uart_rx_buf[1] == HEADER)
            && (uart_rx_buf[2] == RX_PASSWORD_RESP_NL)
            && (uart_rx_buf[3] == RX_PASSWORD_RESP_NH)
            && (uart_rx_buf[4] == RX_PASSWORD_RESP_CMD)
            && (uart_rx_buf[5] == 0x00)) {

            print_debug("... successful!\r\n");
            result = true;
        }
        else {
            print_debug("... failed!\r\n");
            result = false;
        }

        delay(50);

        return result;
    }

    bool write_firmware() {

        bool numberOfErrors = 0;
        bool result = true;
        uint16_t region = 0;
        unsigned char* framStartOfData = 0;

        for (region = 0; region < fram_sections; region++) {

            framStartOfData = &firmware_bytes[section[region][0]];

            result = send_large_data(fram_address[region], fram_length_of_sections[region], framStartOfData);

            if (!result) {
                print_debug("... failed!\r\n");
                return false;
            }
            else
                print_debug("... successful!\r\n");
        }

        if (numberOfErrors != 0)
            return false;

        return true;
    }

    void load_firmware_from_spiffs() {

        File file = SPIFFS.open(spiffs_addr, "r");

        if (!file) {
            println_debug("Failed to open file in SPIFFS");
            return;
        }

        size_t firmware_size = file.size();

        printf_debug_1arg("MSP430 firmware file size: %d bytes\r\n", firmware_size);

        const uint16_t MAX_MEMORY_LENGTH = 4;
        uint16_t sections_count = 0;
        uint32_t memory_address[MAX_MSP430_MEMORY_SECTIONS];
        char address[MAX_MEMORY_LENGTH + 1] = { 0 };
        uint32_t i = 0;
        unsigned char _byte = 0;

        firmware_bytes = (unsigned char*)malloc(firmware_size + 32);

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
                    firmware_bytes[i++] = (unsigned char)((msb << 4) | lsb);
                }
            }
        }

        file.close();

        section[sections_count - 1][1] = i - 2; //END-INDEX of section
        section[sections_count - 1][2] = section[sections_count - 1][1] - section[sections_count - 1][0] + 1; //LENGTH (in bytes) of section

        for (i = 0; i < sections_count; i++) {
            printf_debug_2arg("Section %d: %#lX\r\n", i, memory_address[i]);
            fram_address[i] = memory_address[i];
        }

        fram_sections = sections_count;

        printf_debug_1arg("Total sections: %d\r\n", sections_count);

        for (i = 0; i < fram_sections; i++) {
            printf_debug_2arg("Total bytes in section %d: %d bytes\r\n", i, section[i][2]);
        }

        for (i = 0; i < sections_count; i++) {
            fram_length_of_sections[i] = section[i][2];
        }
    }

private:
    HardwareSerial setDebugPin;
    byte test_pin;
    byte reset_pin;
    //HardwareSerial& target_uart;
    const char* spiffs_addr;

    uint16_t calc_checksum(char* data, uint16_t length) {

        char x;
        uint16_t crc = 0xFFFF;

        while (length--) {
            x = crc >> 8 ^ *data++;
            x ^= x >> 4;
            crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ ((uint16_t)x);
        }

        return crc;
    }

    bool send_large_data(uint32_t startAddress, uint32_t length, unsigned char* data) {

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

    bool write_memory(uint32_t startAddress, char lenght, unsigned char* data) {

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

        print_debug("Writing in memory... ");

        flush_bsl_uart_buffer();

        /*     target_uart.write(uart_tx_buf, lenght + 9);

            while (target_uart.available() <= 0) {
                ;
            }

            target_uart.readBytes(uart_rx_buf, 8); */

        print_debug("response: ");
        print_debug_hex(uart_rx_buf[0]);
        print_debug_hex(uart_rx_buf[1]);
        print_debug_hex(uart_rx_buf[2]);
        print_debug_hex(uart_rx_buf[3]);
        print_debug_hex(uart_rx_buf[4]);
        print_debug_hex(uart_rx_buf[5]);

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

    char  ctoh(char data) {
        if (data > '9') {
            data += 9;
        }
        return (data &= 0x0F);
    }

    void flush_bsl_uart_buffer() {
        /*     while (target_uart.available() > 0) {
                target_uart.read();
            } */
    }
};


#endif // MYCLASS_H