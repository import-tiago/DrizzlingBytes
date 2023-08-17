#include <Arduino.h>
#include <SPIFFS.h>
#include "MSP430.h"

//You need to ensure that the MSP430 firmware is on SPIFFS as TI-TXT format (exactly as compiled).
#define SPIFFS_FIRMWARE_ADDRESS "/txt/firmware.txt"

MSP BSL(2, 4, Serial2, SPIFFS_FIRMWARE_ADDRESS);

void setup() {

	Serial.begin(9600);

	if (!SPIFFS.begin()) {
		Serial.println("SPIFFS initialization failed.");
		return;
	}

	Serial.println("\r\n---START---");
}

void loop() {

	int16_t attempts = 3;

	BSL.load_firmware_from_spiffs();

	do {

		BSL.invoke_target_bsl_mode_operation();

		if (!BSL.write_default_password()) {
			attempts--;

			if (!attempts)
				break;

			continue;
		}

		delay(5);

		if (!BSL.write_firmware()) {
			Serial.print("\r\nMSP430 programming failed\r\n");
			attempts--;
			continue;
		}

		Serial.print("\r\nMSP430 programmed successfully\r\n");
		attempts = 0;

		BSL.invoke_target_normal_mode_operation();

		Serial.print("Device is reset\r\n");

	} while (attempts > 0);


	while (1) {
		;
	}
}
