// =====================================================================
// net_profile.cpp - Perfilado local + internet + clasificador
// =====================================================================
#include "net_profile.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP32Ping.h>

#define DEFAULT_API "http://ip-api.com/json?fields=query,org,as,mobile,country,city,hosting"

// ============================================================
// Modulo B: Red local
// ============================================================
void net_profile_local(LocalNetData& out) {
  strlcpy(out.local_ip, WiFi.localIP().toString().c_str(), 16);
  strlcpy(out.gateway_ip, WiFi.gatewayIP().toString().c_str(), 16);
  strlcpy(out.subnet_mask, WiFi.subnetMask().toString().c_str(), 16);
  strlcpy(out.dns1, WiFi.dnsIP(0).toString().c_str(), 16);
  strlcpy(out.dns2, WiFi.dnsIP(1).toString().c_str(), 16);

  // Prefijo CIDR desde mascara
  IPAddress mask = WiFi.subnetMask();
  uint32_t m = ((uint32_t)mask[0]<<24)|((uint32_t)mask[1]<<16)|((uint32_t)mask[2]<<8)|mask[3];
  out.subnet_prefix = __builtin_popcount(m);

  strlcpy(out.ssid_name, WiFi.SSID().c_str(), 33);
  uint8_t* b = WiFi.BSSID();
  if (b) memcpy(out.bssid, b, 6);

  // Clasificacion de subred (hotspot hint)
  IPAddress local = WiFi.localIP();
  out.is_likely_mobile_hotspot = false;

  if (local[0] == 172 && local[1] == 20 && local[2] == 10 && out.subnet_prefix == 28) {
    out.is_likely_mobile_hotspot = true;
    strlcpy(out.hotspot_hint, "iPhone hotspot", 24);
  }
  else if (local[0] == 192 && local[1] == 168 && local[2] == 43) {
    out.is_likely_mobile_hotspot = true;
    strlcpy(out.hotspot_hint, "Android hotspot", 24);
  }
  else if (out.subnet_prefix >= 28) {
    out.is_likely_mobile_hotspot = true;
    strlcpy(out.hotspot_hint, "Posible hotspot", 24);
  }
  else {
    strlcpy(out.hotspot_hint, "Router/AP", 24);
  }
}

