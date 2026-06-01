// =====================================================================
// storage.cpp - Logging a microSD del Cardputer ADV
// Pinout SD del Cardputer ADV:
//   CS=12, MOSI=14, MISO=39, SCK=40
// =====================================================================
#include "storage.h"
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>

// Pinout SD del M5Stack Cardputer ADV
// Fuente: docs.m5stack.com/en/core/Cardputer-Adv
//   CS=GPIO5, MOSI=GPIO14, MISO=GPIO39, CLK=GPIO40
#define SD_CS    5
#define SD_MOSI  14
#define SD_MISO  39
#define SD_SCK   40

#define DIR_PROFILES "/wira"
#define CSV_PATH "/wira/log.csv"

static bool s_sd_ok = false;

bool storage_init() {
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, SPI, 25000000)) {
    s_sd_ok = false;
    Serial.println("[SD] No detectada");
    return false;
  }
  s_sd_ok = true;
  Serial.println("[SD] OK");

  if (!SD.exists(DIR_PROFILES)) {
    SD.mkdir(DIR_PROFILES);
  }
  if (!SD.exists(CSV_PATH)) {
    File f = SD.open(CSV_PATH, FILE_WRITE);
    if (f) {
      f.println("ts_ms,ssid,bssid,verdict,confidence,public_ip,asn,is_mobile,cgnat,ping_ms,subnet,hotspot");
      f.close();
    }
  }
  return true;
}

bool storage_available() { return s_sd_ok; }

bool storage_save_profile(const ConnectionProfile& p) {
  if (!s_sd_ok) return false;

  char fname[40];
  snprintf(fname, sizeof(fname), "%s/p_%lu.json",
           DIR_PROFILES, (unsigned long)(p.timestamp_ms));

  File f = SD.open(fname, FILE_WRITE);
  if (!f) return false;

  StaticJsonDocument<1024> doc;
  doc["ts_ms"] = p.timestamp_ms;
  doc["verdict"] = p.verdict_label;
  doc["confidence"] = p.confidence;

  JsonObject loc = doc.createNestedObject("local");
  loc["ssid"] = p.local.ssid_name;
  loc["ip"] = p.local.local_ip;
  loc["gateway"] = p.local.gateway_ip;
  loc["mask"] = p.local.subnet_mask;
  loc["prefix"] = p.local.subnet_prefix;
  loc["dns1"] = p.local.dns1;
  loc["dns2"] = p.local.dns2;
  loc["hotspot_hint"] = p.local.hotspot_hint;
  loc["is_likely_mobile"] = p.local.is_likely_mobile_hotspot;

  JsonObject inet = doc.createNestedObject("internet");
  inet["api_ok"] = p.internet.api_ok;
  inet["public_ip"] = p.internet.public_ip;
  inet["asn"] = p.internet.asn;
  inet["org"] = p.internet.org;
  inet["country"] = p.internet.country;
  inet["city"] = p.internet.city;
  inet["is_mobile"] = p.internet.is_mobile;
  inet["isp_type"] = p.internet.isp_type;
  inet["cgnat"] = p.internet.cgnat_detected;
  inet["ping_ms"] = p.internet.ping_ms;
  inet["jitter_ms"] = p.internet.ping_jitter;

  serializeJsonPretty(doc, f);
  f.close();
  return true;
}

bool storage_append_csv(const ConnectionProfile& p) {
  if (!s_sd_ok) return false;
  File f = SD.open(CSV_PATH, FILE_APPEND);
  if (!f) return false;

  char bssid_str[18];
  snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X",
           p.local.bssid[0], p.local.bssid[1], p.local.bssid[2],
           p.local.bssid[3], p.local.bssid[4], p.local.bssid[5]);

  f.printf("%lu,\"%s\",%s,%s,%d,%s,%s,%d,%d,%d,/%d,\"%s\"\n",
           (unsigned long)p.timestamp_ms,
           p.local.ssid_name, bssid_str,
           p.verdict_label, p.confidence,
           p.internet.public_ip, p.internet.asn,
           p.internet.is_mobile ? 1 : 0,
           p.internet.cgnat_detected ? 1 : 0,
           p.internet.ping_ms,
           p.local.subnet_prefix,
           p.local.hotspot_hint);
  f.close();
  return true;
}
