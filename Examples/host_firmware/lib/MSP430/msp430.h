#pragma once
#define DRIZZLING_BYTES_VERSION "2.2"
/*
 *	v1.0 - Initial release.
 *	v1.1 - Flush RX UART buffer (MSP430 received bytes) before each transmission to allow multiple sequential OTA updates.
 *  v2.0 - Changes from static memory allocation to dynamic.
 *  v2.1 - Improved firmware flow and debug messages.
 *  v2.2 - Added two class constructors to allows (or not) code execution with serial debug messages.
 */

#include <Arduino.h>
#include <HardwareSerial.h>
#include <FS.h>
#include <SPIFFS.h>

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
    MSP(byte reset_pin, byte test_pin, HardwareSerial& serial_port, const char* spiffs_addr, HardwareSerial& debug_port) : serial(serial_port), serial_debug(debug_port) {

        print_debug = [this](const char* x) { serial_debug.print(x); };
        println_debug = [this](const char* x) { serial_debug.println(x); };
        print_debug_hex = [this](byte x) { serial_debug.printf("0x%02X ", x); };
        printf_debug_1arg = [this](const char* x, int y) { serial_debug.printf(x, y); };
        printf_debug_2arg = [this](const char* x, int y, int z) { serial_debug.printf(x, y, z); };

        if (!SPIFFS.begin(true)) {
            Serial.println("Flash filesystem initialization failed!");
            return;
        }

        this->reset_pin = reset_pin;
        this->test_pin = test_pin;
        this->spiffs_addr = spiffs_addr;

        serial_debug.begin(115200);

        serial.begin(9600, SERIAL_8E1);
        serial.setTimeout(500);

        pinMode(test_pin, OUTPUT);
        pinMode(reset_pin, OUTPUT);

        digitalWrite(test_pin, LOW);
        digitalWrite(reset_pin, HIGH);
    }
    MSP(byte reset_pin, byte test_pin, HardwareSerial& serial_port, const char* spiffs_addr) : serial(serial_port), serial_debug(-1) {

        if (!SPIFFS.begin(true)) {
            Serial.println("Flash filesystem initialization failed!");
            return;
        }

        print_debug = [](const char*) {};
        println_debug = [](const char*) {};
        print_debug_hex = [](byte) {};
        printf_debug_1arg = [](const char*, int) {};
        printf_debug_2arg = [](const char*, int, int) {};

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
    HardwareSerial serial_debug;
    const char* spiffs_addr;

    using PrintDebugFunc = std::function<void(const char*)>;
    using PrintDebugHexFunc = std::function<void(byte)>;
    using PrintfDebug1ArgFunc = std::function<void(const char*, int)>;
    using PrintfDebug2ArgFunc = std::function<void(const char*, int, int)>;

    PrintDebugFunc print_debug;
    PrintDebugFunc println_debug;
    PrintDebugHexFunc print_debug_hex;
    PrintfDebug1ArgFunc printf_debug_1arg;
    PrintfDebug2ArgFunc printf_debug_2arg;

    uint16_t calc_checksum(char* data, uint16_t length);
    bool send_large_data(uint32_t startAddress, uint32_t length, unsigned char* data);
    bool write_memory(uint32_t startAddress, char lenght, unsigned char* data);
    char ctoh(char data);
    void flush_bsl_uart_buffer();
};