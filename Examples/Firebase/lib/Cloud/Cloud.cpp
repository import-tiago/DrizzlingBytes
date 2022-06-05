#include <Arduino.h>
#include "Cloud.h"
#include <Firebase_ESP_Client.h>
#include "Firebase_Secrets.h"
#include <addons/TokenHelper.h>

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

void fcsDownloadCallback(FCS_DownloadStatusInfo info) {
	if (info.status == fb_esp_fcs_download_status_init) {
		Serial.printf("Downloading file %s (%d) to %s\n", info.remoteFileName.c_str(), info.fileSize, info.localFileName.c_str());
	}
	else if (info.status == fb_esp_fcs_download_status_download) {
		Serial.printf("Downloaded %d%s\n", (int)info.progress, "%");
	}
	else if (info.status == fb_esp_fcs_download_status_complete) {
		Serial.println("Download completed\n");
	}
	else if (info.status == fb_esp_fcs_download_status_error) {
		Serial.printf("Download failed, %s\n", info.errorMsg.c_str());
	}
}

void FirebaseStorage::Firebase_Init(String email, String password, String api_key) {

	Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

	// Assign the api key (required)
	config.api_key = api_key;

	// Assign the user sign in credentials
	auth.user.email = email;
	auth.user.password = password;

	// Assign the callback function for the long running token generation task
	config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

	// Assign download buffer size in byte
	// Data to be downloaded will read as multiple chunks with this size, to compromise between speed and memory used for buffering.
	// The memory from external SRAM/PSRAM will not use in the TCP client internal rx buffer.
	config.fcs.download_buffer_size = 4096;

	Firebase.begin(&config, &auth);

	Firebase.reconnectWiFi(true);
}

bool FirebaseStorage::Download_Firmware_and_Store_in_SPIFFS() {

	bool success = false;

	if (Firebase.ready()) {

		Serial.println("\nPreparing download...\n");
		if (!Firebase.Storage.download(&fbdo, store_bucket_address, cloud_address, spiffs_address, mem_storage_type_flash, fcsDownloadCallback)) {
			Serial.println(fbdo.errorReason());
			ESP.restart();
		}
		else
			success = true;
	}
	return success;
}