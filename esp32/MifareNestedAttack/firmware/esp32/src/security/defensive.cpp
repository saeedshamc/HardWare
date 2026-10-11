#include "defensive.h"

#include <Arduino.h>

void defensive_analysis_print() {
  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("        DEFENSIVE SECURITY MODULE"));
  Serial.println(F("========================================"));
  Serial.println();

  Serial.println(F("Legacy Architecture"));
  Serial.println(F("────────────────────"));
  Serial.println(F("MIFARE Classic"));
  Serial.println(F("      |"));
  Serial.println(F("      v"));
  Serial.println(F("   Crypto1"));
  Serial.println(F("      |"));
  Serial.println(F("      v"));
  Serial.println(F("Access Control  <<-- weak if Crypto1 is the only gate"));
  Serial.println();

  Serial.println(F("Modern Architecture"));
  Serial.println(F("────────────────────"));
  Serial.println(F("Secure Card (e.g. DESFire / MIFARE Plus)"));
  Serial.println(F("      |"));
  Serial.println(F("      v"));
  Serial.println(F("Modern Authentication"));
  Serial.println(F("      |"));
  Serial.println(F("      v"));
  Serial.println(F("Session Security"));
  Serial.println(F("      |"));
  Serial.println(F("      v"));
  Serial.println(F("Backend Authorization"));
  Serial.println();

  Serial.println(F("Practical defenses for classroom/IoT labs:"));
  Serial.println(F("  - Do not trust UID alone (UIDs can be cloned)."));
  Serial.println(F("  - Prefer diversified keys + server-side validation."));
  Serial.println(F("  - Use mutual auth + challenge/response with modern crypto."));
  Serial.println(F("  - Keep secrets off the card when possible."));
  Serial.println(F("  - Log denials; rate-limit readers; detect counter rollback."));
  Serial.println(F("  - For security-sensitive access, follow NXP guidance:"));
  Serial.println(F("    migrate away from Classic Crypto1 to DESFire/Plus."));
  Serial.println();
  Serial.println(F("Attack demo always ends with: why it worked + how to prevent it."));
}
