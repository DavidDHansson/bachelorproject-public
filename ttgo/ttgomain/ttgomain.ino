// -----------------------------------------------------------------------------------------
// Imports and define
// -----------------------------------------------------------------------------------------

// WiFi
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// NFC
#include "emulatetag.h"
#include "NdefMessage.h"
#include "PN532.h"
#include <PN532_SPI.h>
#include <NfcAdapter.h>

// Graphics
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>  // Hardware-specific library for ST7789

// Misc.
#include <SPI.h>

// PN532 SPI
#define PN532_SCK (25)
#define PN532_MOSI (26)
#define PN532_SS (33)
#define PN532_MISO (27)

// OLED SPI
#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_CS 5
#define TFT_DC 16
#define TFT_RST 23
#define TFT_BL 4


// -----------------------------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------------------------

const int locationId = 3;

// PN532, scan and emulate
SPIClass customSPI(VSPI);
PN532_SPI pn532spi(customSPI, PN532_SS);
NfcAdapter nfcAdapter(pn532spi);
EmulateTag ncfEmulater(pn532spi);

uint8_t ndefBuf[120];
NdefMessage message;
int messageSize;

uint8_t uid[3] = { 0x12, 0x34, 0x56 };

// OLED
Adafruit_ST7789 tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// WiFi
const char *ssid = "";
const char *password = "";

const char *mqtt_broker = "";
const char *mqtt_topic = "emqx/esp32/3"; // REMEMBER LOCATION ID IS ALSO SET HERE
const char *mqtt_username = "";
const char *mqtt_password = "";
const int mqtt_port = 8883;

WiFiClientSecure esp_client;
PubSubClient mqtt_client(esp_client);

const char *ca_cert = R"EOF(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
)EOF";

// Misc.
int haltedTimeRemaining = 0;


// -----------------------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------------------


void setup() {
  Serial.begin(115200);
  delay(1000);

  // OLED
  pinMode(TFT_BL, OUTPUT);     // TTGO T-Display enable Backlight pin 4
  digitalWrite(TFT_BL, HIGH);  // T-Display turn on Backlight
  tft.init(135, 240);          // Initialize ST7789 240x135
  drawText("Setting\nup...");

  // PN532
  customSPI.begin(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
  delay(1000);

  message = NdefMessage();
  message.addUriRecord("location/" + String(locationId));
  messageSize = message.getEncodedSize();

  if (messageSize > sizeof(ndefBuf)) {
    Serial.println("ndefBuf is too small, aborting");
    while (1) {}
  }
  // Serial.print("Ndef encoded message size: ");
  // Serial.println(messageSize);

  message.encode(ndefBuf);
  ncfEmulater.setNdefFile(ndefBuf, messageSize);
  ncfEmulater.setUid(uid);  // 3 bytes please
  ncfEmulater.init();
  nfcAdapter.begin();

  // WiFi
  connectToWiFi();
  esp_client.setCACert(ca_cert);
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setKeepAlive(60);
  mqtt_client.setCallback(mqttCallback);
  connectToMqtt();

  drawText("Done!");
  delay(1000);
  Serial.println("Initialized");
  drawText("");
}


// -----------------------------------------------------------------------------------------
// Loop
// -----------------------------------------------------------------------------------------


void loop() {
  mqtt_client.loop();

  if (haltedTimeRemaining > 0) {
    haltedTimeRemaining--;
    delay(100);

    if (haltedTimeRemaining == 0) {
      drawText("");
    }

    return;
  }

  drawText("Scan\nphone or\ncard", false);

  /*
  Serial.println("Ready to emulate");
  ncfEmulater.emulate(1000);  // Time for emulating open
  ncfEmulater.setTagWriteable(false);

  if (ncfEmulater.writeOccured()) {
    Serial.println("\nWrite occured !");

    drawTextWithStatusBackground("\nSuccess", true);
    timeoutForSeconds(2);

    uint8_t *tag_buf;
    uint16_t length;

    ncfEmulater.getContent(&tag_buf, &length);
    NdefMessage msg = NdefMessage(tag_buf, length);
    msg.print();
  }
  */

  
  Serial.println("Ready to scan");
  if (nfcAdapter.tagPresent()) {
    NfcTag tag = nfcAdapter.read();
    if (tag.hasNdefMessage()) {
      NdefMessage message = tag.getNdefMessage();
      NdefRecord record = message.getRecord(0);

      int payloadLength = record.getPayloadLength();
      byte payload[payloadLength];
      record.getPayload(payload);

      // Processes the message as a string
      String payloadAsString = "";
      for (int i = 0; i < payloadLength; i++)  {
        payloadAsString += (char)payload[i];
      }

      // Ensure we have user tag
      if (payloadAsString.startsWith("user")) {
        // Extract user id, go from "user/x" to "x"
        int slashIndex = payloadAsString.indexOf('/');
        String userIdString = payloadAsString.substring(slashIndex + 1);
        int userId = userIdString.toInt();

        Serial.println("Tag scanned with user id: " + userIdString);

        processUser(userId);
      } else {
        mqtt_client.publish(mqtt_topic, "failure ");  // Public MQTT failure
        timeoutForSeconds(3);
      }
    }

  }

}


// -----------------------------------------------------------------------------------------
// Helper functions
// -----------------------------------------------------------------------------------------

// OLED
void drawTextWithStatusBackground(char *text, bool isSuccess) {
  if (isSuccess) {
    tft.fillScreen(ST77XX_GREEN);
  } else {
    tft.fillScreen(ST77XX_RED);
  }
  
  drawText(text, false);
}

void drawText(char *text) {
  drawText(text, true);
}

void drawText(char *text, bool clearScreen) {
  if (clearScreen) {
    tft.fillScreen(ST77XX_BLACK);
  }
  tft.setCursor(0, 0);
  tft.setRotation(1);
  tft.setTextSize(5);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextWrap(true);
  tft.print(text);
}

// WiFi
void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
}

