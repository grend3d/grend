#pragma once
// unified input polling, allows checking the state of buttons on
// controllers, mice and keyboards with the same set of functions

#include <grend/sdlContext.hpp>
#include <stdint.h>

namespace grendx {

// button identifier format:
// 31..29: device type
// 28..25: device ID (16 IDs)
// 24..21: keyboard modifier flags, must match
//         low bits to high:
//         shift, alt/meta, control
//  9..0 : button ID

enum device_type {
	DEV_NONE       = 0,
	DEV_KEYBOARD   = 1,
	DEV_MOUSE      = 2,
	DEV_CONTROLLER = 3,
	// TODO: dev_fingers for touchscreen devices, maybe
};

enum key_modifiers {
	MOD_SHIFT   = (1 << 0),
	MOD_META    = (1 << 1),
	MOD_CONTROL = (1 << 2),
};

// TODO: own wrappers around SDL scancodes,
//       can map these to enum values for brevity
static inline constexpr
uint32_t keyButton(unsigned scancode) {
	// TODO: handle more than one keyboard (?)
	return (DEV_KEYBOARD << 29) | scancode;
}

static inline constexpr
uint32_t mouseButton(unsigned button) {
	return (DEV_MOUSE << 29) | button;
}

static inline constexpr
uint32_t controllerButton(unsigned button, unsigned id=0) {
	return (DEV_CONTROLLER << 29) | (id << 25) | button;
}

static inline constexpr
uint32_t getKeysymDev(uint32_t sym) {
	return sym >> 29;
}

static inline constexpr
uint32_t getKeysymButton(uint32_t sym) {
	return sym & 0x3ff;
}

// distinction between first press and whether it's currently pressed
// keyIsPressed() is level signal, keyJustPressed() is edge signal
bool  keyIsPressed(uint32_t keysym);
bool  keyJustPressed(uint32_t keysym);
bool  keyLastPressed(uint32_t keysym);
// keyHeld() returns true if the key is currently pressed and
// was pressed at the time of the last update
bool  keyHeld(uint32_t keysym);
bool  keyReleased(uint32_t keysym);
float keyHoldTime(uint32_t keysym);
float keyReleaseTime(uint32_t keysym);
void  updateKeyStates(float delta);

bool toggleGamepad(SDL_GameControllerButton n);
bool edgeGamepad(SDL_GameControllerButton n);

// namespace grendx
}
