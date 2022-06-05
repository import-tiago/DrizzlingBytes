#include <Arduino.h>
#include "MSP430.h"
#include "Network.h"
#include "WiFi_Secrets.h"
#include "Firebase_Secrets.h"
#include "Cloud.h"
#include <Flash.h>

#define FIREBASE_FIRMWARE_ADDRESS "txt/firmware.txt"
#define SPIFFS_FIRMWARE_ADDRESS "/txt/firmware.txt"

/*
The BSL object from MSP430 class is not Firebase-dependent. It's SPIFFS dependent.

So, you just need ensure that the firmware there is on SPIFFS in the TI-TXT format (without any modification, exactly as it was compiled).

In this way, your application can download firmware from any other source and to use the ESP32's internal flash memory as an common interface.
*/
MSP430 BSL(2, 4, Serial2, SPIFFS_FIRMWARE_ADDRESS);

/*
Just as an example and for demonstration purposes,
in this implementation Firebase will be used as a server that stores the firmware that will be programmed in the MSP430.
*/
FirebaseStorage OTA(STORAGE_BUCKET_ADDRESS, FIREBASE_FIRMWARE_ADDRESS, SPIFFS_FIRMWARE_ADDRESS);

void setup() {
	Serial.begin(9600);

	WiFi_Init();

	OTA.Firebase_Init(USER_EMAIL, USER_PASSWORD, API_KEY);

	SPI_Flash_File_System_Init(SPIFFS_FIRMWARE_ADDRESS, DONT_RUN_MEMORY_FORMAT);

	Serial.println("\r\n---START---");
}

void loop() {

	static int retry = 3;

	if (OTA.Download_Firmware_and_Store_in_SPIFFS()) {

		BSL.Load_from_SPIFFS_and_Store_in_RAM();

		do {

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
	}

	while (1) {
		;
	}
}
