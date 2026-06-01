// =====================================================================
// keyboard_input.cpp - Lectura teclado M5Cardputer ADV
// Soporta: navegacion, key repeat, modo raw para password
// =====================================================================
#include "keyboard_input.h"
#include <M5Cardputer.h>
#include <Arduino.h>

static const uint32_t DEBOUNCE_MS = 50;
static const uint32_t REPEAT_DELAY = 500;     // antes de empezar repeticion
static const uint32_t REPEAT_INTERVAL = 120;  // entre repeticiones

static uint32_t s_last_emit_ms = 0;
static KeyCode s_last_emitted = KEY_NONE;
static uint32_t s_hold_start_ms = 0;

void keyboard_init() {
  s_last_emit_ms = 0;
  s_last_emitted = KEY_NONE;
  s_hold_start_ms = 0;
  Serial.println("[KB] Init");
}

KeyEvent keyboard_process(bool raw) {
  KeyEvent ev;
  ev.press_time = millis();
  uint32_t now = ev.press_time;

  if (!M5Cardputer.Keyboard.isChange() && !M5Cardputer.Keyboard.isPressed()) {
    // Nadie pulsa -> reset estado repeticion
    s_last_emitted = KEY_NONE;
    s_hold_start_ms = 0;
    return ev;
  }

  if (!M5Cardputer.Keyboard.isPressed()) return ev;

  Keyboard_Class::KeysState st = M5Cardputer.Keyboard.keysState();

  // Teclas especiales con flags propios
  if (st.enter) { ev.key = KEY_SELECT; }
  else if (st.del) { ev.key = KEY_BACK; }
  else {
    // Iterar chars pulsados
    for (auto c : st.word) {
      if (raw) {
        // Modo password: todo es char literal salvo ` (escape)
        if (c == '`') { ev.key = KEY_BACK; break; }
        ev.key = KEY_CHAR;
        ev.ch = c;
        break;
      }
      // Modo navegacion
      switch (c) {
        case ',':  ev.key = KEY_LEFT; break;
        case '/':  ev.key = KEY_RIGHT; break;
        case ';':  ev.key = KEY_UP; break;
        case '.':  ev.key = KEY_DOWN; break;
        case '`':  ev.key = KEY_BACK; break;
        case 'u': case 'U': ev.key = KEY_UP; break;
        case 'd': case 'D': ev.key = KEY_DOWN; break;
        case 'e': case 'E': ev.key = KEY_SELECT; break;
        case 'b': case 'B': ev.key = KEY_BACK; break;
        case 'r': case 'R': ev.key = KEY_RESCAN; break;
        case 'c': case 'C': ev.key = KEY_CONNECT; break;
        case 's': case 'S': ev.key = KEY_SAVE; break;
        default: break;
      }
      if (ev.key != KEY_NONE) break;
    }
  }

  if (ev.key == KEY_NONE) return ev;

  // === Key repeat logic ===
  // Solo aplicar a teclas de navegacion (UP/DOWN/LEFT/RIGHT)
  bool repeatable = (ev.key == KEY_UP || ev.key == KEY_DOWN ||
                     ev.key == KEY_LEFT || ev.key == KEY_RIGHT);

  if (ev.key == s_last_emitted) {
    // Misma tecla siendo mantenida
    if (s_hold_start_ms == 0) s_hold_start_ms = now;

    if (repeatable) {
      uint32_t held_for = now - s_hold_start_ms;
      if (held_for < REPEAT_DELAY) {
        ev.key = KEY_NONE;  // todavia no repetir
        return ev;
      }
      // En modo repeat: emitir cada REPEAT_INTERVAL
      if (now - s_last_emit_ms < REPEAT_INTERVAL) {
        ev.key = KEY_NONE;
        return ev;
      }
    } else {
      // No repetir teclas de accion - solo 1 evento por hold
      ev.key = KEY_NONE;
      return ev;
    }
  } else {
    // Tecla nueva - debounce simple
    if (now - s_last_emit_ms < DEBOUNCE_MS) {
      ev.key = KEY_NONE;
      return ev;
    }
    s_hold_start_ms = now;
  }

  s_last_emitted = ev.key;
  s_last_emit_ms = now;

  if (ev.key != KEY_NONE && ev.key != KEY_CHAR) {
    Serial.printf("[KB] %s\n", keyboard_get_key_name(ev.key));
  }
  return ev;
}

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
    case KEY_CONNECT: return "CONNECT";
    case KEY_SAVE: return "SAVE";
    case KEY_CHAR: return "CHAR";
    default: return "???";
  }
}
