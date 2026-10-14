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
    if (ws) ws->textAll(json);
}

void webServer_init(void) {
    if (!SPIFFS.begin(true)) {
        reporter_error("SPIFFS_FAIL", "SPIFFS mount failed");
        return;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 8000) {
        delay(200);
    }

    server = new AsyncWebServer(80);
    ws = new AsyncWebSocket("/ws");
    server->addHandler(ws);

    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/data/index.html", "text/html");
    });

    server->on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!g_dump_ptr) {
            request->send(404, "application/json", "{}");
            return;
        }

        DynamicJsonDocument doc(4096);
        char uidhex[32] = {0};
        for (int i = 0; i < g_dump_ptr->uid.len; i++) {
            snprintf(uidhex + (2 * i), 3, "%02X", g_dump_ptr->uid.bytes[i]);
        }
        doc["uid"] = uidhex;
        doc["sectors_total"] = g_dump_ptr->totalSectors;
        doc["cracked"] = ks_countCracked(g_dump_ptr);
        JsonArray sectors = doc.createNestedArray("sectors");
        for (int s = 0; s < g_dump_ptr->totalSectors && s < MAX_SECTORS; s++) {
            JsonObject o = sectors.createNestedObject();
            o["sector"] = s;
            o["keyA_status"] = g_dump_ptr->sectors[s].statusA;
            o["keyB_status"] = g_dump_ptr->sectors[s].statusB;
            char ka[13] = {0}, kb[13] = {0};
            if (g_dump_ptr->sectors[s].statusA == KEY_FOUND) {
                for (int i = 0; i < 6; i++) snprintf(ka + (2 * i), 3, "%02X", g_dump_ptr->sectors[s].keyA[i]);
                o["keyA"] = ka;
            }
            if (g_dump_ptr->sectors[s].statusB == KEY_FOUND) {
                for (int i = 0; i < 6; i++) snprintf(kb + (2 * i), 3, "%02X", g_dump_ptr->sectors[s].keyB[i]);
                o["keyB"] = kb;
            }
            o["data_read"] = g_dump_ptr->sectors[s].dataRead;
        }

        String out;
        serializeJson(doc, out);
        request->send(200, "application/json", out);
    });

    server->on("/api/attack", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (attack_running) {
            request->send(409, "application/json", "{\"error\":\"attack_running\"}");
            return;
        }

        String body = "";
        if (request->hasParam("plain", true)) {
            body = request->getParam("plain", true)->value();
        }
        if (body.length() == 0 && request->hasParam("attack", true)) {
            body = String("{\"attack\":\"") + request->getParam("attack", true)->value() + "\"}";
        }
        if (body.length() == 0) {
            request->send(400, "application/json", "{\"error\":\"missing_body\"}");
            return;
        }

        DynamicJsonDocument doc(256);
        deserializeJson(doc, body);
        const char* atk = doc["attack"];
        if (!atk) {
            request->send(400, "application/json", "{\"error\":\"missing_attack\"}");
            return;
        }

        attack_running = true;
        String a = String(atk);
        xTaskCreate([](void* p) {
            String attackName = *((String*)p);
            delete (String*)p;
            if (attackName == "dictionary" && g_dump_ptr) dictAttack_run((CardDump*)g_dump_ptr);
            else if (attackName == "nested" && g_dump_ptr) nestedAttack_run((CardDump*)g_dump_ptr);
            else if (attackName == "darkside" && g_dump_ptr) {
                for (uint8_t s = 0; s < g_dump_ptr->totalSectors; s++) {
                    if (!ks_isSectorKnown(g_dump_ptr, s)) darksideAttack_runSector((CardDump*)g_dump_ptr, s);
                }
            }
            attack_running = false;
            vTaskDelete(NULL);
        }, "attack_task", 8 * 1024, new String(a), 1, NULL);

        request->send(200, "application/json", String("{\"status\":\"queued\",\"attack\":\"") + atk + "\"}");
    });

    server->on("/api/dump", HTTP_GET, [](AsyncWebServerRequest *request) {
        const char* p = dump_lastPath();
        if (!p) {
            request->send(404, "application/json", "{\"error\":\"no_dump\"}");
            return;
        }
        File f = SD.open(p);
        if (!f) {
            request->send(500, "application/json", "{\"error\":\"file_open_failed\"}");
            return;
        }
        AsyncWebServerResponse* response = request->beginResponseStream("application/octet-stream");
        response->addHeader("Content-Disposition", String("attachment; filename=") + String(p));
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
