/*
  =====================================================================
  MÓDULO: UI Render (Implementación)
  =====================================================================
  Ver ui_render.h para documentación de funciones

  Layout de pantalla: 240 px (ancho) x 135 px (alto)
  Orientación: Landscape

  =====================================================================
*/

#include "ui_render.h"
#include "wifi_scan.h"
#include <cstdio>
#include <cmath>

// Constantes de posicionamiento
#define HEADER_HEIGHT 20
#define FOOTER_HEIGHT 15
#define ITEM_HEIGHT 18
#define TEXT_SIZE 1

// =====================================================================
// Función: Renderizar pantalla de escaneo
// =====================================================================
void ui_render_scanning() {
  M5.Display.fillScreen(TFT_BLACK);

  // Encabezado
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_CYAN);
  M5.Display.setCursor(50, 10);
  M5.Display.println("WiFi Analyzer");

  // Mensaje de escaneo
  M5.Display.setTextSize(TEXT_SIZE);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.setCursor(30, 50);
  M5.Display.println("Escaneando redes WiFi...");

  // Spinner simple (barra de progreso ancha)
  static uint32_t spinner_time = 0;
  uint32_t now = millis();

  if (now - spinner_time > 200) {
    spinner_time = now;
  }

  uint8_t spinner_phase = (now / 200) % 4;
  const char* spinner[] = {"|", "/", "-", "\\"};

  M5.Display.setTextSize(3);
  M5.Display.setTextColor(TFT_YELLOW);
  M5.Display.setCursor(100, 70);
  M5.Display.print(spinner[spinner_phase]);

  // Instrucción
  M5.Display.setTextSize(TEXT_SIZE);
  M5.Display.setTextColor(TFT_GRAY);
  M5.Display.setCursor(20, 115);
  M5.Display.println("Espera unos segundos...");
}

// =====================================================================
// Función: Renderizar lista de APs
// =====================================================================
void ui_render_list(const WiFiScanResult networks[],
                    uint16_t network_count,
                    uint16_t selected_index) {
  M5.Display.fillScreen(TFT_BLACK);

  // Encabezado
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_CYAN);
  M5.Display.setCursor(5, 2);
  M5.Display.print("WiFi Scan");

  // Si no hay redes
  if (network_count == 0) {
    M5.Display.setTextSize(TEXT_SIZE);
    M5.Display.setTextColor(TFT_RED);
    M5.Display.setCursor(30, 60);
    M5.Display.println("No se encontraron redes");

    M5.Display.setTextColor(TFT_GRAY);
    M5.Display.setCursor(20, 85);
    M5.Display.println("[R] Rescanear");
    return;
  }

  // Calcular cuántas redes caben en pantalla
  // 135px - 20px (header) - 15px (footer) = 100px disponibles
  // 100px / 18px (item height) = 5-6 items máximo
  const uint8_t max_visible = 5;

  // Calcular índice de scroll
  uint16_t scroll_start = 0;
  if (selected_index > max_visible - 1) {
    scroll_start = selected_index - (max_visible - 1);
  }
  uint16_t scroll_end = scroll_start + max_visible;
  if (scroll_end > network_count) {
    scroll_end = network_count;
  }

  // Renderizar lista
  int16_t y = HEADER_HEIGHT + 5;

  for (uint16_t i = scroll_start; i < scroll_end; i++) {
    const WiFiScanResult& net = networks[i];

    // Fondo de selección
    if (i == selected_index) {
      M5.Display.fillRect(0, y - 2, 240, ITEM_HEIGHT, TFT_DARKGRAY);
      M5.Display.setTextColor(TFT_BLACK);
    } else {
      M5.Display.setTextColor(TFT_WHITE);
    }

    M5.Display.setTextSize(TEXT_SIZE);
    M5.Display.setCursor(5, y);

    // Símbolo de cifrado
    M5.Display.print(net.is_open ? " " : "L");
    M5.Display.print(" ");

    // SSID (truncar a 18 chars)
    char ssid_short[20] = {};
    strncpy(ssid_short, net.ssid, 18);
    M5.Display.print(ssid_short);

    // RSSI y canal alineados a la derecha
    char rssi_str[20];
    snprintf(rssi_str, sizeof(rssi_str), "CH%d %3d", net.channel, net.rssi);

    int16_t text_width = strlen(rssi_str) * 6;  // ~6px por carácter (fuente pequeña)
    M5.Display.setCursor(240 - text_width - 5, y);
    M5.Display.print(rssi_str);

    y += ITEM_HEIGHT;
  }

  // Pie de página con instrucciones
  M5.Display.setTextSize(TEXT_SIZE);
  M5.Display.setTextColor(TFT_GRAY);
  M5.Display.setCursor(5, 135 - FOOTER_HEIGHT);
  M5.Display.print("[E]Sel [R]Scan [U/D]Nav");

  // Indicador de scroll
  if (network_count > max_visible) {
    M5.Display.setTextColor(TFT_YELLOW);
    M5.Display.setCursor(215, 135 - FOOTER_HEIGHT);
    M5.Display.printf("%d/%d", selected_index + 1, network_count);
  }
}

