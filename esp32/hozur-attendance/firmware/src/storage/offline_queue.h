#pragma once

#include <Arduino.h>
#include <stdint.h>

struct QueuedScan {
  char uid[21];
  uint32_t user_id;
  uint32_t counter;
  char device_time[32];
};

class OfflineQueue {
 public:
  bool begin();
  bool enqueue(const QueuedScan& scan);
  /** Peek oldest record; false if empty. */
  bool peek(QueuedScan* out) const;
  /** Remove oldest after successful sync. */
  bool pop();
  size_t size() const;
  bool empty() const { return size() == 0; }

 private:
  static constexpr const char* kPath = "/offline_queue.jsonl";
  bool rewriteWithoutFirstLine();
};
