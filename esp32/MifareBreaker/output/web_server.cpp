#include "output/web_server.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "output/serial_reporter.h"

static AsyncWebServer* server = nullptr;
static AsyncWebSocket* ws = nullptr;
static const CardDump* g_dump_ptr = nullptr;

static void _onReporter(const char* json) {
    if (!ws) return;
    ws->textAll(json);
}

void webServer_init(void) {
    if(!SPIFFS.begin(true)) {
        reporter_error("SPIFFS_FAIL","SPIFFS mount failed");
        return;
    }
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    uint32_t t0=millis();
    while (WiFi.status()!=WL_CONNECTED && millis()-t0<8000) delay(200);

    server = new AsyncWebServer(80);
    ws = new AsyncWebSocket("/ws");
    server->addHandler(ws);

    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/data/index.html", "text/html");
    });
    server->on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
        if (!g_dump_ptr) { request->send(404, "application/json", "{}"); return; }
        DynamicJsonDocument doc(1024);
        char uidhex[32]={0}; for (int i=0;i<g_dump_ptr->uid.len;i++) snprintf(uidhex+2*i,3, "%02X", g_dump_ptr->uid.bytes[i]);
        doc["uid"] = uidhex;
        doc["sectors"] = g_dump_ptr->totalSectors;
        String out; serializeJson(doc, out);
        request->send(200, "application/json", out);
    });
    server->on("/api/attack", HTTP_POST, [](AsyncWebServerRequest *request){
        if (request->hasParam("attack", true)) {
            AsyncWebParameter* p = request->getParam("attack", true);
            // For simplicity: just echo
            String body = String("{\"status\":\"started\",\"attack\":\"") + p->value() + "\"}";
            request->send(200, "application/json", body);
        } else {
            request->send(400, "application/json", "{\"error\":\"missing attack\"}");
        }
    });

    server->begin();
    reporter_register_cb(_onReporter);
}

void webServer_setDump(const CardDump* d) { g_dump_ptr = d; }
