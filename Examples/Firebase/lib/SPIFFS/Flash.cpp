#include <Arduino.h>
#include <Flash.h>
#include <SPIFFS.h>

File Save_File;

void SPI_Flash_File_System_Init(String SPIFFS_Firmware_Address, bool format_spiffs) {

	if (format_spiffs) {
		if (SPIFFS.format())
			Serial.println("Success formatting");
		else
			Serial.println("Error formatting");
	}

	if (!SPIFFS.begin(true)) {
		Serial.println("SPIFFS/LittleFS initialization failed.");
		return;
	}

	Serial.print("TOTAL FLASH: ");
	Serial.println(SPIFFS.totalBytes());

	Serial.print("USED FLASH: ");
	Serial.println(SPIFFS.usedBytes());

	Serial.println(ESP.getFlashChipSize());
}