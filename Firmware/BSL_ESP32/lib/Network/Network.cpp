#include "WiFi_Secrets.h"
#include <Arduino.h>
#include <WiFi.h>

void WiFi_Init() {

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("\r\nonnecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {

        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
}