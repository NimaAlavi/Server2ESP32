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

unsigned long lastSampleTime = 0;
const unsigned long sampleInterval = 1000000 / sampleRate; // Interval between samples in microseconds
volatile bool sampleReady = false;
int bufferIndex = 0;

void reconnectToServer() {
  while (!client.connect(SERVER_IP, SERVER_PORT)) {
    Serial.println("Attempting to reconnect to server...");
    delay(5000);  // Wait for 5 seconds before retrying
  }
  Serial.println("Reconnected to server");
}

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println("\nConnected to WiFi");

  // Configure ADC
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_6);

  // Initialize variables
  lastSampleTime = micros();
}

void loop() {
  unsigned long currentTime = micros();
  
  // Sample audio data based on the sample interval
  if (currentTime - lastSampleTime >= sampleInterval) {
    if (bufferIndex < bufferSize) {
      audioBuffer[bufferIndex++] = adc1_get_raw(ADC1_CHANNEL_7);
      if (bufferIndex >= bufferSize) {
        sampleReady = true;
        bufferIndex = 0;
      }
    }
    lastSampleTime = currentTime;
  }

  // Check if the client is connected to the server
  if (!client.connected()) {
    reconnectToServer();
  }

  // Send the audio buffer if ready
  if (sampleReady) {
    if (client.connected()) {
      client.write((uint8_t*)audioBuffer, bufferSize * sizeof(uint16_t));
      sampleReady = false;
    } else {
      Serial.println("Connection lost, unable to send data");
      reconnectToServer();
    }
  }
}