// =====================================================================
// Función: Renderizar medidor de RSSI
// =====================================================================
void ui_render_measuring(const WiFiScanResult network,
                         const int16_t rssi_history[],
                         uint8_t history_count) {
  M5.Display.fillScreen(TFT_BLACK);

  // Encabezado: SSID
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_CYAN);
  M5.Display.setCursor(5, 2);

  char ssid_display[30] = {};
  strncpy(ssid_display, network.ssid, 25);
  M5.Display.println(ssid_display);

  // Sección 1: RSSI numérico y barra
  int16_t y_section = 25;

  // Obtener RSSI actual (último valor del histórico)
  int16_t current_rssi = (history_count > 0) ? rssi_history[history_count - 1] : -99;
  uint8_t quality_percent = wifi_rssi_to_percent(current_rssi);
  uint32_t quality_color = wifi_get_quality_color(current_rssi);
  const char* quality_str = wifi_get_quality_string(current_rssi);

  // Mostrar RSSI en dBm
  M5.Display.setTextSize(TEXT_SIZE);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.setCursor(5, y_section);
  M5.Display.printf("RSSI: ");
  M5.Display.setTextColor(quality_color);
  M5.Display.printf("%4d dBm  (%3d%%)\n", current_rssi, quality_percent);

  // Barra de intensidad
  y_section += 12;
  ui_draw_bar(5, y_section, 230, 10, quality_percent, TFT_DARKGRAY, quality_color);

  // Etiqueta de calidad
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.setTextSize(TEXT_SIZE);
  y_section += 15;
  M5.Display.setCursor(5, y_section);
  M5.Display.printf("Calidad: ");
  M5.Display.setTextColor(quality_color);
  M5.Display.println(quality_str);

  // Sección 2: Información del AP
  y_section += 12;
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.setTextSize(TEXT_SIZE);
  M5.Display.setCursor(5, y_section);
  M5.Display.printf("Canal: %d  ", network.channel);

  if (network.is_open) {
    M5.Display.setTextColor(TFT_RED);
    M5.Display.print("ABIERTO");
  } else {
    M5.Display.setTextColor(TFT_GREEN);
    M5.Display.print("CIFRADO");
  }

  // Sección 3: Mini gráfico de histórico (si hay datos)
  if (history_count > 1) {
    y_section += 12;
    ui_draw_graph(5, y_section, 230, 25, rssi_history, history_count, -90, -30);
    y_section += 30;
  }

  // Pie de página
  M5.Display.setTextSize(TEXT_SIZE);
  M5.Display.setTextColor(TFT_GRAY);
  M5.Display.setCursor(5, 135 - FOOTER_HEIGHT);
  M5.Display.print("[B]Atras [R]Scan");
}

// =====================================================================
// Función auxiliar: Dibujar barra de progreso
// =====================================================================
void ui_draw_bar(int16_t x, int16_t y, int16_t width, int16_t height,
                 uint8_t value, uint32_t color_empty, uint32_t color_full) {
  // Borde
  M5.Display.drawRect(x, y, width, height, TFT_WHITE);

  // Relleno vacío
  M5.Display.fillRect(x + 1, y + 1, width - 2, height - 2, color_empty);

  // Relleno lleno (proporcional a value 0-100)
  uint16_t fill_width = ((width - 2) * value) / 100;
  if (fill_width > 0) {
    M5.Display.fillRect(x + 1, y + 1, fill_width, height - 2, color_full);
  }
}

// =====================================================================
// Función auxiliar: Dibujar gráfico de líneas
// =====================================================================
void ui_draw_graph(int16_t x, int16_t y, int16_t width, int16_t height,
                   const int16_t data[], uint8_t data_count,
                   int16_t min_val, int16_t max_val) {
  if (data_count < 2) return;

  // Dibujar marco del gráfico
  M5.Display.drawRect(x, y, width, height, TFT_GRAY);

  // Escala vertical
  int16_t range = max_val - min_val;  // -30 - (-90) = 60
  if (range <= 0) range = 1;

  // Dibujar línea
  for (uint8_t i = 1; i < data_count; i++) {
    // Punto anterior
    int16_t x1 = x + ((i - 1) * width) / data_count;
    int16_t y1_val = data[i - 1];
    int16_t y1 = y + height - 2 - ((y1_val - min_val) * height) / range;
    y1 = (y1 < y) ? y : (y1 > y + height) ? y + height : y1;

    // Punto actual
    int16_t x2 = x + (i * width) / data_count;
    int16_t y2_val = data[i];
    int16_t y2 = y + height - 2 - ((y2_val - min_val) * height) / range;
    y2 = (y2 < y) ? y : (y2 > y + height) ? y + height : y2;

    // Color según intensidad
    uint32_t line_color = wifi_get_quality_color(y2_val);

    // Dibujar línea
    M5.Display.drawLine(x1, y1, x2, y2, line_color);
  }

  // Etiquetas de escala (opcional)
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(TFT_GRAY);
  M5.Display.setCursor(x + 5, y + 2);
  M5.Display.printf("-30");

  M5.Display.setCursor(x + 5, y + height - 8);
  M5.Display.printf("-90");
}
