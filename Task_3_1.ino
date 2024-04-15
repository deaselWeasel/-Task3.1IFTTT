#include <WiFiNINA.h>
#include <Wire.h>
#include "secrets.h" 

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

WiFiClient client;
char HOST_NAME[] = "maker.ifttt.com";
String YOUR_IFTTT_KEY = "dIvjVeS12dE3xkjdOPEzxY"; 

const int BH1750_address = 0x23;

void setup() {
  Serial.begin(9600);
  Wire.begin(); // Initialize I2C for the BH1750 sensor
  WiFi.begin(ssid, pass); // Connect to WiFi

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // Initialize BH1750 sensor
  Wire.beginTransmission(BH1750_address);
  Wire.write(0x01); // Power on
  Wire.endTransmission();
  delay(10); // Allow time for sensor to power up
  
  Wire.beginTransmission(BH1750_address);
  Wire.write(0x10); // Continuously measure in high resolution mode
  Wire.endTransmission();
  delay(10); // Allow time for mode setup
}

void loop() {
  int lightLevel = readLightSensor(); // Get the current light level from the sensor
  
  if (lightLevel == -1) {
    Serial.println("Error reading light sensor");
    return; // Skip this loop iteration if an error occurs
  }

  if (lightLevel > 300) {
    triggerIFTTT("part_1", lightLevel);
  } else if (lightLevel < 30) {
    triggerIFTTT("part_2", lightLevel);
  }

  delay(3000); // Wait for 3 seconds before next reading
}

int readLightSensor() {
  Wire.beginTransmission(BH1750_address);
  Wire.requestFrom(BH1750_address, 2);
  if (Wire.available() == 2) {
    int reading = (Wire.read() << 8) | Wire.read();
    return reading / 1.2; // Convert to lux
  }
  return -1; // Error reading sensor
}

void triggerIFTTT(String eventName, int lightLevel) {
  if (client.connect(HOST_NAME, 80)) {
    String queryString = "/trigger/" + eventName + "/with/key/" + YOUR_IFTTT_KEY 
                         + "?value1=" + String(lightLevel);
    // Make the HTTP request to IFTTT
    client.println("GET " + queryString + " HTTP/1.1");
    client.println("Host: " + String(HOST_NAME));
    client.println("Connection: close");
    client.println(); // End HTTP header
    
    while (client.connected() || client.available()) {
      char c = client.read();
      Serial.write(c);
    }
    client.stop(); // Disconnect after sending the request
    Serial.println("IFTTT event '" + eventName + "' triggered with light level: " + String(lightLevel));
  } else {
    Serial.println("Failed to connect to IFTTT");
  }
}
