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
  bool rainbowEffect;
};

SequenceEntry sequence[100];
int sequenceLength = 0;
bool sequenceRunning = false;
unsigned long sequenceStartTime = 0;
int currentSequenceIndex = 0;
bool rainbowActive = false;
unsigned long rainbowStartTime = 0;
uint8_t rainbowBrightness = 255;
unsigned long rainbowDuration = 1000; // Rainbow runs for 1 second by default

void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial time to initialize
  
  Serial.println("=== ESP32 Starting ===");
  
  // Initialize NeoPixel
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
  Serial.println("NeoPixel initialized");
  
  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed!");
    return;
  }
  
  // Setup web server routes
  server.on("/ping", HTTP_GET, handlePing);
  server.on("/setcolor", HTTP_GET, handleSetColor);
  server.on("/rainbow", HTTP_GET, handleRainbow);
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
  
  Serial.println("=== Setup Complete ===");
}

void loop() {
  static unsigned long lastDebug = 0;
  
  // Debug heartbeat every 5 seconds
  if (millis() - lastDebug > 5000) {
    Serial.println("Main loop running...");
    lastDebug = millis();
  }
  
  server.handleClient();
  
  // Handle rainbow effect animation
  if (rainbowActive) {
    unsigned long rainbowElapsed = millis() - rainbowStartTime;
    // Check if rainbow duration has expired
    if (rainbowElapsed > rainbowDuration) {
      Serial.print("Rainbow duration expired: ");
      Serial.print(rainbowElapsed);
      Serial.print("ms > ");
      Serial.print(rainbowDuration);
      Serial.println("ms");
      stopRainbow();
    } else {
      updateRainbow();
    }
  }
  
  if (sequenceRunning && sequenceLength > 0) {
    unsigned long elapsed = millis() - sequenceStartTime;
    
    // Check if we need to update to next sequence entry
    if (currentSequenceIndex < sequenceLength) {
      if (elapsed >= sequence[currentSequenceIndex].timeMs) {
        SequenceEntry* entry = &sequence[currentSequenceIndex];
        
        // Stop any active rainbow before setting new color
        stopRainbow();
        
        Serial.print("Sequence entry ");
        Serial.print(currentSequenceIndex);
        Serial.print(" at ");
        Serial.print(elapsed);
        Serial.print("ms - ");
        
        if (entry->rainbowEffect) {
          // Calculate rainbow duration based on time until next entry
          if (currentSequenceIndex + 1 < sequenceLength) {
            rainbowDuration = sequence[currentSequenceIndex + 1].timeMs - entry->timeMs;
            Serial.print("Calculated rainbow duration: ");
            Serial.print(rainbowDuration);
            Serial.println("ms");
          } else {
            rainbowDuration = 1000; // Default 1 second if this is the last entry
            Serial.println("Using default rainbow duration: 1000ms");
          }
          
          // Start rainbow effect for this entry
          Serial.print("Starting rainbow for ");
          Serial.print(rainbowDuration);
          Serial.println("ms");
          startRainbow(entry->brightness);
        } else {
          Serial.print("Setting color R:");
          Serial.print(entry->r);
          Serial.print(" G:");
          Serial.print(entry->g);
          Serial.print(" B:");
          Serial.println(entry->b);
          setAllPixels(entry->r, entry->g, entry->b, entry->brightness);
        }
        
        currentSequenceIndex++;
      }
    } else {
      // Sequence finished
      sequenceRunning = false;
      stopRainbow();
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

void handleRainbow() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  
  int brightness = server.hasArg("brightness") ? server.arg("brightness").toInt() : 255;
  
  startRainbow(brightness);
  server.send(200, "text/plain", "Rainbow effect started");
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
    sequence[i].rainbowEffect = entry["rainbowEffect"] | false;
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
  stopRainbow();
  strip.clear();
  strip.show();
  server.send(200, "text/plain", "Stopped");
}

void handleClear() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  stopRainbow();
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

// Rainbow effect functions
uint32_t Wheel(byte WheelPos) {
  WheelPos = 63 - WheelPos; // Adjusted for 64 color range
  if(WheelPos < 21) {
    return strip.Color(255 - WheelPos * 12, 0, WheelPos * 12);
  }
  if(WheelPos < 43) {
    WheelPos -= 21;
    return strip.Color(0, WheelPos * 12, 255 - WheelPos * 12);
  }
  WheelPos -= 43;
  return strip.Color(WheelPos * 12, 255 - WheelPos * 12, 0);
}

void rainbowCycle(uint8_t brightness) {
  uint16_t i, j;
  strip.setBrightness(brightness);
  
  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(20);
  }
}

// WARNING: This function is BLOCKING and should NOT be used during sequence playback
// Use startRainbow() + updateRainbow() instead for non-blocking rainbow effects
void rainbow(uint8_t brightness) {
  uint16_t i, j;
  strip.setBrightness(brightness);
  
  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(20);
  }
}

// Non-blocking rainbow functions
void startRainbow(uint8_t brightness) {
  rainbowActive = true;
  rainbowBrightness = brightness;
  rainbowStartTime = millis();
  strip.setBrightness(brightness);
  Serial.print("Rainbow started with duration: ");
  Serial.print(rainbowDuration);
  Serial.println("ms");
}

void stopRainbow() {
  rainbowActive = false;
  Serial.println("Rainbow stopped");
}

void updateRainbow() {
  static uint8_t rainbowIndex = 0;
  static unsigned long lastUpdate = 0;
  
  if (millis() - lastUpdate > 25) { // Update every 25ms for faster spinning
    for(int i=0; i<strip.numPixels(); i++) {
      // Use 64 colors for even faster transitions
      strip.setPixelColor(i, Wheel(((i * 64 / strip.numPixels()) + rainbowIndex) & 63));
    }
    strip.show();
    rainbowIndex++;
    // Reset index at 64 for faster cycling
    if (rainbowIndex >= 64) {
      rainbowIndex = 0;
    }
    lastUpdate = millis();
  }
}
