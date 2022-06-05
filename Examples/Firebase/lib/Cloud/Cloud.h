#ifndef CLOUD_H
#define CLOUD_H

#include <Arduino.h>
#include <Firebase_ESP_Client.h>

extern FirebaseData fbdo;
extern FirebaseAuth auth;
extern FirebaseConfig config;

class FirebaseStorage {

	private:

	String cloud_address;
	String spiffs_address;
	String store_bucket_address;

	public:
	FirebaseStorage(String store_bucket_address, String cloud_address, String spiffs_address) {
		this->cloud_address = cloud_address;
		this->spiffs_address = spiffs_address;
		this->store_bucket_address = store_bucket_address;
	}
	bool Download_Firmware_and_Store_in_SPIFFS();
	void Firebase_Init(String email, String password, String api_key);
};

#endif // CLOUD_H