// =====================================================================
// WiRa.ino - Firmware principal
// M5Stack Cardputer ADV - ESP32-S3FN8
// v2.0
// =====================================================================
// WiRa = WiFi Radar/Analyzer + Network Profiler
// Funcionalidades:
//  - Scan WiFi 2.4GHz + medidor RSSI tiempo real
//  - Conexion a AP + perfilado local/internet
//  - Clasificacion CELULAR / ISP_FIJO / INDETERMINADO
//  - Logging JSON+CSV a microSD
// =====================================================================

#include <M5Cardputer.h>
#include "wifi_scan.h"
#include "wifi_connect.h"
#include "net_profile.h"
#include "ui_render.h"
#include "keyboard_input.h"
#include "storage.h"

// =====================================================================
// MAQUINA DE ESTADOS
// =====================================================================
enum AppState {
  STATE_SCANNING,
  STATE_LIST,
  STATE_MEASURING,
  STATE_CONNECTING,
  STATE_PROFILING,
  STATE_PROFILE_VIEW,
  STATE_ERROR
};

// =====================================================================
// ESTADO GLOBAL
// =====================================================================
struct AppStateData {
  AppState current_state = STATE_SCANNING;
  AppState next_state = STATE_SCANNING;
  uint16_t selected_ap_index = 0;
  uint32_t last_scan_time = 0;
  uint32_t last_measure_time = 0;
  int16_t rssi_history[30] = {};
  uint8_t rssi_history_count = 0;
  char error_message[64] = {0};

  // Pantalla password
  char password_buffer[PASSWORD_MAX_LEN] = {0};
  uint8_t password_cursor = 0;

  // Profiler
  ConnectionProfile profile;
  uint8_t profile_phase = 0;     // 0..3
  uint8_t profile_total = 3;
  bool profile_show_saved = false;
  uint32_t profile_saved_time_ms = 0;
} app_state;

WiFiScanResult wifi_networks[MAX_NETWORKS];
uint16_t network_count = 0;

// =====================================================================
// CALLBACK: progreso profiling -> UI
// =====================================================================
static void profile_progress_cb(uint8_t phase, uint8_t total) {
  app_state.profile_phase = phase;
  app_state.profile_total = total;
}

// =====================================================================
// SETUP
// =====================================================================
void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.setRotation(1);

  Serial.begin(115200);
  delay(100);
  Serial.println("\n=== WiRa v2.0 ===");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);

  keyboard_init();
  ui_init_canvas();
  storage_init();  // No critico si falla

  app_state.current_state = STATE_SCANNING;
}

// =====================================================================
// HANDLERS POR ESTADO
// =====================================================================
static void handle_scanning(const KeyEvent& key_event) {
  uint32_t now = millis();
  if (app_state.last_scan_time == 0 || (now - app_state.last_scan_time > 10000)) {
    app_state.last_scan_time = now;
    network_count = wifi_scan_networks(wifi_networks, MAX_NETWORKS);
    app_state.next_state = STATE_LIST;
    app_state.rssi_history_count = 0;
  }
  ui_render_scanning();
}

static void handle_list(const KeyEvent& key_event) {
  if (key_event.key == KEY_UP && app_state.selected_ap_index > 0) {
    app_state.selected_ap_index--;
  }
  else if (key_event.key == KEY_DOWN &&
           app_state.selected_ap_index + 1 < network_count) {
    app_state.selected_ap_index++;
  }
  else if (key_event.key == KEY_SELECT) {
    // Medidor RSSI (modo legacy)
    app_state.next_state = STATE_MEASURING;
    app_state.rssi_history_count = 0;
  }
  else if (key_event.key == KEY_CONNECT) {
    // Conectar a AP -> entrada password o directo si abierto
    const WiFiScanResult& ap = wifi_networks[app_state.selected_ap_index];
    if (ap.is_open) {
      app_state.password_buffer[0] = 0;
      app_state.password_cursor = 0;
      app_state.next_state = STATE_CONNECTING;
    } else {
      app_state.password_buffer[0] = 0;
      app_state.password_cursor = 0;
      app_state.next_state = STATE_CONNECTING; // entra a password screen primero
    }
  }
  else if (key_event.key == KEY_RESCAN) {
    app_state.next_state = STATE_SCANNING;
    app_state.last_scan_time = 0;
  }

  ui_render_list(wifi_networks, network_count, app_state.selected_ap_index);
}

static void handle_measuring(const KeyEvent& key_event) {
  uint32_t now = millis();
  bool new_data = false;

  if (now - app_state.last_measure_time > 350) {
    app_state.last_measure_time = now;
    const WiFiScanResult& sel = wifi_networks[app_state.selected_ap_index];
    int16_t rssi = wifi_measure_rssi(sel.ssid, sel.bssid, sel.channel);

    if (app_state.rssi_history_count < 30) {
      app_state.rssi_history[app_state.rssi_history_count++] = rssi;
    } else {
      for (int i = 0; i < 29; i++) {
        app_state.rssi_history[i] = app_state.rssi_history[i + 1];
      }
      app_state.rssi_history[29] = rssi;
    }
    new_data = true;
  }

  if (key_event.key == KEY_BACK) {
    app_state.next_state = STATE_LIST;
  }
  else if (key_event.key == KEY_RESCAN) {
    app_state.next_state = STATE_SCANNING;
    app_state.last_scan_time = 0;
  }

  // Render condicional - solo si hay datos nuevos o input
  if (new_data || key_event.key != KEY_NONE) {
    ui_render_measuring(wifi_networks[app_state.selected_ap_index],
                        app_state.rssi_history,
                        app_state.rssi_history_count);
  }
}

