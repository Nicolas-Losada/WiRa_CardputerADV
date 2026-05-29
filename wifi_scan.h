/*
  =====================================================================
  MÓDULO: WiFi Scan
  =====================================================================
  Función: Escaneo de redes WiFi 2.4 GHz y medición de RSSI

  Proporciona:
  - wifi_scan_networks(): Escanea y retorna lista de APs
  - wifi_measure_rssi(): Mide intensidad de un AP específico
  - Estructura para almacenar datos de red

  =====================================================================
*/

#ifndef WIFI_SCAN_H
#define WIFI_SCAN_H

#include <WiFi.h>
#include <cstring>

// Constantes
#define MAX_NETWORKS 50
#define SSID_MAX_LEN 32
#define BSSID_LEN 6

// =====================================================================
// ESTRUCTURA: Resultado de escaneo WiFi
// =====================================================================
struct WiFiScanResult {
  char ssid[SSID_MAX_LEN];      // Nombre de la red
  uint8_t bssid[BSSID_LEN];     // Dirección MAC del AP
  int16_t rssi;                 // Intensidad de señal (dBm)
  uint8_t channel;              // Canal WiFi (1-13)
  bool is_open;                 // true = sin cifrado, false = cifrado

  WiFiScanResult() : rssi(0), channel(0), is_open(false) {
    memset(ssid, 0, SSID_MAX_LEN);
    memset(bssid, 0, BSSID_LEN);
  }
};

// =====================================================================
// FUNCIONES PÚBLICAS
// =====================================================================

/**
 * Escanea todas las redes WiFi disponibles en el área.
 *
 * @param results Buffer de salida (array de WiFiScanResult)
 * @param max_results Cantidad máxima de redes a retornar
 * @return Número de redes encontradas (0 si ninguna)
 */
uint16_t wifi_scan_networks(WiFiScanResult results[], uint16_t max_results);

/**
 * Mide el RSSI de un AP específico.
 *
 * Realiza un escaneo rápido (solo ese canal) para obtener el RSSI actual.
 * Si el AP no está disponible, retorna -99 dBm (muy débil/no encontrado).
 *
 * @param ssid Nombre de la red (SSID)
 * @param bssid Dirección MAC del AP (6 bytes)
 * @param channel Canal WiFi donde está el AP
 * @return RSSI en dBm (-30 a -90 típicamente; -99 = no encontrado)
 */
int16_t wifi_measure_rssi(const char* ssid, const uint8_t* bssid, uint8_t channel);

/**
 * Convierte RSSI (dBm) a descripción de calidad.
 *
 * @param rssi Valor de RSSI en dBm
 * @return Cadena descriptiva ("Excelente", "Bueno", "Débil", etc.)
 */
const char* wifi_get_quality_string(int16_t rssi);

/**
 * Convierte RSSI a color (para representación visual).
 *
 * @param rssi Valor de RSSI en dBm
 * @return Color en formato TFT_* (TFT_GREEN, TFT_YELLOW, TFT_RED, etc.)
 */
uint32_t wifi_get_quality_color(int16_t rssi);

/**
 * Obtiene porcentaje de fuerza de señal (0-100%).
 *
 * @param rssi Valor de RSSI en dBm
 * @return Porcentaje (0-100)
 */
uint8_t wifi_rssi_to_percent(int16_t rssi);

#endif  // WIFI_SCAN_H
