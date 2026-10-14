#include "web_server.h"
#include "output/serial_reporter.h"
#include "config.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <string.h>
#include <stdio.h>

static AsyncWebServer  _srv(80);
static AsyncWebSocket  _ws("/ws");
static const CardDump* _dump = nullptr;

// ─── WebSocket event handler ────────────────────────────────────────────────
static void _wsEvent(AsyncWebSocket* ws, AsyncWebSocketClient* client,
                     AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    if (type == WS_EVT_CONNECT) {
        // Send current dump state as a snapshot on connect
        if (_dump) {
            char buf[128];
            snprintf(buf, sizeof(buf),
                "{\"event\":\"snapshot\",\"data\":{\"cracked\":%d,\"total\":%d,\"uid_len\":%d}}",
                ks_countCracked(_dump), _dump->totalSectors, _dump->uid.len);
            client->text(buf);
        }
    }
    // WS_EVT_DISCONNECT, WS_EVT_DATA handled automatically
    (void)arg; (void)data; (void)len;
}

// ─── Broadcast (called by reporter observer) ────────────────────────────────
void webServer_broadcastEvent(const char* jsonLine) {
    if (_ws.count() > 0) _ws.textAll(jsonLine);
}

void webServer_setDump(const CardDump* dump) { _dump = dump; }

// ─── Status serializer ─────────────────────────────────────────────────────
static String _buildStatus() {
    if (!_dump) return "{\"status\":\"idle\"}";
    char uid[21] = "N/A";
    if (_dump->uid.len > 0) {
        const char* H = "0123456789ABCDEF";
        for (uint8_t i = 0; i < _dump->uid.len && i < 10; i++) {
            uid[i*2]   = H[(_dump->uid.bytes[i]>>4)&0xF];
            uid[i*2+1] = H[ _dump->uid.bytes[i]    &0xF];
        }
        uid[_dump->uid.len*2] = '\0';
    }
    char buf[512];
    snprintf(buf, sizeof(buf),
        "{\"uid\":\"%s\",\"type\":\"%s\",\"sectors\":%d,\"cracked\":%d}",
        uid,
        _dump->cardType == 4 ? "MIFARE_4K" : "MIFARE_1K",
        _dump->totalSectors,
        ks_countCracked(_dump));
    return String(buf);
}

// ─── Init ───────────────────────────────────────────────────────────────────
void webServer_init(void) {
    // WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    for (uint8_t i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) delay(500);

    if (WiFi.status() != WL_CONNECTED) {
        reporter_error("WIFI_FAIL", "Cannot connect to WiFi — web dashboard unavailable");
        return;
    }
    {
        char msg[64];
        snprintf(msg, sizeof(msg), "http://%s", WiFi.localIP().toString().c_str());
        reporter_error("WIFI_OK", msg);
    }

    // SPIFFS
    if (!SPIFFS.begin(true)) {
        reporter_error("SPIFFS_FAIL", "SPIFFS mount failed");
        return;
    }

    // WebSocket
    _ws.onEvent(_wsEvent);
    _srv.addHandler(&_ws);

    // Static files from SPIFFS /data/
    _srv.serveStatic("/", SPIFFS, "/data/").setDefaultFile("index.html");

    // GET /api/status — current dump snapshot
    _srv.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/json", _buildStatus());
    });

    // GET /api/dump — download latest .mfd file
    _srv.on("/api/dump", HTTP_GET, [](AsyncWebServerRequest* req) {
        if (SPIFFS.exists("/dump_latest.mfd")) {
            req->send(SPIFFS, "/dump_latest.mfd", "application/octet-stream",
                      true /* download */);
        } else {
            req->send(404, "text/plain", "No dump available yet");
        }
    });

    // POST /api/attack — trigger attack from dashboard
    // Body: {"attack":"dictionary"|"nested"|"darkside"}
    _srv.on("/api/attack", HTTP_POST,
        [](AsyncWebServerRequest* req) {
            req->send(200, "application/json", "{\"queued\":true}");
        },
        nullptr,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
            // Parse attack type from body (simple substring match — no JSON lib needed)
            char body[64] = {};
            memcpy(body, data, len < 63 ? len : 63);
            reporter_error("ATTACK_REQ", body);  // re-emit as event so observer routes it
        }
    );

    // POST /api/wifi — update credentials at runtime
    _srv.on("/api/wifi", HTTP_POST,
        [](AsyncWebServerRequest* req) {
            req->send(200, "application/json", "{\"ok\":true,\"msg\":\"Reconnecting...\"}");
        },
        nullptr,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
            // In a full implementation: parse SSID/pass, call WiFi.begin(), update NVS
            reporter_error("WIFI_CHANGE", "Runtime WiFi change requested (not yet implemented)");
        }
    );

    // 404 handler
    _srv.onNotFound([](AsyncWebServerRequest* req) {
        req->send(404, "application/json", "{\"error\":\"not_found\"}");
    });

    _srv.begin();
    // Register ourselves as an observer so every JSON event is broadcast
    reporter_addObserver(webServer_broadcastEvent);
}
