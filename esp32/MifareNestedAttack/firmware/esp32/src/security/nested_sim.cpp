#include "nested_sim.h"

#include <Arduino.h>
#include <esp_random.h>

/*
 * EDUCATIONAL ONLY
 * ----------------
 * This module visualizes why Nested Attacks were historically dangerous
 * against MIFARE Classic Crypto1. It does NOT recover real sector keys,
 * does NOT implement darkside/nested key cracking, and is not a Proxmark3
 * substitute.
 */

static uint32_t g_demoNonce = 0;
static bool g_authLoaded = false;
static bool g_stateReady = false;

static uint32_t fakeNonce() {
  return esp_random();
}

static void step_load_known_auth() {
  Serial.println(F("\n[1] Load known authentication"));
  Serial.println(F("    (classroom: assume Sector 0 KeyA is already known)"));
  Serial.println(F("    Known Key ----> Authentication #1 ----> Crypto session"));
  g_authLoaded = true;
  g_stateReady = false;
  Serial.println(F("[+] Known authentication context loaded (simulated)"));
}

static void step_generate_nonce() {
  Serial.println(F("\n[2] Generate card nonce"));
  g_demoNonce = fakeNonce();
  Serial.printf("    Card nonce nT (demo): %08lX\n", (unsigned long)g_demoNonce);
  Serial.println(F("    Real cards also emit nonces during auth."));
}

static void step_establish_state() {
  Serial.println(F("\n[3] Establish Crypto1 state"));
  if (!g_authLoaded) {
    Serial.println(F("[-] Load known authentication first"));
    return;
  }
  Serial.println(F("    Auth #1 mixes key + nonces into internal cipher state."));
  Serial.println(F("    That state is meant to stay secret."));
  g_stateReady = true;
  Serial.println(F("[+] Crypto1 state established (conceptual)"));
}

static void step_nested_auth() {
  Serial.println(F("\n[4] Start nested authentication"));
  if (!g_stateReady) {
    Serial.println(F("[-] Establish Crypto1 state first"));
    return;
  }
  Serial.println(F("    While still in session #1, start Authentication #2"));
  Serial.println(F("    against another sector (nested)."));
  const uint32_t n2 = fakeNonce();
  Serial.printf("    Nested nonce (demo): %08lX\n", (unsigned long)n2);
  Serial.println(F("    Conceptually, session #2 is no longer independent."));
}

static void step_analyze_leak() {
  Serial.println(F("\n[5] Analyze leaked information (concept)"));
  Serial.println(F("    Historical weakness (simplified):"));
  Serial.println(F("      Auth #1  -->  Crypto State"));
  Serial.println(F("                      |"));
  Serial.println(F("                      | relationship / PRNG weakness"));
  Serial.println(F("                      v"));
  Serial.println(F("      Auth #2  -->  Information Leakage"));
  Serial.println(F("                      |"));
  Serial.println(F("                      v"));
  Serial.println(F("              Candidate key space shrinks"));
  Serial.println();
  Serial.println(F("    This lab stops at the concept. It does not crack keys."));
}

static void step_visualize() {
  Serial.println(F("\n[6] Visualize attack concept"));
  Serial.println(F("Known Authentication"));
  Serial.println(F("        |"));
  Serial.println(F("        v"));
  Serial.println(F("Authenticated Session"));
  Serial.println(F("        |"));
  Serial.println(F("        v"));
  Serial.println(F("Crypto1 State Relationship"));
  Serial.println(F("        |"));
  Serial.println(F("        v"));
  Serial.println(F("Second Authentication"));
  Serial.println(F("        |"));
  Serial.println(F("        v"));
  Serial.println(F("Information Leakage"));
  Serial.println();
  Serial.println(F("Lesson: \"We use encryption\" != \"the system is secure\"."));
}

void nested_simulator_menu() {
  while (true) {
    Serial.println();
    Serial.println(F("========================================"));
    Serial.println(F("       NESTED ATTACK SIMULATOR"));
    Serial.println(F("========================================"));
    Serial.println(F("Educational model — no real key recovery"));
    Serial.println();
    Serial.println(F("[1] Load known authentication"));
    Serial.println(F("[2] Generate card nonce"));
    Serial.println(F("[3] Establish Crypto1 state"));
    Serial.println(F("[4] Start nested authentication"));
    Serial.println(F("[5] Analyze leaked information"));
    Serial.println(F("[6] Visualize attack concept"));
    Serial.println(F("[0] Back"));
    Serial.print(F("\nSelect: "));

    while (!Serial.available()) {
      delay(10);
    }
    const int choice = Serial.parseInt();
    while (Serial.available()) Serial.read();

    switch (choice) {
      case 1: step_load_known_auth(); break;
      case 2: step_generate_nonce(); break;
      case 3: step_establish_state(); break;
      case 4: step_nested_auth(); break;
      case 5: step_analyze_leak(); break;
      case 6: step_visualize(); break;
      case 0: return;
      default: Serial.println(F("Unknown option")); break;
    }
  }
}
