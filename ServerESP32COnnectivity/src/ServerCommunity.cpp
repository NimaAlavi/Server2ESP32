#include <WiFi.h>
#include <WiFiClient.h>
#include "driver/adc.h"

#define WIFI_SSID "Alavi001"
#define WIFI_PASSWORD "09128171868AB"
#define SERVER_IP "194.60.230.41"
#define SERVER_PORT 12345
const int sampleRate = 16000;  // Sample rate in Hz
const int bufferSize = 1024;
uint16_t audioBuffer[bufferSize];
WiFiClient client;

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  Serial.println("Connected to WiFi");

  // Configure ADC
  adc1_config_width(ADC_WIDTH_BIT_12);  // Set ADC resolution to 12-bit
  adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_6);  // Set attenuation for input range 0-3.3V (ADC1_CHANNEL_7 corresponds to GPIO 35)
}

void reconnectToServer() {
  while (!client.connect(SERVER_IP, SERVER_PORT)) {
    Serial.println("Attempting to reconnect to server...");
    delay(5000);  // Wait for 5 seconds before retrying
  }
  Serial.println("Reconnected to server");
}

void loop() {
  // Check if the client is connected to the server
  if (!client.connected()) {
    reconnectToServer();
  }

  // Sample audio data
  for (int i = 0; i < bufferSize; i++) {
    audioBuffer[i] = adc1_get_raw(ADC1_CHANNEL_7);
    delayMicroseconds(1000000 / sampleRate);  // Sample at the specified rate
  }

  // Send the audio buffer to the server
  if (client.connected()) {
    client.write((uint8_t*)audioBuffer, bufferSize * sizeof(uint16_t));
  } else {
    Serial.println("Connection lost, unable to send data");
  }
}