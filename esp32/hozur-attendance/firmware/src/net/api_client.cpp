#include "api_client.h"

#include "config.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <mbedtls/md.h>
#include <mbedtls/base64.h>
#include <esp_random.h>
#include <time.h>
#include <string.h>

void ApiClient::begin(const char* baseUrl, int readerId, const char* apiKey) {
  baseUrl_ = baseUrl ? baseUrl : "";
  while (baseUrl_.endsWith("/")) {
    baseUrl_.remove(baseUrl_.length() - 1);
  }
  readerId_ = readerId;
  apiKey_ = apiKey ? apiKey : "";
}

String ApiClient::makeNonce() {
  char buf[33];
  for (int i = 0; i < 16; ++i) {
    sprintf(buf + i * 2, "%02x", (unsigned)(esp_random() & 0xFF));
  }
  buf[32] = '\0';
  return String(buf);
}

String ApiClient::hmacHex(const String& key, const String& msg) {
  uint8_t digest[32];
  const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (!info) return String();
  mbedtls_md_hmac(info,
                  reinterpret_cast<const uint8_t*>(key.c_str()), key.length(),
                  reinterpret_cast<const uint8_t*>(msg.c_str()), msg.length(),
                  digest);
  char hex[65];
  for (int i = 0; i < 32; ++i) {
    sprintf(hex + i * 2, "%02x", digest[i]);
  }
  hex[64] = '\0';
  return String(hex);
}

bool ApiClient::signedRequest(const char* method, const char* path, const String& body,
                              int* httpCode, String* responseBody) {
  if (httpCode) *httpCode = 0;
  if (responseBody) responseBody->clear();
  if (WiFi.status() != WL_CONNECTED || baseUrl_.isEmpty()) {
    return false;
  }

  const String ts = String((uint32_t)time(nullptr));
  const String nonce = makeNonce();
  const String toSign = ts + "." + nonce + "." + body;
  const String sig = hmacHex(apiKey_, toSign);

  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  const String url = baseUrl_ + path;
  if (!http.begin(url)) {
    return false;
  }
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Reader-Id", String(readerId_));
  http.addHeader("X-Api-Key", apiKey_);
  http.addHeader("X-Timestamp", ts);
  http.addHeader("X-Nonce", nonce);
  http.addHeader("X-Signature", sig);

  int code = 0;
  if (strcmp(method, "GET") == 0) {
    code = http.GET();
  } else {
    code = http.POST(body);
  }
  if (httpCode) *httpCode = code;
  if (responseBody) *responseBody = http.getString();
  http.end();
  return code > 0;
}

void ApiClient::freeNameBitmap(ApiScanResponse* resp) {
  if (!resp) return;
  if (resp->nameRgb565) {
    free(resp->nameRgb565);
    resp->nameRgb565 = nullptr;
  }
  resp->nameW = resp->nameH = 0;
}

static bool decodeRgb565Base64(const char* b64, uint16_t w, uint16_t h, ApiScanResponse* out) {
  if (!b64 || !out || w == 0 || h == 0) return false;
  const size_t need = (size_t)w * h * 2;
  size_t olen = 0;
  mbedtls_base64_decode(nullptr, 0, &olen,
                        reinterpret_cast<const uint8_t*>(b64), strlen(b64));
  if (olen < need) return false;
  auto* buf = static_cast<uint8_t*>(malloc(olen));
  if (!buf) return false;
  if (mbedtls_base64_decode(buf, olen, &olen,
                            reinterpret_cast<const uint8_t*>(b64), strlen(b64)) != 0) {
    free(buf);
    return false;
  }
  out->nameRgb565 = reinterpret_cast<uint16_t*>(buf);
  out->nameW = w;
  out->nameH = h;
  return true;
}

bool ApiClient::postScan(const ApiScanRequest& req, ApiScanResponse* out) {
  if (!out) return false;
  memset(out, 0, sizeof(*out));
  strlcpy(out->status, "denied", sizeof(out->status));

  JsonDocument doc;
  doc["reader_id"] = readerId_;
  doc["uid"] = req.uid ? req.uid : "";
  doc["user_id"] = req.user_id;
  doc["counter"] = req.counter;
  doc["device_time"] = req.device_time ? req.device_time : "";
  String body;
  serializeJson(doc, body);

  int code = 0;
  String resp;
  if (!signedRequest("POST", "/api/scan/", body, &code, &resp)) {
    out->httpOk = false;
    return false;
  }
  out->httpOk = (code >= 200 && code < 300);

  JsonDocument parsed;
  if (deserializeJson(parsed, resp)) {
    return out->httpOk;
  }
  strlcpy(out->status, parsed["status"] | "denied", sizeof(out->status));
  strlcpy(out->nameLatin, parsed["name_latin"] | parsed["name"] | "", sizeof(out->nameLatin));
  strlcpy(out->department, parsed["department_or_class"] | "", sizeof(out->department));
  strlcpy(out->direction, parsed["direction"] | "", sizeof(out->direction));
  strlcpy(out->timeStr, parsed["time"] | "", sizeof(out->timeStr));
  strlcpy(out->dateJalali, parsed["date_jalali"] | "", sizeof(out->dateJalali));
  strlcpy(out->message, parsed["message"] | "", sizeof(out->message));

  if (parsed["display_image"].is<JsonObject>()) {
    JsonObject img = parsed["display_image"];
    const char* fmt = img["format"] | "";
    const uint16_t w = img["width"] | 0;
    const uint16_t h = img["height"] | 0;
    const char* b64 = img["data_base64"] | "";
    if (strcmp(fmt, "rgb565") == 0 && b64[0]) {
      decodeRgb565Base64(b64, w, h, out);
    }
  }
  return out->httpOk;
}

bool ApiClient::postEnroll(const char* uid, uint32_t userId) {
  JsonDocument doc;
  doc["reader_id"] = readerId_;
  doc["uid"] = uid ? uid : "";
  doc["user_id"] = userId;
  String body;
  serializeJson(doc, body);
  int code = 0;
  String resp;
  if (!signedRequest("POST", "/api/enroll/", body, &code, &resp)) {
    return false;
  }
  return code >= 200 && code < 300;
}

bool ApiClient::postOfflineSync(const String& scansJsonArray) {
  String body = "{\"reader_id\":";
  body += readerId_;
  body += ",\"scans\":";
  body += scansJsonArray;
  body += "}";
  int code = 0;
  String resp;
  if (!signedRequest("POST", "/api/offline-sync/", body, &code, &resp)) {
    return false;
  }
  return code >= 200 && code < 300;
}

bool ApiClient::getSyncOk() {
  int code = 0;
  String resp;
  if (!signedRequest("GET", "/api/sync/", "", &code, &resp)) {
    return false;
  }
  return code >= 200 && code < 300;
}

void ApiClient::fillUiData(const ApiScanResponse& resp, ScanUiData* ui) const {
  if (!ui) return;
  memset(ui, 0, sizeof(*ui));
  ui->nameLatin = resp.nameLatin;
  ui->department = resp.department;
  ui->direction = resp.direction;
  ui->timeStr = resp.timeStr;
  ui->dateJalali = resp.dateJalali;
  ui->nameBitmapRgb565 = resp.nameRgb565;
  ui->nameBmpW = resp.nameW;
  ui->nameBmpH = resp.nameH;
}
