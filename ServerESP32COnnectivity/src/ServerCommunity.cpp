#include <WiFi.h>
#include <WiFiClient.h>
#include "driver/adc.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FS.h>

#define WIFI_SSID "Alavi001"
#define WIFI_PASSWORD "09128171868AB"
#define SERVER_IP "194.60.230.41"
#define SERVER_PORT 12345

const uint16_t sampleRate = 16000;  // Sample rate in Hz
const uint16_t bufferSize = 512;
const uint16_t totalSampleBuff = 64000;
uint16_t audioBuffer[bufferSize];
WiFiClient client;

unsigned long lastSampleTime = 0;
const unsigned long sampleInterval = 1000000 / sampleRate; // Interval between samples in microseconds
volatile bool sampleReady = false;
int bufferIndex = 0;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
void LoadingDraw(int Sample);
uint8_t loadingPosition = 0; // Position of the loading line

void reconnectToServer() {
  while (!client.connect(SERVER_IP, SERVER_PORT)) {
    Serial.println("Attempting to reconnect to server...");
    delay(5000);  // Wait for 5 seconds before retrying
  }
  Serial.println("Reconnected to server");
}

void setup() {
  Serial.begin(115200);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
    
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Clear the display
  display.clearDisplay();
  display.display();

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

      loadingPosition += 1;
      uint8_t pos = loadingPosition * SCREEN_WIDTH / (totalSampleBuff / bufferSize);
      LoadingDraw(pos);
      if (pos > SCREEN_WIDTH) {
        loadingPosition = 0; // Reset the position when it exceeds the screen width

        display.clearDisplay(); // Clear the display buffer
        display.setCursor(0, 0); // Position the cursor at the top-left corner
        display.print("Sent."); // Print the text
        display.display();
        delay(2000);
      }

    } else {
      Serial.println("Connection lost, unable to send data");
      reconnectToServer();
    }
  }
}

void LoadingDraw(int position) {
  display.clearDisplay(); // Clear the display buffer
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0, 0); // Position the cursor at the top-left corner
  display.print("Listening..."); // Print the text
  // Draw loading line
  display.drawLine(0, SCREEN_HEIGHT / 2-1, position, SCREEN_HEIGHT / 2-1, SSD1306_WHITE);
  display.drawLine(0, SCREEN_HEIGHT / 2  , position, SCREEN_HEIGHT / 2, SSD1306_WHITE);
  display.drawLine(0, SCREEN_HEIGHT / 2+1, position, SCREEN_HEIGHT / 2+1, SSD1306_WHITE);

  // Display the current drawing
  display.display();
}