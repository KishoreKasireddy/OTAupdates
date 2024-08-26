#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ArduinoOTA.h>
#include <FirebaseESP8266.h>
#include <EEPROM.h>

// WiFi Credentials
const char* ssid = "vivo T3x 5G";
const char* password = "12345678";

// Firebase Config
#define FIREBASE_HOST "https://lottery-ticket-scanner-f9948-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "ZD5XnBwrXeQAOU6yscn1hWZZac4KTVEBBbZEE1RY"

// OTA and Firebase URLs
const char* updateUrl = "https://raw.githubusercontent.com/KishoreKasireddy/OTAupdates/main/firmware.ino.bin";
const char* versionPath = "/firmware/version";  // Firebase path for version
const char* urlPath = "/firmware/url";          // Firebase path for update URL

WiFiClientSecure wifiClient;
FirebaseData firebaseData;
String currentVersion = "2.0.0"; // Updated firmware version

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512); // Initialize EEPROM

  Serial.println("Starting WiFi connection...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    Serial.print("Connecting to WiFi... ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Not Connected");
  }
  Serial.println("Connected to WiFi");

  // Print WiFi signal strength
  Serial.print("WiFi Signal Strength (RSSI): ");
  Serial.println(WiFi.RSSI());

  // OTA setup
  Serial.println("Starting OTA setup...");
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname("esp8266");
  ArduinoOTA.begin();
  Serial.println("OTA setup completed.");

  // Firebase setup
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  // Retrieve and display the stored version from EEPROM
  currentVersion = getStoredVersion();
  Serial.print("Current firmware version: ");
  Serial.println(currentVersion);

  // Setup LED pin
  Serial.println("Setting up LED pin...");
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("LED pin setup completed.");
}

void loop() {
  ArduinoOTA.handle();  // Handle OTA updates

  // Blink the LED every 1 second
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);  // LED on for 1 second
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);  // LED off for 1 second

  // Check for updates periodically
  checkForUpdate();
}

void checkForUpdate() {
  Serial.println("Starting update check...");

  if (Firebase.getString(firebaseData, versionPath)) {
    String firebaseVersion = firebaseData.stringData();
    Serial.print("Current version in Firebase: ");
    Serial.println(firebaseVersion);

    if (firebaseVersion != currentVersion) {
      Serial.println("New version available, starting update...");

      if (Firebase.getString(firebaseData, urlPath)) {
        String newUpdateUrl = firebaseData.stringData();
        Serial.print("Firmware URL: ");
        Serial.println(newUpdateUrl);
        performOTAUpdate(newUpdateUrl);
      } else {
        Serial.println("Failed to get the update URL from Firebase");
      }
    } else {
      Serial.println("Firmware is up-to-date.");
    }
  } else {
    Serial.println("Failed to get the version from Firebase");
  }
}

void performOTAUpdate(const String& url) {
  wifiClient.setInsecure();

  HTTPClient httpClient;
  httpClient.begin(wifiClient, url);

  Serial.println("Performing update...");
  t_httpUpdate_return ret = ESPhttpUpdate.update(httpClient);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("Update failed: %s\n", ESPhttpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("No updates available.");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("Update successful, storing new version...");
      storeNewVersion();
      ESP.restart();
      break;
  }

  httpClient.end();
}

void storeNewVersion() {
  // Store the new version in EEPROM
  EEPROM.writeString(0, currentVersion);
  EEPROM.commit();
  Serial.println("New firmware version stored in EEPROM.");
}

String getStoredVersion() {
  // Read the stored version from EEPROM
  return EEPROM.readString(0);
}
