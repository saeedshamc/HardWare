#include "output/web_server.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <SD.h>
#include "config.h"
#include "output/serial_reporter.h"
#include "storage/dump_manager.h"
#include "attacks/dictionary_attack.h"
#include "attacks/nested_attack.h"
#include "attacks/darkside_attack.h"
#include "core/key_store.h"

static AsyncWebServer* server = nullptr;
static AsyncWebSocket* ws = nullptr;
static const CardDump* g_dump_ptr = nullptr;
static bool attack_running = false;
static Preferences g_wifiPrefs;
static String g_wifiSsid;
static String g_wifiPass;

static void _onReporter(const char* json) {
    if (ws) ws->textAll(json);
}

static void _loadWifiCreds(void) {
    g_wifiSsid = WIFI_SSID;
    g_wifiPass = WIFI_PASS;
    if (g_wifiPrefs.begin(WIFI_NVS_NAMESPACE, true)) {
        String s = g_wifiPrefs.getString("ssid", "");
        String p = g_wifiPrefs.getString("pass", "");
        g_wifiPrefs.end();
        if (s.length() > 0) {
            g_wifiSsid = s;
            g_wifiPass = p;
        }
    }
}

static bool _saveWifiCreds(const String& ssid, const String& pass) {
    if (!g_wifiPrefs.begin(WIFI_NVS_NAMESPACE, false)) return false;
    g_wifiPrefs.putString("ssid", ssid);
    g_wifiPrefs.putString("pass", pass);
    g_wifiPrefs.end();
    g_wifiSsid = ssid;
    g_wifiPass = pass;
    return true;
}

static void _connectWifi(uint32_t timeoutMs) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(g_wifiSsid.c_str(), g_wifiPass.c_str());
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < timeoutMs) {
        delay(200);
    }
}

void webServer_init(void) {
    if (!SPIFFS.begin(true)) {
        reporter_error("SPIFFS_FAIL", "SPIFFS mount failed");
        return;
    }

    _loadWifiCreds();
    _connectWifi(8000);

    server = new AsyncWebServer(80);
    ws = new AsyncWebSocket("/ws");
    server->addHandler(ws);

    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (SPIFFS.exists("/index.html"))
            request->send(SPIFFS, "/index.html", "text/html");
        else
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
        doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
        doc["ip"] = WiFi.localIP().toString();
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

    server->on("/api/wifi", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            static String body;
            if (index == 0) body = "";
            for (size_t i = 0; i < len; i++) body += (char)data[i];
            if (index + len < total) return;

            DynamicJsonDocument doc(512);
            if (deserializeJson(doc, body)) {
                request->send(400, "application/json", "{\"error\":\"bad_json\"}");
                body = "";
                return;
            }
            const char* ssid = doc["ssid"];
            const char* pass = doc["pass"] | "";
            if (!ssid || strlen(ssid) == 0) {
                request->send(400, "application/json", "{\"error\":\"missing_ssid\"}");
                body = "";
                return;
            }

            if (!_saveWifiCreds(String(ssid), String(pass))) {
                request->send(500, "application/json", "{\"error\":\"nvs_fail\"}");
                body = "";
                return;
            }

            request->send(200, "application/json", "{\"status\":\"saved\",\"note\":\"reconnecting\"}");
            body = "";

            // Reconnect after response is queued
            xTaskCreate([](void*) {
                delay(300);
                WiFi.disconnect(true);
                delay(200);
                _connectWifi(12000);
                if (WiFi.status() == WL_CONNECTED)
                    reporter_error("WIFI_OK", WiFi.localIP().toString().c_str());
                else
                    reporter_error("WIFI_FAIL", "reconnect failed");
                vTaskDelete(NULL);
            }, "wifi_reconn", 4096, nullptr, 1, nullptr);
        });

    server->on("/api/attack", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            static String body;
            if (index == 0) body = "";
            for (size_t i = 0; i < len; i++) body += (char)data[i];
            if (index + len < total) return;

            if (attack_running) {
                request->send(409, "application/json", "{\"error\":\"attack_running\"}");
                body = "";
                return;
            }
            if (body.length() == 0) {
                request->send(400, "application/json", "{\"error\":\"missing_body\"}");
                body = "";
                return;
            }

            DynamicJsonDocument doc(256);
            deserializeJson(doc, body);
            const char* atk = doc["attack"];
            body = "";
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

String webServer_ipOrStatus(void) {
    if (WiFi.status() == WL_CONNECTED) return WiFi.localIP().toString();
    return String("WiFi offline");
}
