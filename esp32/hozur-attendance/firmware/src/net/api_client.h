#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "../ui/ui_screens.h"

struct ApiScanRequest {
  const char* uid;
  uint32_t user_id;
  uint32_t counter;
  const char* device_time;
};

struct ApiScanResponse {
  bool httpOk;
  char status[16];       // ok | denied | duplicate
  char nameLatin[64];
  char department[64];
  char direction[8];
  char timeStr[16];
  char dateJalali[16];
  char message[64];
  // optional decoded RGB565 name bitmap (heap); caller frees with freeNameBitmap()
  uint16_t* nameRgb565;
  uint16_t nameW;
  uint16_t nameH;
};

class ApiClient {
 public:
  void begin(const char* baseUrl, int readerId, const char* apiKey);

  bool postScan(const ApiScanRequest& req, ApiScanResponse* out);
  bool postEnroll(const char* uid, uint32_t userId);
  bool postOfflineSync(const String& scansJsonArray);
  bool getSyncOk();  // lightweight connectivity / auth check via GET /api/sync/

  static void freeNameBitmap(ApiScanResponse* resp);
  void fillUiData(const ApiScanResponse& resp, ScanUiData* ui) const;

 private:
  String baseUrl_;
  int readerId_ = 1;
  String apiKey_;

  bool signedRequest(const char* method, const char* path, const String& body,
                     int* httpCode, String* responseBody);
  static String makeNonce();
  static String hmacHex(const String& key, const String& msg);
};
