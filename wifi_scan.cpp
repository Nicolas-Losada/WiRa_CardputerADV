/*
  =====================================================================
  MÓDULO: WiFi Scan (Implementación)
  =====================================================================
  Ver wifi_scan.h para documentación de funciones

  =====================================================================
*/

#include "wifi_scan.h"
#include <Arduino.h>
#include <M5GFX.h>

// =====================================================================
// Función: Escanear todas las redes WiFi
// =====================================================================
uint16_t wifi_scan_networks(WiFiScanResult results[], uint16_t max_results) {
  if (!results || max_results == 0) {
    return 0;
  }

  Serial.println("[WiFi Scan] Iniciando escaneo asincrónico...");

  // Iniciar escaneo asincrónico (no bloqueante en teoría, pero esperamos)
  int networks_found = WiFi.scanNetworks(
    false,  // async = false (bloqueante, más simple)
    true,   // show_hidden = true (mostrar redes ocultas)
    false,  // passive = false (activo, más rápido)
    300U    // max_ms = 300ms por canal
  );

  Serial.printf("[WiFi Scan] Escaneo completado. Redes encontradas: %d\n", networks_found);

  if (networks_found <= 0) {
    Serial.println("[WiFi Scan] No se encontraron redes WiFi");
    return 0;
  }

  // Limitar a max_results
  uint16_t count = (networks_found > max_results) ? max_results : networks_found;

  // Procesar y almacenar resultados
  for (uint16_t i = 0; i < count; i++) {
    // SSID
    String ssid = WiFi.SSID(i);
    if (ssid.length() > SSID_MAX_LEN - 1) {
      ssid = ssid.substring(0, SSID_MAX_LEN - 1);
    }
    ssid.toCharArray(results[i].ssid, SSID_MAX_LEN);

    // BSSID (MAC del AP)
    uint8_t* bssid = WiFi.BSSID(i);
    if (bssid) {
      memcpy(results[i].bssid, bssid, BSSID_LEN);
    }

    // RSSI (intensidad de señal)
    results[i].rssi = WiFi.RSSI(i);

    // Canal
    results[i].channel = WiFi.channel(i);

    // Cifrado (true = abierto, false = cifrado)
    // WiFi.encryptionType() retorna valores como WIFI_AUTH_OPEN, etc.
    wifi_auth_mode_t auth = WiFi.encryptionType(i);
    results[i].is_open = (auth == WIFI_AUTH_OPEN);

    Serial.printf("[WiFi Scan] [%d] SSID: %-32s | CH: %2d | RSSI: %4d dBm | %s\n",
                  i,
                  results[i].ssid,
                  results[i].channel,
                  results[i].rssi,
                  results[i].is_open ? "ABIERTO" : "CIFRADO");
  }

  // Limpiar memoria del escaneo WiFi
  WiFi.scanDelete();

  return count;
}

// =====================================================================
// Función: Medir RSSI de un AP específico
// =====================================================================
int16_t wifi_measure_rssi(const char* ssid, const uint8_t* bssid, uint8_t channel) {
  if (!ssid || !bssid) {
    return -99;
  }

  // Iniciar un nuevo escaneo (buscando el canal específico)
  // Para medir solo un AP, podríamos hacer escaneo pasivo en ese canal,
  // pero por simplicidad hacemos escaneo general y buscamos ese BSSID

  int networks = WiFi.scanNetworks(
    false,  // async = false
    true,   // show_hidden
    false,  // passive
    100U    // max_ms (rápido)
  );

  int16_t measured_rssi = -99;  // Valor por defecto si no se encuentra

  if (networks > 0) {
    for (int i = 0; i < networks; i++) {
      uint8_t* found_bssid = WiFi.BSSID(i);

      // Comparar BSSID (6 bytes)
      if (found_bssid && memcmp(found_bssid, bssid, BSSID_LEN) == 0) {
        measured_rssi = WiFi.RSSI(i);
        break;
      }
    }
  }

  WiFi.scanDelete();

  return measured_rssi;
}

// =====================================================================
// Función: Obtener descripción de calidad de señal
// =====================================================================
const char* wifi_get_quality_string(int16_t rssi) {
  // Rangos aproximados:
  // -30 a -67: Excelente (muy cerca)
  // -68 a -70: Bueno
  // -71 a -80: Aceptable
  // -81 a -90: Débil
  // < -90: Muy débil / No disponible

  if (rssi >= -67) {
    return "Excelente";
  } else if (rssi >= -70) {
    return "Bueno";
  } else if (rssi >= -80) {
    return "Aceptable";
  } else if (rssi >= -90) {
    return "Debil";
  } else {
    return "Muy debil";
  }
}

// =====================================================================
// Función: Obtener color según calidad de señal
// =====================================================================
uint32_t wifi_get_quality_color(int16_t rssi) {
  if (rssi >= -67) {
    return TFT_GREEN;      // Verde: Excelente
  } else if (rssi >= -70) {
    return TFT_GREEN;      // Verde: Bueno
  } else if (rssi >= -80) {
    return TFT_YELLOW;     // Amarillo: Aceptable
  } else if (rssi >= -90) {
    return TFT_ORANGE;     // Naranja: Débil
  } else {
    return TFT_RED;        // Rojo: Muy débil
  }
}

// =====================================================================
// Función: Convertir RSSI a porcentaje (0-100%)
// =====================================================================
uint8_t wifi_rssi_to_percent(int16_t rssi) {
  // Escala aproximada:
  // -30 dBm = 100% (muy fuerte)
  // -90 dBm = 0% (muy débil)

  if (rssi >= -30) return 100;
  if (rssi <= -90) return 0;

  // Interpolación lineal: (rssi - (-90)) / ((-30) - (-90)) * 100
  int16_t range = 60;  // -30 - (-90) = 60
  int16_t offset = rssi + 90;  // Desplazar a escala 0-60
  uint8_t percent = (offset * 100) / range;

  return (percent > 100) ? 100 : percent;
}
