/*
  =====================================================================
  MÓDULO: UI Render
  =====================================================================
  Función: Renderizado de pantalla (lista, medidor, escaneo)

  Proporciona:
  - ui_render_scanning(): Pantalla de escaneo en progreso
  - ui_render_list(): Menú de selección de AP
  - ui_render_measuring(): Medidor en tiempo real de RSSI

  Optimizado para 240x135 px (M5Stack Cardputer ADV)

  =====================================================================
*/

#ifndef UI_RENDER_H
#define UI_RENDER_H

#include <M5Unified.h>
#include <M5GFX.h>
#include "wifi_scan.h"

// =====================================================================
// FUNCIONES PÚBLICAS
// =====================================================================

/**
 * Renderiza la pantalla de escaneo en progreso.
 * Muestra un spinner y el mensaje "Escaneando redes WiFi..."
 */
void ui_render_scanning();

/**
 * Renderiza el menú de lista de APs.
 *
 * Muestra:
 * - Encabezado con instrucciones
 * - Lista de APs (máx 5-6 visibles en 135px)
 * - AP seleccionado resaltado
 * - Indicador de cifrado/abierto
 * - RSSI visual (barra o número)
 *
 * @param networks Array de redes escaneadas
 * @param network_count Cantidad de redes
 * @param selected_index Índice del AP seleccionado
 */
void ui_render_list(const WiFiScanResult networks[],
                    uint16_t network_count,
                    uint16_t selected_index);

/**
 * Renderiza el medidor en tiempo real de RSSI.
 *
 * Muestra:
 * - Nombre del AP (SSID)
 * - Valor de RSSI en dBm
 * - Barra de intensidad con código de color (rojo-amarillo-verde)
 * - Porcentaje de fuerza
 * - Pequeño gráfico de histórico (últimas 30 mediciones)
 * - Instrucciones de navegación (BACK, RESCAN)
 *
 * @param network Red seleccionada
 * @param rssi_history Array de histórico de RSSI
 * @param history_count Cantidad de valores en el histórico
 */
void ui_render_measuring(const WiFiScanResult network,
                         const int16_t rssi_history[],
                         uint8_t history_count);

/**
 * Dibuja una barra horizontal de progreso/intensidad.
 *
 * @param x Posición X
 * @param y Posición Y
 * @param width Ancho de la barra
 * @param height Alto de la barra
 * @param value Valor actual (0-100)
 * @param color_1 Color de la barra (fondo)
 * @param color_2 Color del progreso
 */
void ui_draw_bar(int16_t x, int16_t y, int16_t width, int16_t height,
                 uint8_t value, uint32_t color_1, uint32_t color_2);

/**
 * Dibuja un pequeño gráfico de líneas (histórico de RSSI).
 *
 * @param x Posición X
 * @param y Posición Y
 * @param width Ancho del gráfico
 * @param height Alto del gráfico
 * @param data Array de valores
 * @param data_count Cantidad de valores
 * @param min_val Mínimo esperado (-90)
 * @param max_val Máximo esperado (-30)
 */
void ui_draw_graph(int16_t x, int16_t y, int16_t width, int16_t height,
                   const int16_t data[], uint8_t data_count,
                   int16_t min_val, int16_t max_val);

#endif  // UI_RENDER_H
