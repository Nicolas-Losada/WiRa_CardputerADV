# WiRa v2.0 - WiFi Radar + Network Profiler

Analizador WiFi 2.4 GHz + perfilador de conexion + clasificador celular/ISP fijo para **M5Stack Cardputer ADV** (ESP32-S3FN8).

Herramienta de diagnostico pasivo: escanea APs, mide RSSI, conecta a redes propias, analiza el backhaul (IP publica, ASN, CGNAT) y determina si la conexion es de operadora celular o ISP fijo con un puntaje de confianza.

![Platform](https://img.shields.io/badge/Platform-ESP32--S3-blue)
![Framework](https://img.shields.io/badge/Framework-Arduino-teal)
![Device](https://img.shields.io/badge/Device-Cardputer%20ADV-red)
![Version](https://img.shields.io/badge/Version-2.0-brightgreen)
![License](https://img.shields.io/badge/License-MIT-green)

---

## Que hay nuevo en v2.0

| Feature | v1.x | v2.0 |
|---------|------|------|
| Renderizado | flicker en cada redraw | **sprite 240x135 anti-flicker** |
| Scan medidor RSSI | scan todos canales (~1300ms) | **canal especifico (~100ms)** |
| Cifrado mostrado | solo abierto/cerrado | **WPA2/WPA3/WEP/OPEN/ENT** |
| BSSID | no visible | **MAC formateada en medidor** |
| Scroll lista | sin indicador | **flechas ^v + contador N/T** |
| Bateria | no mostrada | **%color en header todas vistas** |
| Teclado | 1 evento por pulsacion | **key repeat: 500ms+120ms** |
| Conexion a AP | no soportada | **conectar + perfilar** |
| Perfilado red | no | **local + internet + clasificador** |
| Persistencia | no | **JSON+CSV a microSD** |
| Estados | 3 (scan, list, RSSI) | **7 (boot, scan, list, RSSI, password, connect, profiling, report, error)** |

---

## Que hace

WiRa tiene **dos modos** operativos:

### Modo Radar (clasico)
1. Escanea WiFi 2.4 GHz - lista todos los APs con SSID, canal, RSSI, cifrado
2. Selecciona un AP -> entra al **medidor RSSI tiempo real**
3. Grafica historica de las ultimas 30 muestras con codigo de color

### Modo Profiler (v2.0)
1. Selecciona un AP con `C` -> entrada contrasena por teclado fisico
2. Conecta como station -> recolecta perfil completo:
   - **Enlace WiFi:** SSID, BSSID, fabricante (OUI), RSSI, canal, cifrado
   - **Red local:** IP, mascara/CIDR, gateway, DNS, clasificacion subnet
   - **Internet:** IP publica, ASN, ISP, tipo (movil/residencial/hosting), pais, CGNAT, RTT/jitter
3. Clasificador con scoring -> veredicto **CELULAR / ISP FIJO / INDETERMINADO** + % confianza
4. Guarda perfil a microSD (JSON detallado + CSV resumen)

---

## Hardware

| Componente | Detalle |
|------------|---------|
| Dispositivo | M5Stack Cardputer ADV |
| SoC | ESP32-S3FN8 (dual-core Xtensa LX7, 240 MHz) |
| Memoria | 512 KB SRAM, 8 MB Flash, **sin PSRAM** |
| Pantalla | IPS 1.14" 240x135 (ST7789V2) |
| Teclado | 56 teclas matriz 4x14 |
| WiFi | 2.4 GHz 802.11 b/g/n (WiFi 4) |
| Storage | microSD SPI (CS=GPIO5, MOSI=14, MISO=39, SCK=40) |
| Bateria | 1750 mAh |

---

## Controles

### Lista de APs
| Tecla | Accion |
|-------|--------|
| `;` / `U` | Arriba (con key repeat) |
| `.` / `D` | Abajo (con key repeat) |
| `Enter` / `E` | Medidor RSSI clasico |
| `C` | **Conectar y perfilar** |
| `R` | Re-escanear |

### Pantalla password
| Tecla | Accion |
|-------|--------|
| `0-9 a-z` etc. | Insertar caracter |
| `, . ; /` | Caracteres literales (modo raw) |
| `Bksp` | Borrar |
| `Enter` | Conectar |
| `` ` `` | Cancelar |

### Vista de perfil (reporte)
| Tecla | Accion |
|-------|--------|
| `B` / `Bksp` | Volver a lista |
| `R` | Re-perfilar |
| `S` | Guardar a microSD |

### Medidor RSSI
| Tecla | Accion |
|-------|--------|
| `B` | Volver a lista |
| `R` | Re-escanear |

---

## Pantallas

| Estado | Descripcion |
|--------|-------------|
| **SCANNING** | Spinner + "Escaneando redes WiFi..." |
| **LIST** | Lista APs con `AUTH SSID CH RSSI`, scroll indicators ^v |
| **MEASURING** | Valor RSSI dBm + %, barra color, calidad textual, BSSID, grafico 30 muestras |
| **PASSWORD** | Caja entrada texto + contador caracteres |
| **CONNECTING** | "Conectando: SSID  Intento N..." |
| **PROFILING** | Barra progreso 3/3: Red local -> Internet -> Clasificando |
| **PROFILE_VIEW** | Veredicto grande + confianza + IP/ASN/Sub/CGN/Mob/Ping/Tipo |
| **ERROR** | Mensaje + opcion volver |

---

## Modulos de analisis

### A - Enlace WiFi local
- SSID + BSSID (MAC AP)
- Fabricante por OUI lookup
- RSSI dBm, canal 1-13
- Cifrado: OPEN, WEP, WPA, WPA2, WPA3, WPA2/3, ENT
- Soporta SSID ocultos

### B - Red local (DHCP)
- IP local, mascara, prefijo CIDR
- Gateway, DNS1, DNS2
- IPv6 link-local
- **Clasificacion subnet** (discriminador fuerte):
  - `172.20.10.0/28` -> iPhone hotspot
  - `192.168.43.0/24` -> Android hotspot legacy
  - `192.168.x` o `10.x` -> Router/AP

### C - Internet / backhaul
- IP publica via [api.ipify.org](https://api.ipify.org)
- ASN + ISP via [ip-api.com](http://ip-api.com) (limite 45 req/min, plan gratuito)
- Tipo IP: mobile, residential, hosting, business
- Pais, region, rDNS
- **CGNAT detection**: IP publica en rango `100.64.0.0/10` (RFC 6598)
- RTT promedio a Cloudflare (1.1.1.1) y Google (8.8.8.8)
- Jitter (max - min de muestras)

---

## Motor de clasificacion (scoring)

| Senal | Peso | Direccion |
|-------|------|-----------|
| Subnet iPhone `172.20.10.0/28` | +60 | CELULAR |
| Subnet Android `192.168.43.x` | +45 | CELULAR |
| Subnet pequena (/28+) generica | +25 | CELULAR (debil) |
| Subnet `192.168.x` / `10.x` (/24) | -30 | ISP FIJO |
| Flag `mobile` de API | +40 | CELULAR |
| CGNAT detectado | +25 | CELULAR |
| RTT > 80ms | +10 | CELULAR (debil) |
| RTT < 30ms estable | -15 | ISP FIJO |

**Umbrales:**
- `score >= 50` -> CELULAR
- `score <= -20` -> ISP FIJO
- en medio -> INDETERMINADO

**Confianza:** `|score| / max_score * 100`

---

## Estructura del proyecto

```
WiRa/
├── WiRa/
│   ├── WiRa.ino             # Main + maquina estados FreeRTOS
│   ├── wifi_scan.h/.cpp     # Modulo escaneo + medicion RSSI
│   ├── wifi_connect.h/.cpp  # Conexion robusta (BSSID dirigido + reintento)
│   ├── net_profile.h/.cpp   # Modulos A/B/C + clasificador scoring
│   ├── ui_render.h/.cpp     # Renderizado sprite anti-flicker
│   ├── keyboard_input.h/.cpp # Teclado + raw mode + key repeat
│   ├── storage.h/.cpp       # SD logging JSON+CSV
│   └── WiRa.bin             # Binario compilado
└── README.md
```

---

## Persistencia (microSD)

### Archivos generados

| Archivo | Formato | Contenido |
|---------|---------|-----------|
| `/wira/log.csv` | CSV | Resumen una fila por perfil (timestamp, SSID, veredicto, IP publica, ASN, ping, etc.) |
| `/wira/p_NNNNN.json` | JSON | Perfil completo individual con todos los modulos |

### CSV columnas
```
ts_ms,ssid,bssid,verdict,confidence,public_ip,asn,is_mobile,cgnat,ping_ms,subnet,hotspot
```

### JSON estructura
```json
{
  "ts_ms": 12345678,
  "verdict_label": "CELULAR",
  "confidence": 85,
  "local": { "ssid": "...", "ip": "...", "prefix": 28, "hotspot_hint": "iPhone hotspot", ... },
  "internet": { "api_ok": true, "public_ip": "...", "asn": "AS3462", "is_mobile": true, "cgnat": true, ... }
}
```

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

| Opcion | Valor |
|--------|-------|
| Placa | **ESP32S3 Dev Module** (NO ESP32 Dev Module) |
| PSRAM | **Disabled** (FN8 no tiene PSRAM) |
| Flash Size | 8MB |
| Partition Scheme | 8M with spiffs (3MB APP/1.5MB SPIFFS) |
| USB CDC On Boot | Enabled |
| Upload Speed | 115200 (subir despues a 921600 si va bien) |
| CPU Frequency | 240MHz |

### Cargar binario precompilado

1. Descarga `WiRa/WiRa.bin`
2. Conecta Cardputer ADV via USB-C
3. Sube con Arduino IDE o esptool:

```bash
esptool.py --chip esp32s3 --port COM3 write_flash 0x0 WiRa.bin
```

### Compilar desde fuente

```bash
git clone https://github.com/Nicolas-Losada/WiRa_CardputerADV.git
cd WiRa_CardputerADV/WiRa
# Abrir WiRa.ino en Arduino IDE, configurar placa, Ctrl+U
```

---

## Uso

### Modo Radar (medidor)
1. Enciende el Cardputer ADV
2. Espera escaneo inicial automatico
3. Navega con `;` y `.`
4. Pulsa `Enter` -> medidor RSSI tiempo real
5. `B` para volver, `R` para re-escanear

### Modo Profiler
1. Selecciona AP propio en la lista
2. Pulsa `C` -> entrada contrasena
3. Escribe contrasena (chars literales en modo raw)
4. `Enter` -> conecta y perfila
5. Espera 3 pasos: Red local, Internet, Clasificacion
6. Ve resultado con veredicto
7. Pulsa `S` para guardar a SD

---

## Limitaciones

- Solo WiFi **2.4 GHz** (limitacion hardware ESP32-S3)
- Hotspots 5 GHz no detectables
- Solo **diagnostico pasivo** de la propia conexion
- NO captura trafico ajeno, NO deauth, NO modo monitor
- Sin PSRAM: buffers acotados, max 50 APs en scan
- API ip-api.com gratis: 45 req/min, HTTP (no TLS)

---

## Uso de recursos

| Recurso | Valor |
|---------|-------|
| Flash | ~1140 KB (35%) |
| RAM global | ~53 KB (16%) |
| Sprite anti-flicker | ~65 KB (240x135 @ 16bpp) |
| APs max en scan | 50 |
| Histograma RSSI | 30 muestras |

---

## Solucion de problemas

### El AP de mi celular no aparece
- Verifica que el hotspot esta en **2.4 GHz** (no 5 GHz)
- Algunos celulares (Xiaomi, Samsung) usan canal 12/13 — el firmware ya habilita esos canales
- Activa "Banda preferida: 2.4 GHz" en ajustes hotspot

### No puedo conectar a mi router
- Verifica contrasena (modo raw - `Fn` para revelar)
- Algunos routers WPA3 puros pueden tardar mas que el timeout
- Reintento automatico despues del primer fallo (sin BSSID dirigido)

### Pantalla con flicker
- Si el sprite no se creo (revisa Serial Monitor), fallback a 8bpp
- 16bpp = 64.8 KB en SRAM, debe caber siempre

### Subir falla "Wrong chip"
- Asegurate de tener **ESP32S3 Dev Module** seleccionado, no ESP32 generico

---

## Creditos

- Hardware: [M5Stack Cardputer ADV](https://shop.m5stack.com/products/m5stack-cardputer-adv-version-esp32-s3)
- Librerias: [M5Cardputer](https://github.com/m5stack/M5Cardputer), [M5Unified](https://github.com/m5stack/M5Unified), [M5GFX](https://github.com/m5stack/M5GFX), [ArduinoJson](https://github.com/bblanchon/ArduinoJson), [ESP32Ping](https://github.com/marian-craciunescu/ESP32Ping)
- APIs externas: [ip-api.com](http://ip-api.com), [ipify.org](https://api.ipify.org)
- Pinout SD: [docs.m5stack.com/en/core/Cardputer-Adv](https://docs.m5stack.com/en/core/Cardputer-Adv)
- Desarrollado con asistencia de Claude (Anthropic)

---

## Licencia

MIT - libre para uso educativo y de diagnostico personal.
