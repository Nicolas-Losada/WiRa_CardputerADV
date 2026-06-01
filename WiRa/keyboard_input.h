#ifndef KEYBOARD_INPUT_H
#define KEYBOARD_INPUT_H

#include <cstdint>

enum KeyCode {
  KEY_NONE = 0,
  KEY_UP = 1,
  KEY_DOWN = 2,
  KEY_LEFT = 3,
  KEY_RIGHT = 4,
  KEY_SELECT = 5,   // Enter (medidor RSSI)
  KEY_BACK = 6,     // Backspace / Esc
  KEY_RESCAN = 7,   // R - rescan/re-perfil
  KEY_CONNECT = 8,  // C - conectar a AP
  KEY_SAVE = 9,     // S - guardar a SD
  KEY_CHAR = 10,    // caracter ASCII (modo password)
  KEY_UNKNOWN = 255
};

struct KeyEvent {
  KeyCode key;
  char ch;            // valido si key==KEY_CHAR
  uint32_t press_time;
  KeyEvent() : key(KEY_NONE), ch(0), press_time(0) {}
};

void keyboard_init();
// raw=true: chars literales (no convertir , . ; / a flechas)
KeyEvent keyboard_process(bool raw = false);
const char* keyboard_get_key_name(KeyCode key);

#endif
