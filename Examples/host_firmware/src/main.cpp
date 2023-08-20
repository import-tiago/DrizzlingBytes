#include <Arduino.h>
#include "msp430.h"

#define MSP30_RESET_PIN 26
#define MSP30_TEST_PIN 25

MSP BSL(MSP30_RESET_PIN, MSP30_TEST_PIN, Serial2, "/target_firmware.txt");
//MSP BSL(MSP30_RESET_PIN, MSP30_TEST_PIN, Serial2, "/target_firmware.txt", Serial); //DEBUG MODE

bool upload_msp430_firmware() {

    int16_t attempts = 3;
    BSL.load_firmware_from_spiffs();

    do {
        BSL.invoke_target_bsl_mode_operation();

        if (!BSL.write_default_password()) {
            if (!--attempts)
                return false;
            continue;
        }

        delay(5);

        attempts = 3;
        if (!BSL.write_firmware()) {
            if (!--attempts)
                return false;
            continue;
        }
        else {
            BSL.invoke_target_normal_mode_operation();
            return true;
        }

    } while (attempts > 0);

    BSL.invoke_target_normal_mode_operation();
    return false;
}

void setup() {
    Serial.begin(115200);

    if (upload_msp430_firmware())
        Serial.print("\r\nMSP430 successfully programmed!\r\n");
    else
        Serial.print("\r\nMSP430 programming failed!\r\n");
}

void loop() {}