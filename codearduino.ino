#include <WiFi.h>
#include <HTTPClient.h>

// Wi-Fi credentials
const char* ssid = "";
const char* password = "";

// Firebase URL (Realtime Database)
const char* firebaseURL = "put your firebase URL here";

// SIM808 HardwareSerial Configuration
#define RX2 16  // ESP32 GPIO pin for SIM808 TX
#define TX2 17  // ESP32 GPIO pin for SIM808 RX
HardwareSerial SerialSIM808(2);  // Use Serial2 for communication

// GPS Data
float latitude = 0.0;
float longitude = 0.0;

void setup() {
  Serial.begin(115200);
  SerialSIM808.begin(9600, SERIAL_8N1, RX2, TX2);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
}

void loop() {
  // Update GPS data
  updateGPS();

  // Send GPS data to Firebase
  sendToFirebase(latitude, longitude);

  delay(10000);  // Update every 10 seconds
}

// Function to update GPS data
void updateGPS() {
  SerialSIM808.println("AT+CGNSINF");
  delay(500);
  
  if (SerialSIM808.available()) {
    String gpsData = SerialSIM808.readString();
    Serial.println(gpsData);

    if (gpsData.indexOf("+CGNSINF:") >= 0) {
      int start = gpsData.indexOf(":") + 1;
      String data = gpsData.substring(start);
      String tokens[10];
      int index = 0;

      while (data.length() > 0 && index < 10) {
        int comma = data.indexOf(",");
        if (comma == -1) {
          tokens[index++] = data;
          break;
        }
        tokens[index++] = data.substring(0, comma);
        data = data.substring(comma + 1);
      }

      if (tokens[3].length() > 0 && tokens[4].length() > 0) {
        latitude = tokens[3].toFloat();
        longitude = tokens[4].toFloat();
        Serial.print("Latitude: ");
        Serial.print(latitude, 6);
        Serial.print(", Longitude: ");
        Serial.println(longitude, 6);
      } else {
        Serial.println("Invalid GPS data. Retrying...");
      }
    }
  }
}

// Function to send data to Firebase
void sendToFirebase(float lat, float lon) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(firebaseURL) + "/gps.json";

    // Construct JSON payload
    String jsonPayload = "{\"latitude\": " + String(lat, 6) +
                         ", \"longitude\": " + String(lon, 6) + "}";

    http.begin(url.c_str());
    http.addHeader("Content-Type", "application/json");

    // Send HTTP POST request
    int httpResponseCode = http.PUT(jsonPayload);
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Response: " + response);
    } else {
      Serial.println("Error sending data to Firebase");
    }

    http.end();
  } else {
    Serial.println("Wi-Fi not connected");
  }
}
