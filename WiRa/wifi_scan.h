#ifndef WIFI_SCAN_H
#define WIFI_SCAN_H

#include <WiFi.h>
#include <cstring>

#define MAX_NETWORKS 50
#define SSID_MAX_LEN 32
#define BSSID_LEN 6

struct WiFiScanResult {
  char ssid[SSID_MAX_LEN];
  uint8_t bssid[BSSID_LEN];
  int16_t rssi;
  uint8_t channel;
  bool is_open;
  wifi_auth_mode_t auth_type;
  char auth_label[6];

  WiFiScanResult() : rssi(0), channel(0), is_open(false), auth_type(WIFI_AUTH_OPEN) {
    memset(ssid, 0, SSID_MAX_LEN);
    memset(bssid, 0, BSSID_LEN);
    memset(auth_label, 0, sizeof(auth_label));
  }
};

uint16_t wifi_scan_networks(WiFiScanResult results[], uint16_t max_results);
int16_t wifi_measure_rssi(const char* ssid, const uint8_t* bssid, uint8_t channel);
const char* wifi_get_quality_string(int16_t rssi);
uint32_t wifi_get_quality_color(int16_t rssi);
uint8_t wifi_rssi_to_percent(int16_t rssi);

#endif