static void handle_connecting(const KeyEvent& key_event) {
  const WiFiScanResult& ap = wifi_networks[app_state.selected_ap_index];

  // Si red abierta, saltar a conexion directa
  if (ap.is_open) {
    ui_render_connecting(ap.ssid, "Conectando...", 0);
    ConnectResult res = wifi_connect(ap, "");
    if (res == CONN_OK) {
      app_state.next_state = STATE_PROFILING;
      app_state.profile_phase = 0;
    } else {
      strlcpy(app_state.error_message, "Fallo conexion red abierta", 64);
      app_state.next_state = STATE_ERROR;
    }
    return;
  }

  // Red cifrada: pantalla de password
  // Entrada caracter
  if (key_event.key == KEY_CHAR && key_event.ch >= 32 && key_event.ch < 127) {
    if (app_state.password_cursor + 1 < PASSWORD_MAX_LEN) {
      app_state.password_buffer[app_state.password_cursor++] = key_event.ch;
      app_state.password_buffer[app_state.password_cursor] = 0;
    }
  }
  else if (key_event.key == KEY_BACK) {
    if (app_state.password_cursor > 0) {
      app_state.password_cursor--;
      app_state.password_buffer[app_state.password_cursor] = 0;
    } else {
      // Buffer vacio -> volver
      app_state.next_state = STATE_LIST;
    }
  }
  else if (key_event.key == KEY_SELECT) {
    // Intentar conectar
    ui_render_connecting(ap.ssid, "Conectando...", 1);
    ConnectResult res = wifi_connect(ap, app_state.password_buffer);
    if (res == CONN_OK) {
      app_state.next_state = STATE_PROFILING;
      app_state.profile_phase = 0;
    } else if (res == CONN_WRONG_PASSWORD) {
      strlcpy(app_state.error_message, "Contrasena incorrecta", 64);
      app_state.next_state = STATE_ERROR;
    } else if (res == CONN_TIMEOUT) {
      strlcpy(app_state.error_message, "Timeout conexion (15s)", 64);
      app_state.next_state = STATE_ERROR;
    } else {
      strlcpy(app_state.error_message, "Fallo conexion", 64);
      app_state.next_state = STATE_ERROR;
    }
    return;
  }

  ui_render_password(ap.ssid, app_state.password_buffer, app_state.password_cursor);
}

static void handle_profiling(const KeyEvent& key_event) {
  // Ejecutar profile completo (bloqueante por simplicidad; UI muestra fase actual)
  ui_render_profiling(app_state.profile_phase, app_state.profile_total);

  // Ejecutar perfil completo una vez
  net_profile_full(app_state.profile, profile_progress_cb);

  app_state.next_state = STATE_PROFILE_VIEW;
}

static void handle_profile_view(const KeyEvent& key_event) {
  if (key_event.key == KEY_BACK) {
    wifi_disconnect();
    app_state.next_state = STATE_LIST;
  }
  else if (key_event.key == KEY_RESCAN) {
    // Re-perfilar
    app_state.profile_phase = 0;
    app_state.next_state = STATE_PROFILING;
  }
  else if (key_event.key == KEY_SAVE) {
    bool ok = storage_save_profile(app_state.profile) &&
              storage_append_csv(app_state.profile);
    app_state.profile_show_saved = true;
    app_state.profile_saved_time_ms = millis();
    if (!ok) strlcpy(app_state.profile.verdict_label, "Sin SD", 32);
  }

  // Auto-ocultar mensaje "Guardado" tras 2s
  if (app_state.profile_show_saved &&
      millis() - app_state.profile_saved_time_ms > 2000) {
    app_state.profile_show_saved = false;
  }

  ui_render_profile(app_state.profile, app_state.profile_show_saved);
}

static void handle_error(const KeyEvent& key_event) {
  ui_render_error(app_state.error_message);
  if (key_event.key == KEY_RESCAN || key_event.key == KEY_BACK) {
    app_state.next_state = STATE_SCANNING;
    app_state.last_scan_time = 0;
  }
}

// =====================================================================
// LOOP PRINCIPAL
// =====================================================================
void loop() {
  M5Cardputer.update();

  // Modo raw para password (acepta , . ; / como chars)
  bool raw_mode = (app_state.current_state == STATE_CONNECTING &&
                   !wifi_networks[app_state.selected_ap_index].is_open);
  KeyEvent key_event = keyboard_process(raw_mode);

  switch (app_state.current_state) {
    case STATE_SCANNING:    handle_scanning(key_event); break;
    case STATE_LIST:        handle_list(key_event); break;
    case STATE_MEASURING:   handle_measuring(key_event); break;
    case STATE_CONNECTING:  handle_connecting(key_event); break;
    case STATE_PROFILING:   handle_profiling(key_event); break;
    case STATE_PROFILE_VIEW: handle_profile_view(key_event); break;
    case STATE_ERROR:       handle_error(key_event); break;
  }

  // Transicion de estado
  if (app_state.next_state != app_state.current_state) {
    Serial.printf("[STATE] %d -> %d\n",
                  app_state.current_state, app_state.next_state);
    app_state.current_state = app_state.next_state;
  }

  delay(30);
}
