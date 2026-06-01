# WiRa v2.0

Analizador WiFi 2.4 GHz + perfilador de conexion para **M5Stack Cardputer ADV** (ESP32-S3FN8).

Combina dos herramientas en un solo firmware:
- **Modo Radar**: escanea redes WiFi cercanas y mide la intensidad RSSI en tiempo real con grafica historica
- **Modo Profiler**: conecta a un AP propio, analiza la red local y el backhaul, clasifica si la salida a internet es **CELULAR**, **ISP FIJO** o **INDETERMINADO** con porcentaje de confianza

Solo diagnostico pasivo. No incluye captura de trafico ajeno, deauth ni modo monitor.

![Platform](https://img.shields.io/badge/Platform-ESP32--S3-blue)
![Framework](https://img.shields.io/badge/Framework-Arduino-teal)
![Device](https://img.shields.io/badge/Device-Cardputer%20ADV-red)
![Version](https://img.shields.io/badge/Version-2.0-brightgreen)

---

## Hardware objetivo

| Componente | Detalle |
|------------|---------|
| Dispositivo | M5Stack Cardputer ADV |
| SoC | ESP32-S3FN8 (dual-core LX7, 240 MHz) |
| Memoria | 512 KB SRAM, 8 MB Flash, **sin PSRAM** |
| Pantalla | IPS 1.14" 240x135 (ST7789V2) |
| Teclado | 56 teclas matriz 4x14 |
| WiFi | 2.4 GHz 802.11 b/g/n |
| Storage | microSD SPI (CS=GPIO5, MOSI=14, MISO=39, SCK=40) |

---

## Maquina de estados

7 estados implementados (`enum AppState` en `WiRa.ino`):

| Estado | Funcion |
|--------|---------|
| `STATE_SCANNING` | Escaneo inicial de redes WiFi (auto cada 10s) |
| `STATE_LIST` | Lista de APs detectados, navegacion |
| `STATE_MEASURING` | Medidor RSSI tiempo real del AP seleccionado |
| `STATE_CONNECTING` | Pantalla password + intento de conexion |
| `STATE_PROFILING` | Ejecucion del perfil completo (3 fases) |
| `STATE_PROFILE_VIEW` | Resultado del perfilado con veredicto |
| `STATE_ERROR` | Mensaje de error + opcion volver |

---

## Modulos del proyecto

```
WiRa/
└── WiRa/                       # carpeta sketch Arduino
    ├── WiRa.ino                # Main: setup, loop, maquina de estados
    ├── wifi_scan.h/.cpp        # Scan WiFi + medicion RSSI por canal
    ├── wifi_connect.h/.cpp     # Conexion robusta (BSSID dirigido + reintento)
    ├── net_profile.h/.cpp      # Perfil red local + internet + clasificador
    ├── ui_render.h/.cpp        # Render con sprite anti-flicker
    ├── keyboard_input.h/.cpp   # Teclado con raw mode + key repeat
    ├── storage.h/.cpp          # Persistencia microSD (JSON + CSV)
    └── WiRa.bin                # Binario compilado
```

---

## Modo Radar (medidor RSSI)

### Lista de APs (`STATE_LIST`)
Muestra hasta 50 APs detectados con:
- Etiqueta de cifrado (`OPEN`, `WEP`, `WPA`, `WPA2`, `WPA*`, `WPA3`, `W2/3`, `ENT`)
- SSID truncado a 18 caracteres
- Canal y RSSI (`CH6 -45`)
- Indicador de scroll `^` `v` cuando hay mas elementos
- Contador `N/Total` en footer

### Medidor RSSI (`STATE_MEASURING`)
Tras seleccionar un AP con `Enter`:
- Valor RSSI numerico en dBm con codigo de color
- Porcentaje calculado (0-100%)
- Barra horizontal de intensidad
- Etiqueta de calidad (Excelente/Bueno/Aceptable/Debil/Muy debil)
- Canal + cifrado + BSSID formateado
- Grafica historica de las ultimas 30 muestras (refresca cada 350ms)

**Optimizacion clave:** `wifi_measure_rssi()` escanea **solo el canal del AP** (~100ms), no todos los canales (~1300ms).

---

## Modo Profiler (clasificador)

### Conexion (`STATE_CONNECTING`)
- Al pulsar `C` sobre un AP de la lista
- Si es OPEN: conecta directo
- Si es cifrado: caja de entrada con teclado fisico
  - Caracteres literales en modo raw (incluyendo `,` `.` `;` `/`)
  - `Bksp` borra, `Enter` conecta, `` ` `` cancela

**Conexion robusta** (`wifi_connect.cpp`):
1. `WiFi.disconnect(true, true)` + delay 300ms
2. `WiFi.persistent(false)` (no guarda credenciales)
3. `WiFi.setSleep(false)` (mejor RTT)
4. **Intento 1**: dirigido con BSSID + canal explicito
5. **Intento 2** (si falla): sin BSSID (algunos routers rechazan dirigido)
6. Timeout 15s por intento
7. Detecta `WL_NO_SSID_AVAIL` y `WL_CONNECT_FAILED` rapido

### Perfilado (`STATE_PROFILING`)
Barra de progreso con 3 fases (`net_profile_full()`):

| Fase | Modulo | Contenido |
|------|--------|-----------|
| 1/3 | `net_profile_local()` | IP local, mascara, CIDR, gateway, DNS1/2, IPv6, clasificacion de subnet |
| 2/3 | `net_profile_internet()` | Ping a 1.1.1.1 (4 muestras), IP publica via ip-api.com, ASN, ISP, pais, tipo, CGNAT |
| 3/3 | `net_profile_classify()` | Scoring ponderado + veredicto |

### Vista de perfil (`STATE_PROFILE_VIEW`)
- **Veredicto grande** con color: amarillo (CELULAR), verde (ISP FIJO), gris (INDETERMINADO)
- **Confianza %** con barra horizontal
- Datos en 3 filas:
  ```
  IP: 203.x.x.x    ASN: AS3462
  Sub: /28  CGN: SI  Mob: SI
  Png: 45ms  Tipo: mobile
  ```
- Hint del hotspot detectado al final
- `S` guarda perfil a microSD; mensaje confirmacion 2s

---

## Clasificacion subnet (modulo B)

Discriminador fuerte basado en la IP asignada por DHCP:

| Rango | Hint | is_likely_mobile_hotspot |
|-------|------|--------------------------|
| `172.20.10.0/28` | "iPhone hotspot" | true |
| `192.168.43.0/24` | "Android hotspot" | true |
| Subnet `/28` o mayor | "Posible hotspot" | true |
| Otro `192.168.x` / `10.x` | "Router/AP" | false |

---

## Motor de scoring (clasificador)

Implementado en `net_profile_classify()`:

| Senal | Peso | Direccion |
|-------|------|-----------|
| Subnet iPhone hotspot detectada | +60 | CELULAR |
| Subnet Android hotspot detectada | +45 | CELULAR |
| Otra subnet pequena `/28+` | +25 | CELULAR (debil) |
| Subnet de router (resto) | -30 | ISP FIJO |
| Flag `mobile` de ip-api.com | +40 | CELULAR |
| CGNAT detectado (`100.64.0.0/10`) | +25 | CELULAR |
| Ping > 80ms | +10 | CELULAR (debil) |
| Ping < 30ms | -15 | ISP FIJO |

**Umbrales del veredicto:**
- `score >= 50` -> **CELULAR**
- `score <= -20` -> **ISP FIJO**
- en medio -> **INDETERMINADO**

---

## Persistencia (microSD)

### Pinout (corregido en v2.0)
```
CS   = GPIO 5    (antes era 12 - BUG)
MOSI = GPIO 14
MISO = GPIO 39
SCK  = GPIO 40
```
Fuente: [docs.m5stack.com/en/core/Cardputer-Adv](https://docs.m5stack.com/en/core/Cardputer-Adv)

### Archivos generados (al pulsar `S` en vista de perfil)

| Archivo | Formato | Contenido |
|---------|---------|-----------|
| `/wira/log.csv` | CSV | Una fila por perfil: ts, ssid, bssid, verdict, confidence, ip publica, asn, mobile, cgnat, ping, subnet, hotspot |
| `/wira/p_<ts>.json` | JSON | Perfil completo individual con local + internet + verdict |

Si la SD no esta presente, `storage_init()` retorna false y los datos solo viven en memoria.

---

## Controles (teclado fisico)

### En lista de APs
| Tecla | Accion |
|-------|--------|
| `;` o `U/u` | Arriba |
| `.` o `D/d` | Abajo |
| `Enter` o `E/e` | Medidor RSSI |
| `C/c` | Conectar y perfilar |
| `R/r` | Re-escanear |

### En medidor RSSI
| Tecla | Accion |
|-------|--------|
| `B/b` o `Bksp` | Volver a lista |
| `R/r` | Re-escanear |

### En entrada de password (modo raw)
| Tecla | Accion |
|-------|--------|
| Cualquier char ASCII | Insertar (incluye `, . ; /`) |
| `Bksp` | Borrar ultimo char |
| `Enter` | Conectar |
| `` ` `` | Cancelar y volver |

### En vista de perfil
| Tecla | Accion |
|-------|--------|
| `B/b` | Desconectar y volver |
| `R/r` | Re-perfilar |
| `S/s` | Guardar a microSD |

### Key repeat
- Delay inicial: 500ms
- Intervalo entre repeticiones: 120ms
- Aplica solo a flechas (UP/DOWN/LEFT/RIGHT)
- Teclas de accion (Enter, C, R, S, B) emiten un solo evento por pulsacion

---

## UI con sprite anti-flicker

Implementado en `ui_render.cpp`:
- Sprite `M5Canvas` 240x135 @ 16bpp creado en `ui_init_canvas()`
- Fallback a 8bpp si no hay heap suficiente
- Todas las funciones de render dibujan en el sprite
- Push al display al final con `pushSprite(0, 0)`
- Render condicional en `STATE_MEASURING` (solo cuando hay datos nuevos o input)

**Coste:** ~65 KB SRAM (240 * 135 * 2 bytes).

---

## Instalacion

### Requisitos

- Arduino IDE 2.x o Arduino CLI
- ESP32 Core >= 3.0.0
- Librerias:
  - M5Cardputer
  - M5Unified
  - M5GFX
  - ArduinoJson v7
  - ESP32Ping (GitHub: marian-craciunescu/ESP32Ping)

### Configuracion Arduino IDE

| Opcion | Valor obligatorio |
|--------|-------------------|
| Placa | **ESP32S3 Dev Module** |
| PSRAM | **Disabled** |
| Flash Size | 8MB |
| Partition Scheme | 8M with spiffs (3MB APP / 1.5MB SPIFFS) |
| USB CDC On Boot | Enabled |
| CPU Frequency | 240MHz |
| Upload Speed | 115200 (subir despues si va bien) |

### Cargar binario precompilado

1. Descarga `WiRa/WiRa.bin` de este repositorio
2. Conecta el Cardputer ADV via USB-C
3. En Arduino IDE: Herramientas -> Placa -> ESP32S3 Dev Module
4. Selecciona el puerto COM
5. Sketch -> Subir (Ctrl+U)

O por linea de comandos:
```bash
esptool.py --chip esp32s3 --port COM3 write_flash 0x0 WiRa.bin
```

### Compilar desde fuente

```bash
git clone https://github.com/Nicolas-Losada/WiRa_CardputerADV.git
cd WiRa_CardputerADV/WiRa
# Abrir WiRa.ino en Arduino IDE, Ctrl+U
```

---

## Uso de recursos

Reportado por el compilador (Arduino IDE):

| Recurso | Valor |
|---------|-------|
| Flash | **1170 KB (35%)** de 3.3 MB |
| RAM global | **53 KB (16%)** de 327 KB |
| Sprite canvas | ~65 KB SRAM |
| MAX_NETWORKS | 50 |
| Historico RSSI | 30 muestras |
| Buffer password | 64 chars |

---

## Limitaciones conocidas

- Solo WiFi **2.4 GHz** (limitacion hardware ESP32-S3)
- Hotspots en 5 GHz no detectables
- API ip-api.com gratuita: limite 45 req/min, solo HTTP (sin TLS)
- Si el AP cambia su BSSID entre scan y conexion, la conexion dirigida falla y entra al reintento sin BSSID
- IPv6 solo se detecta link-local (sin NAT64 detection en esta version)
- Sin RTC: archivos JSON se nombran por timestamp `millis()`, no por fecha real

---

## Bugs corregidos en v2.0

| Bug | Fix |
|-----|-----|
| `wifi_measure_rssi` escaneaba todos los canales (~1300ms) | Scan por canal especifico (~100ms) |
| `String ssid` en bucle de scan fragmentaba heap | Reemplazado por `strlcpy` directo |
| `strncpy` sin null-terminator | Garantizado con buffer + `\0` final |
| Pantalla con flicker al redibujar | Sprite `M5Canvas` con pushSprite |
| Tab bar excedia 240px de ancho | Indicador centrado con `textWidth()` real |
| Conexion fallaba en routers con multi-BSSID | BSSID + canal dirigido + reintento |
| **SD_CS=12 incorrecto** | **Corregido a GPIO 5** (fuente: docs M5Stack) |

---

## Solucion de problemas

### El AP de mi celular no aparece en el scan
- Verifica que el hotspot esta en **2.4 GHz** (Xiaomi/Samsung suelen poner 5 GHz por defecto)
- En ajustes del hotspot busca "Banda preferida" y selecciona 2.4 GHz
- Algunos canales (12/13) requieren `setCountry` permisivo - el firmware ya lo hace

### No puedo conectar a mi router fijo
- Verifica la contrasena: el modo raw permite ver cada caracter como se inserta
- Algunos routers WPA3 puros pueden tardar mas que el timeout - el firmware reintenta sin BSSID dirigido
- Si el router cambia su BSSID dinamicamente, vuelve a la lista y re-escanea

### Compilacion falla con "WrongChip"
- Asegurate de seleccionar **ESP32S3 Dev Module**, no `ESP32 Dev Module` generico
- PSRAM debe estar en **Disabled** (el ESP32-S3FN8 no la tiene)

### microSD no se detecta
- Verifica que la tarjeta esta en FAT32
- Confirma que es microSD (no SDXC de mas de 32 GB)
- El pinout correcto es CS=5 (no 12 como en versiones previas)

### Pantalla con artefactos o flicker
- Si el sprite no se pudo crear, hay fallback a 8bpp (se ve mas pixelado pero funciona)
- Revisa el Serial Monitor para ver el log `[UI]`

---

## Creditos

- Hardware: [M5Stack Cardputer ADV](https://shop.m5stack.com/products/m5stack-cardputer-adv-version-esp32-s3)
- Librerias: [M5Cardputer](https://github.com/m5stack/M5Cardputer), [M5Unified](https://github.com/m5stack/M5Unified), [M5GFX](https://github.com/m5stack/M5GFX), [ArduinoJson](https://github.com/bblanchon/ArduinoJson), [ESP32Ping](https://github.com/marian-craciunescu/ESP32Ping)
- APIs externas: [ip-api.com](http://ip-api.com), [ipify.org](https://api.ipify.org)
- Pinout SD: [docs.m5stack.com/en/core/Cardputer-Adv](https://docs.m5stack.com/en/core/Cardputer-Adv)
- Desarrollado con asistencia de Claude (Anthropic)

---

## Licencia

MIT - libre para uso educativo y de diagnostico personal de redes propias.
