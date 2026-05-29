/*
  =====================================================================
  MÓDULO: Keyboard Input
  =====================================================================
  Función: Captura y procesamiento de entrada del teclado

  El M5Stack Cardputer tiene 56 teclas organizadas en matriz 4x14.
  Las teclas más importantes para este firmware:
  - Cursor arriba/abajo (UP, DOWN)
  - Enter/Select (E)
  - Backspace/Back (B)
  - Tecla especial para rescanear (R)

  Proporciona:
  - keyboard_init(): Inicialización del teclado
  - keyboard_process(): Procesa pulsaciones y retorna acción

  =====================================================================
*/

#ifndef KEYBOARD_INPUT_H
#define KEYBOARD_INPUT_H

#include <cstdint>

// =====================================================================
// CÓDIGOS DE TECLAS MAPEADAS
// =====================================================================
enum KeyCode {
  KEY_NONE = 0,
  KEY_UP = 1,        // Cursor arriba
  KEY_DOWN = 2,      // Cursor abajo
  KEY_LEFT = 3,      // Cursor izquierda
  KEY_RIGHT = 4,     // Cursor derecha
  KEY_SELECT = 5,    // Enter / Select
  KEY_BACK = 6,      // Backspace / Atrás
  KEY_RESCAN = 7,    // R (Rescanear)
  KEY_UNKNOWN = 255
};

// =====================================================================
// ESTRUCTURA: Evento de teclado
// =====================================================================
struct KeyEvent {
  KeyCode key;
  uint32_t press_time;  // Timestamp de la pulsación (ms)

  KeyEvent() : key(KEY_NONE), press_time(0) {}
};

// =====================================================================
// FUNCIONES PÚBLICAS
// =====================================================================

/**
 * Inicializa el módulo de teclado.
 * Configura los pines y el estado interno.
 */
void keyboard_init();

/**
 * Procesa la entrada del teclado.
 * Debe llamarse cada iteración del loop principal.
 *
 * Retorna:
 * - KeyEvent con key != KEY_NONE si hay una pulsación detectada
 * - KeyEvent con key == KEY_NONE si no hay pulsación
 *
 * Implementa debouncing simple para evitar múltiples lecturas
 * de una sola pulsación.
 */
KeyEvent keyboard_process();

/**
 * Obtiene el nombre de una tecla (para debug).
 *
 * @param key Código de tecla
 * @return Cadena descriptiva (ej: "UP", "SELECT")
 */
const char* keyboard_get_key_name(KeyCode key);

#endif  // KEYBOARD_INPUT_H
