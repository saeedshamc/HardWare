/*
  Space Shooter - Web version
  ESP32 serves the game over WiFi; you play it in your phone's browser.

  No display, joystick or extra wiring needed - just the ESP32 and power.

  How to play:
    1) Power the ESP32 (USB cable / power bank).
    2) On your phone connect to the WiFi named  SpaceShooter  (password: 12345678)
    3) Open the browser and go to   http://192.168.4.1
    4) Drag your finger on the screen to move. The ship fires automatically.

  The game itself (HTML/JS) is in the file index_html.h (tab next to this one).
*/

#include <WiFi.h>
#include <WebServer.h>
#include "index_html.h"

// ===================== Settings =====================
// --- Access Point mode (the ESP32 creates its own WiFi) ---
const char* AP_SSID = "SpaceShooter";
const char* AP_PASS = "12345678";        // must be at least 8 characters

// --- Optional: join your home WiFi instead (set to true and fill in below) ---
// If it can't connect within 15 seconds, it falls back to Access Point mode.
// The IP address is printed in the Serial Monitor (115200 baud).
#define USE_HOME_WIFI false
const char* HOME_SSID = "YOUR_WIFI_NAME";
const char* HOME_PASS = "YOUR_WIFI_PASSWORD";

#define LED_PIN 2                         // built-in LED on most ESP32 boards

WebServer server(80);

void handleRoot() {
  server.sendHeader("Cache-Control", "no-cache");
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleNotFound() {
  // Any unknown address (e.g. /favicon.ico or a captive-portal check) -> game page
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void startNetwork() {
#if USE_HOME_WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin(HOME_SSID, HOME_PASS);
  Serial.print("Connecting to home WiFi");
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Open this address in your phone browser: http://");
    Serial.println(WiFi.localIP());
    return;
  }
  Serial.println("Home WiFi failed -> starting Access Point mode");
#endif
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("WiFi name: ");
  Serial.println(AP_SSID);
  Serial.print("Open this address in your phone browser: http://");
  Serial.println(WiFi.softAPIP());
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  delay(300);

  startNetwork();

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Web server started");
  digitalWrite(LED_PIN, HIGH);            // LED on = server is running
}

void loop() {
  server.handleClient();
}
