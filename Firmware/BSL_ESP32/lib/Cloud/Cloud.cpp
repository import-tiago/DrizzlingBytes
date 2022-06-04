#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include "Firebase_Secrets.h"
#include <addons/TokenHelper.h> //Provide the token generation process info.

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool taskCompleted = false;

void Firebase_Init() {

	Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

	// Assign the api key (required)
	config.api_key = API_KEY;

	// Assign the user sign in credentials
	auth.user.email = USER_EMAIL;
	auth.user.password = USER_PASSWORD;

	// Assign the callback function for the long running token generation task
	config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

	// Assign download buffer size in byte
	// Data to be downloaded will read as multiple chunks with this size, to compromise between speed and memory used for buffering.
	// The memory from external SRAM/PSRAM will not use in the TCP client internal rx buffer.
	config.fcs.download_buffer_size = 4096;

	Firebase.begin(&config, &auth);

	Firebase.reconnectWiFi(true);
}

