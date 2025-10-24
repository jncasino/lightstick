#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "YOUR Wi-Fi SSID HERE";
const char* password = "YOUR Wi-Fi PASSWORD HERE";

// NeoPixel configuration
#define LED_PIN 15         // Change to your data pin
#define NUM_LEDS 32       // 32 LED ring
#define BRIGHTNESS 255

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
WebServer server(80);

// Sequence storage
struct SequenceEntry {
  unsigned long timeMs;
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t brightness;
};

SequenceEntry sequence[100];
int sequenceLength = 0;
bool sequenceRunning = false;
unsigned long sequenceStartTime = 0;
int currentSequenceIndex = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize NeoPixel
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
  
  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Setup web server routes
  server.on("/ping", HTTP_GET, handlePing);
  server.on("/setcolor", HTTP_GET, handleSetColor);
  server.on("/start", HTTP_POST, handleStart);
  server.on("/stop", HTTP_GET, handleStop);
  server.on("/clear", HTTP_GET, handleClear);
  
  // Enable CORS for all routes
  server.enableCORS(true);
  
  server.begin();
  Serial.println("Web server started");
  
  // Startup animation
  colorWipe(strip.Color(0, 255, 0), 50);
  delay(500);
  strip.clear();
  strip.show();
}

void loop() {
  server.handleClient();
  
  if (sequenceRunning && sequenceLength > 0) {
    unsigned long elapsed = millis() - sequenceStartTime;
    
    // Check if we need to update to next sequence entry
    if (currentSequenceIndex < sequenceLength) {
      if (elapsed >= sequence[currentSequenceIndex].timeMs) {
        SequenceEntry* entry = &sequence[currentSequenceIndex];
        setAllPixels(entry->r, entry->g, entry->b, entry->brightness);
        currentSequenceIndex++;
      }
    } else {
      // Sequence finished
      sequenceRunning = false;
      Serial.println("Sequence complete");
    }
  }
}

void handlePing() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
}

void handleSetColor() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b")) {
    int r = server.arg("r").toInt();
    int g = server.arg("g").toInt();
    int b = server.arg("b").toInt();
    int brightness = server.hasArg("brightness") ? server.arg("brightness").toInt() : 255;
    
    setAllPixels(r, g, b, brightness);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

void handleStart() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  
  if (server.method() == HTTP_OPTIONS) {
    server.send(204);
    return;
  }
  
  String body = server.arg("plain");
  
  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    server.send(400, "text/plain", "Invalid JSON");
    return;
  }
  
  JsonArray seqArray = doc["sequence"];
  sequenceLength = min((int)seqArray.size(), 100);
  
  for (int i = 0; i < sequenceLength; i++) {
    JsonObject entry = seqArray[i];
    sequence[i].timeMs = entry["time"];
    sequence[i].r = entry["r"];
    sequence[i].g = entry["g"];
    sequence[i].b = entry["b"];
    sequence[i].brightness = entry["brightness"];
  }
  
  // Start sequence
  sequenceRunning = true;
  sequenceStartTime = millis();
  currentSequenceIndex = 0;
  
  Serial.print("Sequence loaded: ");
  Serial.print(sequenceLength);
  Serial.println(" entries");
  
  server.send(200, "text/plain", "Sequence started");
}

void handleStop() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  sequenceRunning = false;
  strip.clear();
  strip.show();
  server.send(200, "text/plain", "Stopped");
}

void handleClear() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  strip.clear();
  strip.show();
  server.send(200, "text/plain", "Cleared");
}

void setAllPixels(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
  strip.setBrightness(brightness);
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}

void colorWipe(uint32_t color, int wait) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
    strip.show();
    delay(wait);
  }
}
