// =====================================================================
// wifi_connect.cpp - Conexion robusta a AP
// - BSSID + canal dirigido (rapido en redes con multiples BSS)
// - Reintento sin BSSID si falla primer intento
// - Espera DHCP completo
// =====================================================================
#include "wifi_connect.h"
#include <Arduino.h>

static const char* status_str(wl_status_t st) {
  switch (st) {
    case WL_IDLE_STATUS:     return "IDLE";
    case WL_NO_SSID_AVAIL:   return "NO_SSID";
    case WL_SCAN_COMPLETED:  return "SCAN_OK";
    case WL_CONNECTED:       return "CONNECTED";
    case WL_CONNECT_FAILED:  return "FAIL";
    case WL_CONNECTION_LOST: return "LOST";
    case WL_DISCONNECTED:    return "DISC";
    default:                 return "?";
  }
}

static ConnectResult attempt(const char* ssid, const char* pwd,
                              uint8_t channel, const uint8_t* bssid,
                              uint32_t timeout_ms) {
  if (bssid) {
    WiFi.begin(ssid, pwd, channel, bssid, true);
  } else {
    WiFi.begin(ssid, pwd);
  }

  uint32_t t0 = millis();
  wl_status_t last = WL_IDLE_STATUS;

  while (millis() - t0 < timeout_ms) {
    wl_status_t st = WiFi.status();
    if (st != last) {
      Serial.printf("[CONN] %s @ %lums\n", status_str(st), (unsigned long)(millis() - t0));
      last = st;
    }

    if (st == WL_CONNECTED) {
      IPAddress ip = WiFi.localIP();
      if (ip != IPAddress(0,0,0,0)) {
        Serial.printf("[CONN] OK IP=%s\n", ip.toString().c_str());
        delay(200); // estabilizar
        return CONN_OK;
      }
    }
    if (st == WL_NO_SSID_AVAIL) return CONN_FAILED;
    if (st == WL_CONNECT_FAILED) return CONN_WRONG_PASSWORD;

    delay(150);
  }
  return CONN_TIMEOUT;
}

ConnectResult wifi_connect(const WiFiScanResult& ap, const char* password) {
  Serial.printf("[CONN] -> %s CH%d %s\n", ap.ssid, ap.channel, ap.auth_label);

  WiFi.disconnect(true, true);
  delay(300);
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);

  // Intento 1: dirigido (BSSID + canal)
  ConnectResult res = attempt(ap.ssid, password, ap.channel, ap.bssid, CONNECT_TIMEOUT_MS);
  if (res == CONN_OK) return res;
  if (res == CONN_WRONG_PASSWORD) return res;  // no reintento

  // Intento 2: sin BSSID dirigido
  Serial.println("[CONN] Reintento sin BSSID dirigido...");
  WiFi.disconnect(true, true);
  delay(500);
  WiFi.mode(WIFI_STA);
  res = attempt(ap.ssid, password, 0, nullptr, CONNECT_TIMEOUT_MS);
  return res;
}

void wifi_disconnect() {
  WiFi.disconnect(true);
  delay(100);
}

bool wifi_is_connected() {
  return WiFi.status() == WL_CONNECTED &&
         WiFi.localIP() != IPAddress(0,0,0,0);
}
