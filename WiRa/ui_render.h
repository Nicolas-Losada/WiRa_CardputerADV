#ifndef UI_RENDER_H
#define UI_RENDER_H

#include <M5Cardputer.h>
#include "wifi_scan.h"
#include "wifi_connect.h"
#include "net_profile.h"

void ui_init_canvas();

void ui_draw_bar(int16_t x, int16_t y, int16_t width, int16_t height,
                 uint8_t value, uint32_t color_empty, uint32_t color_full);
void ui_draw_graph(int16_t x, int16_t y, int16_t width, int16_t height,
                   const int16_t data[], uint8_t data_count,
                   int16_t min_val, int16_t max_val);

void ui_render_scanning();
void ui_render_list(const WiFiScanResult networks[],
                    uint16_t network_count,
                    uint16_t selected_index);
void ui_render_measuring(const WiFiScanResult network,
                         const int16_t rssi_history[],
                         uint8_t history_count);

void ui_render_password(const char* ssid, const char* buf, uint8_t cursor);
void ui_render_connecting(const char* ssid, const char* msg, uint8_t attempt);
void ui_render_profiling(uint8_t phase, uint8_t total);
void ui_render_profile(const ConnectionProfile& p, bool show_saved);
void ui_render_error(const char* msg);

#endif
