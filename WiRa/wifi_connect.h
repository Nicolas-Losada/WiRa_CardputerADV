#ifndef WIFI_CONNECT_H
#define WIFI_CONNECT_H

#include "wifi_scan.h"
#include <WiFi.h>

#define PASSWORD_MAX_LEN 64
#define CONNECT_TIMEOUT_MS 15000

enum ConnectResult {
  CONN_OK = 0,
  CONN_WRONG_PASSWORD,
  CONN_TIMEOUT,
  CONN_FAILED
};

ConnectResult wifi_connect(const WiFiScanResult& ap, const char* password);
void wifi_disconnect();
bool wifi_is_connected();

#endif
