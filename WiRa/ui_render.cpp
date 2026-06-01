// =====================================================================
// ui_render.cpp - UI con sprite (anti-flicker)
// Sprite 240x135 @ 16bpp = ~64KB en SRAM
// =====================================================================
#include "ui_render.h"
#include <cstdio>

#define HEADER_HEIGHT 20
#define FOOTER_HEIGHT 15
#define ITEM_HEIGHT 18

static M5Canvas canvas(&M5Cardputer.Display);
static bool canvas_ready = false;

void ui_init_canvas() {
  if (canvas_ready) return;
  canvas.setColorDepth(16);
  if (!canvas.createSprite(240, 135)) {
    Serial.println("[UI] Canvas create FAIL - usando display directo");
    canvas_ready = false;
    return;
  }
  canvas.setTextSize(1);
  canvas_ready = true;
  Serial.println("[UI] Canvas OK (240x135 @ 16bpp)");
}

static M5GFX& gfx() {
  // Helper para fallback si canvas no se inicializo
  return canvas_ready ? (M5GFX&)canvas : M5Cardputer.Display;
}

// Bateria en esquina superior derecha
static void draw_battery() {
  int8_t level = M5.Power.getBatteryLevel();
  if (level < 0) level = 0;
  if (level > 100) level = 100;
  uint32_t color = (level > 50) ? TFT_GREEN :
                   (level > 20) ? TFT_YELLOW : TFT_RED;
  char bat[8];
  snprintf(bat, sizeof(bat), "%d%%", level);
  gfx().setTextColor(color);
  gfx().setTextSize(1);
  gfx().setCursor(210, 3);
  gfx().print(bat);
}

static void push() {
  if (canvas_ready) canvas.pushSprite(0, 0);
}

// =====================================================================
// SCANNING
// =====================================================================
void ui_render_scanning() {
  gfx().fillScreen(TFT_BLACK);
  gfx().setTextSize(2);
  gfx().setTextColor(TFT_CYAN);
  gfx().setCursor(50, 10);
  gfx().println("WiRa Scan");

  gfx().setTextSize(1);
  gfx().setTextColor(TFT_WHITE);
  gfx().setCursor(30, 50);
  gfx().println("Escaneando redes WiFi...");

  uint8_t sp = (millis() / 200) % 4;
  const char* spin[] = {"|", "/", "-", "\\"};
  gfx().setTextSize(3);
  gfx().setTextColor(TFT_YELLOW);
  gfx().setCursor(100, 70);
  gfx().print(spin[sp]);

  gfx().setTextSize(1);
  gfx().setTextColor(TFT_DARKGRAY);
  gfx().setCursor(20, 115);
  gfx().println("Espera unos segundos...");

  draw_battery();
  push();
}

// =====================================================================
// LIST
// =====================================================================
void ui_render_list(const WiFiScanResult networks[],
                    uint16_t network_count,
                    uint16_t selected_index) {
  gfx().fillScreen(TFT_BLACK);
  gfx().setTextSize(2);
  gfx().setTextColor(TFT_CYAN);
  gfx().setCursor(5, 2);
  gfx().print("WiRa");

  if (network_count == 0) {
    gfx().setTextSize(1);
    gfx().setTextColor(TFT_RED);
    gfx().setCursor(30, 60);
    gfx().println("No se encontraron redes");
    gfx().setTextColor(TFT_DARKGRAY);
    gfx().setCursor(20, 85);
    gfx().println("[R] Rescanear");
    draw_battery();
    push();
    return;
  }

  const uint8_t max_visible = 5;
  uint16_t scroll_start = 0;
  if (selected_index > max_visible - 1) {
    scroll_start = selected_index - (max_visible - 1);
  }
  uint16_t scroll_end = scroll_start + max_visible;
  if (scroll_end > network_count) scroll_end = network_count;

  int16_t y = HEADER_HEIGHT + 5;

  for (uint16_t i = scroll_start; i < scroll_end; i++) {
    const WiFiScanResult& net = networks[i];

    if (i == selected_index) {
      gfx().fillRect(0, y - 2, 240, ITEM_HEIGHT, TFT_DARKGRAY);
      gfx().setTextColor(TFT_BLACK);
    } else {
      gfx().setTextColor(TFT_WHITE);
    }

    gfx().setTextSize(1);
    gfx().setCursor(5, y);
    gfx().printf("%-4s ", net.auth_label);

    char ssid_short[20];
    strlcpy(ssid_short, net.ssid, 18);
    ssid_short[18] = '\0';
    gfx().print(ssid_short);

    char rssi_str[20];
    snprintf(rssi_str, sizeof(rssi_str), "CH%d %3d", net.channel, net.rssi);
    int16_t text_width = gfx().textWidth(rssi_str);
    gfx().setCursor(240 - text_width - 5, y);
    gfx().print(rssi_str);

    y += ITEM_HEIGHT;
  }

  // Flechas scroll
  if (scroll_start > 0) {
    gfx().setTextColor(TFT_YELLOW);
    gfx().setCursor(230, HEADER_HEIGHT + 2);
    gfx().print("^");
  }
  if (scroll_end < network_count) {
    gfx().setTextColor(TFT_YELLOW);
    gfx().setCursor(230, 135 - FOOTER_HEIGHT - 10);
    gfx().print("v");
  }

  gfx().setTextSize(1);
  gfx().setTextColor(TFT_DARKGRAY);
  gfx().setCursor(5, 135 - FOOTER_HEIGHT);
  gfx().print("[E]RSSI [C]Conect [R]Scan");

  if (network_count > max_visible) {
    gfx().setTextColor(TFT_YELLOW);
    gfx().setCursor(205, 135 - FOOTER_HEIGHT);
    gfx().printf("%d/%d", selected_index + 1, network_count);
  }
  draw_battery();
  push();
}

