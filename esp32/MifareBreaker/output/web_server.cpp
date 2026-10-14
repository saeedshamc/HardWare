#include "output/web_server.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "output/serial_reporter.h"
#include "storage/dump_manager.h"

static AsyncWebServer* server = nullptr;
static AsyncWebSocket* ws = nullptr;
static const CardDump* g_dump_ptr = nullptr;
static bool attack_running = false;

static void _onReporter(const char* json) {
    if (!ws) return;
    ws->textAll(json);
}

static void _runAttackAsync(const String attackName) {
    if (attack_running) return;
    attack_running = true;
    // Run in background task
    xTaskCreate([](void* param){
        String a = *((String*)param);
        delete (String*)param;
        if (a=="dictionary") {
            if (g_dump_ptr) dictAttack_run((CardDump*)g_dump_ptr);
        } else if (a=="nested") {
            if (g_dump_ptr) nestedAttack_run((CardDump*)g_dump_ptr);
        } else if (a=="darkside") {
            if (g_dump_ptr) {
                for (uint8_t s=0;s<g_dump_ptr->totalSectors;s++) {
                    if (!ks_isSectorKnown(g_dump_ptr, s)) darksideAttack_runSector((CardDump*)g_dump_ptr, s);
                }
            }
        }
        attack_running = false;
        vTaskDelete(NULL);
    }, "attackTask", 8*1024, new String(attackName), 1, NULL);
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
        String body = String();
        if (request->contentLength()>0) {
            AsyncWebParameter* p = request->getParam("plain", true);
            if (p) body = p->value();
        }
        if (body.length()==0) {
            if (request->hasParam("attack", true)) body = String("{\"attack\":\"") + request->getParam("attack", true)->value() + "\"}";
        }
        if (body.length()==0) { request->send(400, "application/json", "{\"error\":\"missing attack\"}"); return; }
        DynamicJsonDocument doc(256);
        DeserializationError err = deserializeJson(doc, body);
        if (err) { request->send(400, "application/json", "{\"error\":\"invalid json\"}"); return; }
        const char* atk = doc["attack"];
        if (!atk) { request->send(400, "application/json", "{\"error\":\"missing attack\"}"); return; }
        _runAttackAsync(String(atk));
        String resp = String("{\"status\":\"queued\",\"attack\":\"") + atk + "\"}";
        request->send(200, "application/json", resp);
    });

    server->on("/api/dump", HTTP_GET, [](AsyncWebServerRequest *request){
        const char* p = dump_lastPath();
        if (!p) { request->send(404, "application/json", "{\"error\":\"no_dump\"}"); return; }
        // stream file from SD
        File f = SD.open(String(p));
        if (!f) { request->send(500, "application/json", "{\"error\":\"file_open_failed\"}"); return; }
        AsyncWebServerResponse *response = request->beginResponseStream("application/octet-stream");
        response->addHeader("Content-Disposition", String("attachment; filename=") + String(p));
        // stream in chunks
        uint8_t buf[256];
        while (f.available()) {
            size_t r = f.read(buf, sizeof(buf));
            response->write(buf, r);
        }
        f.close();
        request->send(response);
    });

    server->begin();
    reporter_register_cb(_onReporter);
}

void webServer_setDump(const CardDump* d) { g_dump_ptr = d; }
