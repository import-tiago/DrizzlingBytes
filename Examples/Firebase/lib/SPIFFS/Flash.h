#ifndef FLASH_H
#define FLASH_H

#include <Arduino.h>
#include <SPIFFS.h>

#define RUN_MEMORY_FORMAT true
#define DONT_RUN_MEMORY_FORMAT false

extern File Save_File;

// PROTOTYPES
void SPI_Flash_File_System_Init(String SPIFFS_Firmware_Address, bool format_spiffs);

#endif // FLASH_H