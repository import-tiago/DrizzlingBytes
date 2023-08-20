#pragma once
#define DRIZZLING_BYTES_VERSION "2.1"
/*
 *	v1.0 - Initial release.
 *	v1.1 - Flush RX UART buffer (MSP430 received bytes) before each transmission to allow multiple sequential OTA updates.
 *  v2.0 - Improvements in memory usage
 *  v2.1 - Improved firmware flow and debug messages
 */

#include <Arduino.h>
#include <HardwareSerial.h>
#include <FS.h>
#include <SPIFFS.h>

#define ENABLE_SERIAL_DEBUG 0

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

 /**
  * @brief Construct a new BSL object to control the Bootstrap Loader programming of the target MSP430.
  *
  * @param reset_pin Any GPIO with output mode capability to be connected to the RESET pin of the target MSP430.
  * @param test_pin Any GPIO with output mode capability to be connected to the TEST pin of the target MSP430.
  * @param serial_port HardwareSerial object from Arduino Framework (Serial, Serial1, Serial2...).
  * @param spiffs_addr The local SPI Flash File System address where the BSL object must look for the firmware_bytes file in TI-TXT format.
  */
class MSP {
public:
    MSP(byte reset_pin, byte test_pin, HardwareSerial& serial_port, const char* spiffs_addr) : serial(serial_port) {

        this->reset_pin = reset_pin;
        this->test_pin = test_pin;
        this->spiffs_addr = spiffs_addr;

        serial.begin(9600, SERIAL_8E1);
        serial.setTimeout(500);

        pinMode(test_pin, OUTPUT);
        pinMode(reset_pin, OUTPUT);

        digitalWrite(test_pin, LOW);
        digitalWrite(reset_pin, HIGH);
    }
    ~MSP() {}

    void load_firmware_from_spiffs();
    void invoke_target_bsl_mode_operation();
    bool write_default_password();
    bool write_firmware();
    void invoke_target_normal_mode_operation();

private:
    byte reset_pin;
    byte test_pin;
    HardwareSerial serial;
    const char* spiffs_addr;

    uint16_t calc_checksum(char* data, uint16_t length);
    bool send_large_data(uint32_t startAddress, uint32_t length, unsigned char* data);
    bool write_memory(uint32_t startAddress, char lenght, unsigned char* data);
    char  ctoh(char data);
    void flush_bsl_uart_buffer();
};