// =====================================================================
// MEASURING
// =====================================================================
void ui_render_measuring(const WiFiScanResult network,
                         const int16_t rssi_history[],
                         uint8_t history_count) {
  gfx().fillScreen(TFT_BLACK);

  gfx().setTextSize(2);
  gfx().setTextColor(TFT_CYAN);
  gfx().setCursor(5, 2);
  char ssid_display[30];
  strlcpy(ssid_display, network.ssid, 25);
  ssid_display[25] = '\0';
  gfx().println(ssid_display);

  int16_t y = 25;
  int16_t cur = (history_count > 0) ? rssi_history[history_count - 1] : -99;
  uint8_t pct = wifi_rssi_to_percent(cur);
  uint32_t col = wifi_get_quality_color(cur);
  const char* q = wifi_get_quality_string(cur);

  gfx().setTextSize(1);
  gfx().setTextColor(TFT_WHITE);
  gfx().setCursor(5, y);
  gfx().print("RSSI: ");
  gfx().setTextColor(col);
  gfx().printf("%4d dBm  (%3d%%)\n", cur, pct);

  y += 12;
  ui_draw_bar(5, y, 230, 10, pct, TFT_DARKGRAY, col);

  gfx().setTextColor(TFT_WHITE);
  y += 15;
  gfx().setCursor(5, y);
  gfx().print("Calidad: ");
  gfx().setTextColor(col);
  gfx().println(q);

  y += 12;
  gfx().setTextColor(TFT_WHITE);
  gfx().setCursor(5, y);
  gfx().printf("CH%d  ", network.channel);

  if (network.is_open) {
    gfx().setTextColor(TFT_RED);
    gfx().print("OPEN");
  } else {
    gfx().setTextColor(TFT_GREEN);
    gfx().print(network.auth_label);
  }

  // BSSID
  y += 11;
  gfx().setTextColor(TFT_DARKGRAY);
  gfx().setCursor(5, y);
  gfx().printf("%02X:%02X:%02X:%02X:%02X:%02X",
               network.bssid[0], network.bssid[1], network.bssid[2],
               network.bssid[3], network.bssid[4], network.bssid[5]);

  // Grafico historico
  if (history_count > 1) {
    y += 12;
    ui_draw_graph(5, y, 230, 22, rssi_history, history_count, -90, -30);
  }

  gfx().setTextColor(TFT_DARKGRAY);
  gfx().setCursor(5, 135 - FOOTER_HEIGHT);
  gfx().print("[B]Atras [R]Scan");
  draw_battery();
  push();
}

// =====================================================================
// PASSWORD
// =====================================================================
void ui_render_password(const char* ssid, const char* buf, uint8_t cursor) {
  gfx().fillScreen(TFT_BLACK);

  gfx().setTextSize(1);
  gfx().setTextColor(TFT_CYAN);
  gfx().setCursor(5, 5);
  gfx().print("Contrasena para:");

  gfx().setTextColor(TFT_WHITE);
  gfx().setCursor(5, 20);
  char ssid_short[28];
  strlcpy(ssid_short, ssid, 27);
  ssid_short[27] = '\0';
  gfx().print(ssid_short);

  // Caja input
  gfx().drawRect(5, 45, 230, 28, TFT_WHITE);
  gfx().setTextSize(2);
  gfx().setTextColor(TFT_GREEN);
  gfx().setCursor(10, 52);
  // Mostrar buffer (sin enmascarar para depurar; enmascarar en produccion)
  gfx().print(buf);

  gfx().setTextSize(1);
  gfx().setTextColor(TFT_DARKGRAY);
  gfx().setCursor(5, 90);
  gfx().printf("Caracteres: %d/%d", cursor, PASSWORD_MAX_LEN - 1);

  gfx().setCursor(5, 135 - FOOTER_HEIGHT);
  gfx().print("[Enter]OK [Bksp]Borr [`]Esc");
  draw_battery();
  push();
}

