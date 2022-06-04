#ifndef CLOUD_H
#define CLOUD_H

#include <Arduino.h>
#include <Firebase_ESP_Client.h>

#define CLOUD_FIRMWARE_ADDRESS "txt/Fast_Blinky.txt"
#define LOCAL_FLASH_ADDRESS "/txt/firmware.txt"


extern FirebaseData fbdo;
extern FirebaseAuth auth;
extern FirebaseConfig config;

extern bool taskCompleted;

// PROTOTYPES
void Firebase_Init();
//void fcsDownloadCallback(FCS_DownloadStatusInfo info);

#endif // CLOUD_H