void connectToMqtt() {
  while (!mqtt_client.connected()) {
    String client_id = "esp32-client-" + String(WiFi.macAddress());
    Serial.printf("Connecting to MQTT Broker as %s...\n", client_id.c_str());
    if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT broker");
      mqtt_client.subscribe(mqtt_topic);
      // mqtt_client.publish(mqtt_topic, "Hi EMQX I'm ESP32 ^^");  // Publish message upon connection
    } else {
      Serial.print("Failed to connect to MQTT broker, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" Retrying in 5 seconds.");
      delay(5000);
    }
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  String messageS = "";
  for (unsigned int i = 0; i < length; i++) {
    messageS += (char)payload[i];
  }
  Serial.println(messageS);

  if (messageS.startsWith("success")) {
    String name = messageS.substring(8); // Assuming "success " has length 8

    // Show confirmation and halt
    char messageBuffer[name.length() + 1];  // +1 for null terminator
    name.toCharArray(messageBuffer, sizeof(messageBuffer));
    drawTextWithStatusBackground(messageBuffer, true);
    timeoutForSeconds(3);

  } else if (messageS.startsWith("failure")) {
    drawTextWithStatusBackground("\nFailed", false);
    timeoutForSeconds(3);
  } else {
    Serial.println("Invalid message format");
  }
}

void processUser(int userId) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin("");
    http.addHeader("Authorization", "Bearer ");
    http.addHeader("Content-Type", "application/json");

    int status = http.POST("{\"userId\":" + String(userId) + ",\"locationId\":\"" + String(locationId) + "\", \"invalidate\": true}");

    if (status == HTTP_CODE_OK) {
      Serial.println("Successfully processed user");
    } else {
      Serial.println("Error in processing user");
    }
  }
}

void timeoutForSeconds(int seconds) {
  haltedTimeRemaining = seconds * 10;
}