// ============================================================
// Modulo C: Internet (ping + ip-api)
// ============================================================
bool net_profile_internet(InternetData& out, const char* endpoint_url) {
  // Defaults seguros
  strlcpy(out.public_ip, "N/D", 40);
  strlcpy(out.asn, "N/D", 12);
  strlcpy(out.org, "N/D", 48);
  strlcpy(out.country, "??", 4);
  strlcpy(out.city, "N/D", 32);
  strlcpy(out.isp_type, "?", 16);
  out.is_mobile = false;
  out.cgnat_detected = false;
  out.api_ok = false;
  out.ping_ms = -1;
  out.ping_jitter = 0;
  out.hop_count = 0;

  if (WiFi.status() != WL_CONNECTED) return false;

  // --- Ping a 1.1.1.1 (4 muestras) ---
  IPAddress cf(1,1,1,1);
  uint32_t total = 0, ok = 0;
  uint16_t mn = 65535, mx = 0;
  for (uint8_t i = 0; i < 4; i++) {
    if (Ping.ping(cf, 1)) {
      uint32_t t = Ping.averageTime();
      total += t;
      ok++;
      if (t < mn) mn = t;
      if (t > mx) mx = t;
    }
  }
  if (ok > 0) {
    out.ping_ms = (int16_t)(total / ok);
    out.ping_jitter = (uint8_t)(mx - mn);
  }

  // --- ip-api.com ---
  const char* url = (endpoint_url && endpoint_url[0]) ? endpoint_url : DEFAULT_API;
  WiFiClient client;
  HTTPClient http;
  http.setTimeout(5000);
  if (!http.begin(client, url)) return false;

  int code = http.GET();
  if (code == 200) {
    StaticJsonDocument<128> filter;
    filter["query"]   = true;
    filter["as"]      = true;
    filter["org"]     = true;
    filter["mobile"]  = true;
    filter["country"] = true;
    filter["city"]    = true;
    filter["hosting"] = true;

    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, http.getStream(),
                                DeserializationOption::Filter(filter));
    if (!err) {
      strlcpy(out.public_ip, doc["query"]   | "N/D", 40);

      // Extraer "AS12345" del campo as
      const char* asraw = doc["as"] | "";
      if (strncmp(asraw, "AS", 2) == 0) {
        const char* sp = strchr(asraw, ' ');
        size_t len = sp ? (size_t)(sp - asraw) : strlen(asraw);
        if (len >= sizeof(out.asn)) len = sizeof(out.asn) - 1;
        memcpy(out.asn, asraw, len);
        out.asn[len] = 0;
      }

      strlcpy(out.org,     doc["org"]     | "N/D", 48);
      strlcpy(out.country, doc["country"] | "??",  4);
      strlcpy(out.city,    doc["city"]    | "N/D", 32);
      out.is_mobile = doc["mobile"] | false;
      bool is_hosting = doc["hosting"] | false;

      if (out.is_mobile)    strlcpy(out.isp_type, "mobile", 16);
      else if (is_hosting)  strlcpy(out.isp_type, "hosting", 16);
      else                  strlcpy(out.isp_type, "residential", 16);

      out.api_ok = true;
    }
  }
  http.end();

  // Detectar CGNAT en IP publica
  if (out.api_ok) {
    IPAddress pub;
    if (pub.fromString(out.public_ip)) {
      if (pub[0] == 100 && pub[1] >= 64 && pub[1] <= 127) {
        out.cgnat_detected = true;
      }
    }
  }

  return out.api_ok;
}

// ============================================================
// Clasificador con scoring
// ============================================================
void net_profile_classify(ConnectionProfile& p) {
  int score = 0;  // + = celular, - = ISP fijo

  // Lado local
  if (p.local.is_likely_mobile_hotspot) {
    if (strstr(p.local.hotspot_hint, "iPhone"))       score += 60;
    else if (strstr(p.local.hotspot_hint, "Android")) score += 45;
    else                                              score += 25;
  } else {
    score -= 30;
  }

  // Lado internet
  if (p.internet.api_ok) {
    if (p.internet.is_mobile)      score += 40;
    if (p.internet.cgnat_detected) score += 25;
    if (p.internet.ping_ms > 80)            score += 10;
    else if (p.internet.ping_ms > 0 && p.internet.ping_ms < 30) score -= 15;
  }

  if (score >= 50) {
    p.verdict = CONN_TYPE_CELLULAR;
    strlcpy(p.verdict_label, "CELULAR", 32);
    p.confidence = score > 100 ? 100 : score;
  } else if (score <= -20) {
    p.verdict = CONN_TYPE_ISP_FIXED;
    strlcpy(p.verdict_label, "ISP FIJO", 32);
    uint8_t v = (uint8_t)(score < -100 ? 100 : -score);
    p.confidence = v;
  } else {
    p.verdict = CONN_TYPE_INDETERMINATE;
    strlcpy(p.verdict_label, "INDETERMINADO", 32);
    p.confidence = (uint8_t)(50 - (score < 0 ? -score : score));
  }
}

// ============================================================
// Wrapper completo
// ============================================================
void net_profile_full(ConnectionProfile& out,
                      void (*cb)(uint8_t phase, uint8_t total)) {
  memset(&out, 0, sizeof(out));
  out.timestamp_ms = millis();

  if (cb) cb(1, 3);
  net_profile_local(out.local);

  if (cb) cb(2, 3);
  net_profile_internet(out.internet, nullptr);

  if (cb) cb(3, 3);
  net_profile_classify(out);
}
