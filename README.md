# WiRa - WiFi Radar Analyzer

Analizador de redes WiFi 2.4 GHz para **M5Stack Cardputer ADV** (ESP32-S3).

Herramienta de diagnóstico pasivo que escanea, lista y mide la intensidad de señal de puntos de acceso WiFi en tiempo real.

![Platform](https://img.shields.io/badge/Platform-ESP32--S3-blue)
![Framework](https://img.shields.io/badge/Framework-Arduino-teal)
![License](https://img.shields.io/badge/License-MIT-green)

---

## Funcionalidades

- Escaneo de todos los puntos de acceso WiFi 2.4 GHz del entorno
- Lista de redes con SSID, canal, RSSI (dBm) y estado de cifrado
- Medidor de intensidad de señal en tiempo real (~350ms de refresco)
- Barra visual con colores por calidad (verde/amarillo/naranja/rojo)
- Historial grafico de las ultimas 30 mediciones de RSSI
- Navegacion completa por teclado fisico del Cardputer

---

## Hardware requerido

| Componente | Detalle |
|------------|---------|
| Dispositivo | M5Stack Cardputer ADV |
| SoC | ESP32-S3FN8 (dual-core, 240 MHz) |
| Pantalla | LCD 1.14" 240x135 px (ST7789) |
| Teclado | 56 teclas, matriz 4x14 |
| Radio | WiFi 2.4 GHz (solo lectura pasiva) |

---

## Controles

| Tecla | Accion |
|-------|--------|
| `U` | Navegar arriba en la lista |
| `D` | Navegar abajo en la lista |
| `E` | Seleccionar AP (entrar al medidor) |
| `B` | Volver atras (medidor a lista) |
| `R` | Rescanear redes WiFi |

---

## Pantallas

### 1. Escaneo
Pantalla de carga mientras se detectan las redes WiFi disponibles.

### 2. Lista de APs
Muestra todas las redes encontradas con:
- **SSID** (nombre de la red)
- **Canal** WiFi (1-13)
- **RSSI** en dBm
- **Cifrado**: `L` = cifrado, vacio = abierto
- **Contador**: posicion / total (ej: 1/27)

### 3. Medidor de senal
Vista detallada del AP seleccionado:
- Valor RSSI numerico en dBm
- Porcentaje de fuerza (0-100%)
- Barra de intensidad con codigo de color
- Etiqueta de calidad (Excelente/Bueno/Aceptable/Debil/Muy debil)
- Grafico historico de las ultimas 30 mediciones

#### Escala de calidad

| RSSI (dBm) | Calidad | Color |
|------------|---------|-------|
| >= -67 | Excelente | Verde |
| -68 a -70 | Bueno | Verde |
| -71 a -80 | Aceptable | Amarillo |
| -81 a -90 | Debil | Naranja |
| < -90 | Muy debil | Rojo |

---

## Estructura del proyecto

```
WiRa/
├── Frimware.ino         # Main: setup, loop, maquina de estados
├── wifi_scan.h          # Header: escaneo WiFi y medicion RSSI
├── wifi_scan.cpp        # Implementacion: escaneo y utilidades
├── ui_render.h          # Header: renderizado de pantalla
├── ui_render.cpp        # Implementacion: lista, medidor, graficos
├── keyboard_input.h     # Header: entrada de teclado
├── keyboard_input.cpp   # Implementacion: lectura de teclas
├── platformio.ini       # Configuracion PlatformIO (alternativa)
├── firmware.bin         # Binario compilado listo para cargar
└── README.md
```

---

## Instalacion

### Opcion A: Cargar binario precompilado

1. Descarga `firmware.bin` de este repositorio
2. Conecta el Cardputer ADV via USB-C
3. Usa **M5Burner** o **esptool** para cargar:

```bash
esptool.py --chip esp32s3 --port COM3 write_flash 0x0 firmware.bin
```

### Opcion B: Compilar desde codigo fuente

**Requisitos:**
- Arduino IDE 2.x o Arduino CLI
- ESP32 Core >= 3.0.0
- Librerias: M5Unified, M5GFX

**Pasos:**

1. Clona el repositorio:
```bash
git clone https://github.com/TU_USUARIO/WiRa_CardputerADV.git
```

2. Abre el proyecto en Arduino IDE:
   - Archivo > Abrir > Selecciona la carpeta `WiRa`

3. Configura la placa:
   - Herramientas > Placa > `ESP32 Dev Module` o `M5Stack Cardputer`
   - Herramientas > Puerto > Selecciona tu COM

4. Compila y sube:
   - `Ctrl+U` o Sketch > Subir

### Opcion C: PlatformIO

```bash
pio run --target upload
```

---

## Uso de recursos

| Recurso | Uso |
|---------|-----|
| Flash | ~1050 KB (80%) |
| RAM | ~50 KB (15%) |
| Refresco UI | ~33 FPS |
| Redes max | 50 APs |
| Historico | 30 muestras |

---

## Limitaciones

- Solo WiFi 2.4 GHz (limitacion del hardware ESP32-S3)
- Lectura pasiva unicamente (sin inyeccion, captura ni ataques)
- Si un AP desaparece, el RSSI muestra -99 dBm
- Navegacion por letras del teclado (U/D/E/B/R), no flechas de cursor

---

## Licencia

Este proyecto es de uso libre para fines educativos y de diagnostico.

---

## Creditos

- Hardware: [M5Stack Cardputer ADV](https://shop.m5stack.com/products/m5stack-cardputer-adv-version-esp32-s3)
- Librerias: [M5Unified](https://github.com/m5stack/M5Unified), [M5GFX](https://github.com/m5stack/M5GFX)
- Desarrollado con asistencia de Claude (Anthropic)
