#ifndef NET_PROFILE_H
#define NET_PROFILE_H

#include <Arduino.h>
#include <WiFi.h>

// ----- Modulo B: Red local -----
struct LocalNetData {
  char local_ip[16];
  char gateway_ip[16];
  char subnet_mask[16];
  char dns1[16];
  char dns2[16];
  uint8_t subnet_prefix;
  char ssid_name[33];
  uint8_t bssid[6];
  bool is_likely_mobile_hotspot;
  char hotspot_hint[24];
};

// ----- Modulo C: Internet -----
struct InternetData {
  char public_ip[40];
  char asn[12];
  char org[48];
  char country[4];
  char city[32];
  bool is_mobile;
  char isp_type[16];
  bool cgnat_detected;
  int16_t ping_ms;
  uint8_t ping_jitter;
  uint8_t hop_count;
  bool api_ok;
};

enum ConnType {
  CONN_TYPE_UNKNOWN = 0,
  CONN_TYPE_CELLULAR,
  CONN_TYPE_ISP_FIXED,
  CONN_TYPE_INDETERMINATE
};

struct ConnectionProfile {
  LocalNetData local;
  InternetData internet;
  ConnType verdict;
  uint8_t confidence;
  char verdict_label[32];
  uint32_t timestamp_ms;
};

void net_profile_local(LocalNetData& out);
bool net_profile_internet(InternetData& out, const char* endpoint_url);
void net_profile_classify(ConnectionProfile& profile);
void net_profile_full(ConnectionProfile& out,
                      void (*callback_progress)(uint8_t phase, uint8_t total));

#endif
