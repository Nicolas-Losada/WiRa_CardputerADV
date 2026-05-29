/*
  =====================================================================
  MÓDULO: Keyboard Input (Implementación)
  =====================================================================
  Ver keyboard_input.h para documentación de funciones

  El M5Stack Cardputer utiliza M5.Keyboard para acceder al teclado.
  Este módulo mapea las teclas físicas a códigos de acción lógicos.

  Mapeo de teclas:
  - Cursor arriba (UP arrow key) → KEY_UP
  - Cursor abajo (DOWN arrow key) → KEY_DOWN
  - Enter / 'E' key → KEY_SELECT
  - Backspace / 'B' key → KEY_BACK
  - 'R' key → KEY_RESCAN

  =====================================================================
*/

#include "keyboard_input.h"
#include <M5Cardputer.h>
#include <M5Unified.h>
#include <Arduino.h>

// =====================================================================
// ESTADO INTERNO DEL MÓDULO
// =====================================================================
static struct {
  KeyCode last_key;
  uint32_t last_key_time;
  uint32_t debounce_delay;  // ms (50ms es típico)
} kb_state = {
  .last_key = KEY_NONE,
  .last_key_time = 0,
  .debounce_delay = 50
};

// =====================================================================
// Función: Inicializar teclado
// =====================================================================
void keyboard_init() {
  kb_state.last_key = KEY_NONE;
  kb_state.last_key_time = 0;
  kb_state.debounce_delay = 50;

  Serial.println("[Keyboard] Módulo inicializado");
}

// =====================================================================
// Función: Procesar entrada del teclado
// =====================================================================
KeyEvent keyboard_process() {
  KeyEvent event;
  event.key = KEY_NONE;
  event.press_time = millis();

  // Debouncing: ignorar pulsaciones muy cercanas en tiempo
  if (event.press_time - kb_state.last_key_time < kb_state.debounce_delay) {
    return event;  // Sin tecla detectada
  }

  // ====================================================================
  // LECTURA TECLADO CARDPUTER - API M5Cardputer 1.1+
  // ====================================================================
  // M5Cardputer.Keyboard.isChange() detecta cambio
  // M5Cardputer.Keyboard.isKeyPressed(char) detecta tecla

  if (M5Cardputer.Keyboard.isChange()) {
    // Verificar teclas de navegación y control
    if (M5Cardputer.Keyboard.isKeyPressed('U')) {
      event.key = KEY_UP;
    }
    else if (M5Cardputer.Keyboard.isKeyPressed('D')) {
      event.key = KEY_DOWN;
    }
    else if (M5Cardputer.Keyboard.isKeyPressed('E')) {
      event.key = KEY_SELECT;
    }
    else if (M5Cardputer.Keyboard.isKeyPressed('B')) {
      event.key = KEY_BACK;
    }
    else if (M5Cardputer.Keyboard.isKeyPressed('R')) {
      event.key = KEY_RESCAN;
    }

    // Registrar evento válido
    if (event.key != KEY_NONE) {
      kb_state.last_key = event.key;
      kb_state.last_key_time = event.press_time;
      Serial.printf("[KB] Tecla: %s\n", keyboard_get_key_name(event.key));
    }
  }

  return event;
}

// =====================================================================
// Función auxiliar: Obtener nombre de tecla (para debug)
// =====================================================================
const char* keyboard_get_key_name(KeyCode key) {
  switch (key) {
    case KEY_NONE: return "NONE";
    case KEY_UP: return "UP";
    case KEY_DOWN: return "DOWN";
    case KEY_LEFT: return "LEFT";
    case KEY_RIGHT: return "RIGHT";
    case KEY_SELECT: return "SELECT";
    case KEY_BACK: return "BACK";
    case KEY_RESCAN: return "RESCAN";
    case KEY_UNKNOWN: return "UNKNOWN";
    default: return "???";
  }
}

// =====================================================================
// NOTAS SOBRE LECTURA DE TECLADO EN M5STACK CARDPUTER
// =====================================================================
//
// El Cardputer ADV tiene 56 teclas en matriz 4x14. En M5Unified,
// el teclado se lee mediante:
//
// 1. M5.update() - Actualiza estado global (IMPORTANTE llamar en loop)
// 2. M5.Keyboard.isPressed() - Comprueba si hay tecla presionada
// 3. M5.Keyboard.getKey() - Obtiene el código de la tecla
//
// CÓDIGOS ESPECIALES (referencias):
// - Enter / ASCII 13
// - Backspace / ASCII 8
// - Flechas: códigos extendidos (varían según driver)
//
// Si los códigos no funcionan, alternativa:
// - Usar M5.BtnA, M5.BtnB, M5.BtnC (botones capacitivos si existen)
// - Usar escaneo directo de matriz GPIO (más complejo)
//
// RECOMENDACIÓN DE DEBUGGING:
// Añadir en el loop principal: Serial.println(M5.Keyboard.getKey());
// para ver qué códigos retorna tu Cardputer específico.
//
// =====================================================================
