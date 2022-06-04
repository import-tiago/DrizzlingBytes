#include <Arduino.h>
#include "MSP430.h"
#include "Network.h"
#include "WiFi_Secrets.h"
#include "Firebase_Secrets.h"
#include "Cloud.h"

MSP430_BSL BSL(2, 4, Serial2);
MSP430_OTA OTA;

void setup() {
	Serial.begin(9600);

	WiFi_Init();

	Firebase_Init();

	/* 	if (SPIFFS.format())
			Serial.println("Success formatting");
		else
			Serial.println("Error formatting"); */

	if (!DEFAULT_FLASH_FS.begin()) {
		Serial.println("SPIFFS/LittleFS initialization failed.");
		return;
	}

	if (DEFAULT_FLASH_FS.exists(LOCAL_FLASH_ADDRESS)) {
		Serial.println("Removing old firmware...");
		DEFAULT_FLASH_FS.remove(LOCAL_FLASH_ADDRESS);
	}

	Serial.print("TOTAL FLASH: ");
	Serial.println(SPIFFS.totalBytes());

	Serial.print("USED FLASH: ");
	Serial.println(SPIFFS.usedBytes());

	Serial.println(ESP.getFlashChipSize());

	Serial.println("\r\n---START---");

}

void loop() {

	static int retry = 3;

	do {

		OTA.Download_Firmware();

		BSL.Invoke_MSP_BSL_Mode_Operation();

		if (!BSL.Write_Default_Password()) {
			retry--;

			if (!retry)
				break;

			continue;
		}

		delay(5);

		if (!BSL.Write_Firmware()) {
			Serial.print("\r\nMSP430 programming failed\r\n");
			retry--;
			continue;
		}

		Serial.print("\r\nMSP430 programmed successfully\r\n");
		retry = 0;

		BSL.Invoke_MSP_Normal_Mode_Operation();

		Serial.print("Device is reset\r\n");

	} while (retry > 0);

	while (1) {
		;
	}
}