// =====================================================================
// CONNECTING
// =====================================================================
void ui_render_connecting(const char* ssid, const char* msg, uint8_t attempt) {
  gfx().fillScreen(TFT_BLACK);

  gfx().setTextSize(2);
  gfx().setTextColor(TFT_CYAN);
  gfx().setCursor(20, 20);
  gfx().print("Conectando");

  gfx().setTextSize(1);
  gfx().setTextColor(TFT_WHITE);
  gfx().setCursor(5, 55);
  char ss[33];
  strlcpy(ss, ssid, 32);
  gfx().print(ss);

  if (attempt > 0) {
    gfx().setTextColor(TFT_YELLOW);
    gfx().setCursor(5, 75);
    gfx().printf("Intento %d", attempt);
  }

  gfx().setTextColor(TFT_DARKGRAY);
  gfx().setCursor(5, 95);
  gfx().print(msg);

  draw_battery();
  push();
}

// =====================================================================
// PROFILING (progreso)
// =====================================================================
void ui_render_profiling(uint8_t phase, uint8_t total) {
  gfx().fillScreen(TFT_BLACK);

  gfx().setTextSize(2);
  gfx().setTextColor(TFT_CYAN);
  gfx().setCursor(40, 15);
  gfx().print("Analizando");

  const char* phases[] = {"Iniciando", "Red local", "Internet", "Clasificando"};
  uint8_t idx = (phase >= 1 && phase <= 3) ? phase : 0;

  gfx().setTextSize(1);
  gfx().setTextColor(TFT_WHITE);
  gfx().setCursor(5, 55);
  gfx().printf("Paso %d/%d: %s", phase, total, phases[idx]);

  // Barra progreso
  uint16_t w = (220 * phase) / total;
  gfx().drawRect(10, 80, 220, 16, TFT_WHITE);
  gfx().fillRect(11, 81, w > 2 ? w - 2 : 0, 14, TFT_GREEN);

  draw_battery();
  push();
}

