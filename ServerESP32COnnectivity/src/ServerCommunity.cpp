#include <WiFi.h>
#include <WiFiClient.h>
#include "driver/adc.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FS.h>
#include <string.h>

#define WIFI_SSID "Alavi001"
#define WIFI_PASSWORD "09128171868AB"
#define SERVER_IP "194.60.230.41"
#define SERVER_PORT 13579

const uint16_t sampleRate = 16000;  // Sample rate in Hz
const uint16_t bufferSize = 1024;
const uint16_t totalSampleBuff = 48000;
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

struct KeyValuePair {
  char key[9];
  int value;
};
#define itemNUM 3
#define placeNUM 2
KeyValuePair itemID[itemNUM] = {
  {"00000001", 1}, //Locate to کولر
  {"00000010", 2}, //Locate to چراغ, لامپ
  {"00000011", 2}  //Locate to چراغ, لامپ
};
KeyValuePair placeID[placeNUM] = {
  {"00000001", 12}, //Locate اتاق to PIN 12
  {"00000010", 13}  //Locate آشپزخانه to PIN 13
};
#define LED_Room 12
#define LED_Balcon 13

void LEDCommander(char input[9]);
void reconnectToServer() {
  while (!client.connect(SERVER_IP, SERVER_PORT)) {
    Serial.println("Attempting to reconnect to server...");
    delay(5000);  // Wait for 5 seconds before retrying
  }
  Serial.println("Reconnected to server");
}

// Function to find value by key
int getValueByKey(const KeyValuePair* dict, int size, const char* key) {
    for (int i = 0; i < size; i++) {
        if (strcmp(dict[i].key, key) == 0) {
            return dict[i].value;
        }
    }
    return -1; // Return -1 if key is not found
}
void extractSubstr(const char* input, int start, int end, char* output) {
    int j = 0;
    for (int i = start; i <= end; i++) {
        output[j++] = input[i];
    }
    output[j] = '\0'; // Null-terminate the string
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_Room, OUTPUT);
  pinMode(LED_Balcon, OUTPUT);

  digitalWrite(LED_Room, 0);
  digitalWrite(LED_Balcon, 0);

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
        delay(5000);

        // Wait for response from the server
        if (client.available()) {
          String resultText = client.readStringUntil('\n');  // Read the result text sent by the server
          char resultArray[25];
          resultText.toCharArray(resultArray, sizeof(resultArray));
          LEDCommander(resultArray);

          display.clearDisplay();
          display.setCursor(0, 0);
          display.print("Result:");
          display.setCursor(0, 30);
          display.print(resultText);
          display.display();
          delay(5000);  // Keep the result on display for 5 seconds
        }
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

void LEDCommander(char input[25]) {
    char itemKey[9];
    char placeKey[9];
    // Serial.println(input);

    // Extract the relevant parts of the input
    extractSubstr(input, 16, 23, itemKey);  // Extract characters 16 to 23
    extractSubstr(input, 8, 15, placeKey);  // Extract characters 8 to 15

    int itemValue = getValueByKey(itemID, itemNUM, itemKey);
    int placePin = getValueByKey(placeID, placeNUM, placeKey);

    // Serial.print(itemKey);
    // Serial.print(' ');
    // Serial.print(placePin);
    // Serial.print(' ');
    // Serial.println(input[0]);
    // Check if the item is a light (value 2)
    if (itemValue == 2) {
        // Extract bits from input[0] and input[1] and use them to set the LED state
        int ledState = (input[0] - '0') | (input[1] - '0');  // Assuming input[0] and input[1] are '0' or '1'
        digitalWrite(placePin, ledState);
        // Serial.println(ledState);
    }
}