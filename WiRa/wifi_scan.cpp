// =====================================================================
// wifi_scan.cpp - Escaneo WiFi y medicion RSSI
// =====================================================================
#include "wifi_scan.h"
#include <Arduino.h>
#include <M5GFX.h>

uint16_t wifi_scan_networks(WiFiScanResult results[], uint16_t max_results) {
  if (!results || max_results == 0) return 0;

  // show_hidden=true para captar hotspots
  // 600ms/canal para mejor deteccion
  int networks_found = WiFi.scanNetworks(false, true, false, 600U);
  if (networks_found <= 0) return 0;

  uint16_t count = (networks_found > max_results) ? max_results : networks_found;

  for (uint16_t i = 0; i < count; i++) {
    // strlcpy: sin alocacion dinamica, garantiza null-terminator
    strlcpy(results[i].ssid, WiFi.SSID(i).c_str(), SSID_MAX_LEN);

    uint8_t* bssid = WiFi.BSSID(i);
    if (bssid) memcpy(results[i].bssid, bssid, BSSID_LEN);

    results[i].rssi = WiFi.RSSI(i);
    results[i].channel = WiFi.channel(i);

    wifi_auth_mode_t auth = WiFi.encryptionType(i);
    results[i].is_open = (auth == WIFI_AUTH_OPEN);
    results[i].auth_type = auth;

    switch (auth) {
      case WIFI_AUTH_OPEN:           strlcpy(results[i].auth_label, "OPEN", 6); break;
      case WIFI_AUTH_WEP:            strlcpy(results[i].auth_label, "WEP",  6); break;
      case WIFI_AUTH_WPA_PSK:        strlcpy(results[i].auth_label, "WPA",  6); break;
      case WIFI_AUTH_WPA2_PSK:       strlcpy(results[i].auth_label, "WPA2", 6); break;
      case WIFI_AUTH_WPA_WPA2_PSK:   strlcpy(results[i].auth_label, "WPA*", 6); break;
      case WIFI_AUTH_WPA3_PSK:       strlcpy(results[i].auth_label, "WPA3", 6); break;
      case WIFI_AUTH_WPA2_WPA3_PSK:  strlcpy(results[i].auth_label, "W2/3", 6); break;
      case WIFI_AUTH_WPA2_ENTERPRISE: strlcpy(results[i].auth_label, "ENT", 6); break;
      default:                       strlcpy(results[i].auth_label, "????", 6); break;
    }
  }

  WiFi.scanDelete();
  return count;
}

int16_t wifi_measure_rssi(const char* ssid, const uint8_t* bssid, uint8_t channel) {
  if (!ssid || !bssid) return -99;

  // FIX BUG: scan solo el canal del AP -> ~100ms en vez de ~1300ms
  int networks = WiFi.scanNetworks(
    false,    // async
    true,     // show_hidden
    false,    // passive
    100U,     // max_ms por canal
    channel   // canal especifico (0=todos)
  );

  int16_t measured_rssi = -99;
  if (networks > 0) {
    for (int i = 0; i < networks; i++) {
      uint8_t* found_bssid = WiFi.BSSID(i);
      if (found_bssid && memcmp(found_bssid, bssid, BSSID_LEN) == 0) {
        measured_rssi = WiFi.RSSI(i);
        break;
      }
    }
  }

  WiFi.scanDelete();
  return measured_rssi;
}

const char* wifi_get_quality_string(int16_t rssi) {
  if (rssi >= -67) return "Excelente";
  else if (rssi >= -70) return "Bueno";
  else if (rssi >= -80) return "Aceptable";
  else if (rssi >= -90) return "Debil";
  else return "Muy debil";
}

uint32_t wifi_get_quality_color(int16_t rssi) {
  if (rssi >= -70) return TFT_GREEN;
  else if (rssi >= -80) return TFT_YELLOW;
  else if (rssi >= -90) return TFT_ORANGE;
  else return TFT_RED;
}

uint8_t wifi_rssi_to_percent(int16_t rssi) {
  if (rssi >= -30) return 100;
  if (rssi <= -90) return 0;
  int16_t offset = rssi + 90;
  uint8_t percent = (offset * 100) / 60;
  return (percent > 100) ? 100 : percent;
}