// =====================================================================
// PROFILE VIEW
// =====================================================================
void ui_render_profile(const ConnectionProfile& p, bool show_saved) {
  gfx().fillScreen(TFT_BLACK);

  gfx().setTextSize(1);
  gfx().setTextColor(TFT_CYAN);
  gfx().setCursor(5, 3);
  gfx().print("WiRa Profile");
  draw_battery();

  // SSID
  gfx().setTextColor(TFT_WHITE);
  gfx().setCursor(5, 16);
  char ss[33];
  strlcpy(ss, p.local.ssid_name, 32);
  gfx().print(ss);

  // Veredicto grande
  uint32_t v_col = (p.verdict == CONN_TYPE_CELLULAR) ? TFT_YELLOW :
                   (p.verdict == CONN_TYPE_ISP_FIXED) ? TFT_GREEN :
                   TFT_DARKGRAY;
  gfx().setTextSize(2);
  gfx().setTextColor(v_col);
  gfx().setCursor(5, 28);
  gfx().print(p.verdict_label);

  // Confianza barra
  gfx().setTextSize(1);
  gfx().setTextColor(TFT_WHITE);
  gfx().setCursor(5, 48);
  gfx().printf("Conf:%3d%%", p.confidence);
  ui_draw_bar(80, 48, 150, 8, p.confidence, TFT_DARKGRAY, v_col);

  // Datos clave 3 filas
  int16_t y = 62;
  gfx().setTextColor(TFT_CYAN); gfx().setCursor(5, y); gfx().print("IP:");
  gfx().setTextColor(TFT_WHITE); gfx().setCursor(35, y);
  gfx().print(p.internet.api_ok ? p.internet.public_ip : "N/D");
  gfx().setTextColor(TFT_CYAN); gfx().setCursor(150, y); gfx().print("ASN:");
  gfx().setTextColor(TFT_WHITE); gfx().setCursor(178, y);
  gfx().print(p.internet.asn);

  y += 11;
  gfx().setTextColor(TFT_CYAN); gfx().setCursor(5, y); gfx().print("Sub:");
  gfx().setTextColor(TFT_WHITE); gfx().setCursor(35, y);
  gfx().printf("/%d", p.local.subnet_prefix);
  gfx().setTextColor(TFT_CYAN); gfx().setCursor(80, y); gfx().print("CGN:");
  gfx().setTextColor(p.internet.cgnat_detected ? TFT_YELLOW : TFT_GREEN);
  gfx().setCursor(108, y);
  gfx().print(p.internet.cgnat_detected ? "SI" : "NO");
  gfx().setTextColor(TFT_CYAN); gfx().setCursor(140, y); gfx().print("Mob:");
  gfx().setTextColor(p.internet.is_mobile ? TFT_YELLOW : TFT_GREEN);
  gfx().setCursor(168, y);
  gfx().print(p.internet.is_mobile ? "SI" : "NO");

  y += 11;
  gfx().setTextColor(TFT_CYAN); gfx().setCursor(5, y); gfx().print("Png:");
  gfx().setTextColor(TFT_WHITE); gfx().setCursor(35, y);
  if (p.internet.ping_ms >= 0) gfx().printf("%dms", p.internet.ping_ms);
  else gfx().print("N/D");
  gfx().setTextColor(TFT_CYAN); gfx().setCursor(80, y); gfx().print("Tipo:");
  gfx().setTextColor(TFT_WHITE); gfx().setCursor(112, y);
  gfx().print(p.internet.isp_type);

  // Hint hotspot
  y += 11;
  gfx().setTextColor(TFT_DARKGRAY); gfx().setCursor(5, y);
  gfx().print(p.local.hotspot_hint);

  // Mensaje guardado
  if (show_saved) {
    gfx().fillRect(60, 100, 120, 14, TFT_DARKGREEN);
    gfx().setTextColor(TFT_WHITE);
    gfx().setCursor(70, 103);
    gfx().print("Guardado en SD");
  }

  gfx().setTextColor(TFT_DARKGRAY);
  gfx().setCursor(5, 135 - FOOTER_HEIGHT);
  gfx().print("[B]Volver [R]Re-perfil [S]SD");
  push();
}

// =====================================================================
// ERROR
// =====================================================================
void ui_render_error(const char* msg) {
  gfx().fillScreen(TFT_BLACK);
  gfx().setTextSize(2);
  gfx().setTextColor(TFT_RED);
  gfx().setCursor(80, 25);
  gfx().print("ERROR");

  gfx().setTextSize(1);
  gfx().setTextColor(TFT_WHITE);
  gfx().setCursor(5, 60);
  gfx().print(msg);

  gfx().setTextColor(TFT_DARKGRAY);
  gfx().setCursor(5, 135 - FOOTER_HEIGHT);
  gfx().print("[B]/[R] Volver al scan");
  draw_battery();
  push();
}

// =====================================================================
// HELPERS
// =====================================================================
void ui_draw_bar(int16_t x, int16_t y, int16_t w, int16_t h,
                 uint8_t value, uint32_t empty, uint32_t full) {
  gfx().drawRect(x, y, w, h, TFT_WHITE);
  gfx().fillRect(x + 1, y + 1, w - 2, h - 2, empty);
  uint16_t fw = ((w - 2) * value) / 100;
  if (fw > 0) gfx().fillRect(x + 1, y + 1, fw, h - 2, full);
}

void ui_draw_graph(int16_t x, int16_t y, int16_t w, int16_t h,
                   const int16_t data[], uint8_t n,
                   int16_t mn_val, int16_t mx_val) {
  if (n < 2) return;
  gfx().drawRect(x, y, w, h, TFT_DARKGRAY);

  int16_t range = mx_val - mn_val;
  if (range <= 0) range = 1;

  for (uint8_t i = 1; i < n; i++) {
    int16_t x1 = x + ((i - 1) * w) / n;
    int16_t y1v = data[i - 1];
    int16_t y1 = y + h - 2 - ((y1v - mn_val) * h) / range;
    if (y1 < y) y1 = y;
    if (y1 > y + h) y1 = y + h;

    int16_t x2 = x + (i * w) / n;
    int16_t y2v = data[i];
    int16_t y2 = y + h - 2 - ((y2v - mn_val) * h) / range;
    if (y2 < y) y2 = y;
    if (y2 > y + h) y2 = y + h;

    uint32_t col = wifi_get_quality_color(y2v);
    gfx().drawLine(x1, y1, x2, y2, col);
  }
}
