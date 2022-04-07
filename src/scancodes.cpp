#include <grend/scancodes.hpp>

namespace grendx {

static uint8_t keyboardLastPressed[SDL_NUM_SCANCODES];
static uint8_t keyboardCurPressed[SDL_NUM_SCANCODES];
static float   keyboardHoldTimes[SDL_NUM_SCANCODES];
static float   keyboardReleaseTimes[SDL_NUM_SCANCODES];

// bit overkill, but keep track of each bit returned from
// SDL_GetMouseState() just to account for any future expansion
static uint32_t mouseCurPressed;
static uint32_t mouseLastPressed;
static float    mouseHoldTimes[32];
static float    mouseReleaseTimes[32];

// TODO: might be a good idea to not rely on GetKeyboardState and instead
//       build our own state map from SDL_Events
//       also, maybe shouldn't have a global context, but these shouldn't
//       be hard to change if the need arises
void updateKeyStates(float delta) {
	const uint8_t *keystates = SDL_GetKeyboardState(NULL);

	for (unsigned n = 0; n < SDL_NUM_SCANCODES; n++) {
		bool last = keyboardLastPressed[n] = keyboardCurPressed[n];
		bool cur  = keyboardCurPressed[n]  = keystates[n];

		keyboardHoldTimes[n] = (last && cur)
			? keyboardHoldTimes[n] + delta
			: 0;

		keyboardReleaseTimes[n] = (!last && !cur)
			? keyboardReleaseTimes[n] + delta
			: 0;
	}

	mouseLastPressed = mouseCurPressed;
	// TODO: could do something with mouse coordinates
	mouseCurPressed  = SDL_GetMouseState(nullptr, nullptr);

	for (unsigned n = 0; n < 31; n++) {
		bool last = mouseLastPressed & (1 << n);
		bool cur  = mouseCurPressed  & (1 << n);

		mouseHoldTimes[n+1] = (last && cur)
			? mouseHoldTimes[n+1] + delta
			: 0;

		mouseReleaseTimes[n+1] = (!last && !cur)
			? mouseReleaseTimes[n+1] + delta
			: 0;
	}

	// TODO: handle controllers
}

bool keyIsPressed(uint32_t keysym) {
	switch (getKeysymDev(keysym)) {
		case DEV_KEYBOARD:
			// TODO: does this need bounds checks?
			//       should never have untrusted input I think
			return keyboardCurPressed[getKeysymButton(keysym) % SDL_NUM_SCANCODES];

		case DEV_MOUSE:
			return mouseCurPressed & SDL_BUTTON(getKeysymButton(keysym));

		case DEV_CONTROLLER:
			// TODO: controllers
		
		default:
			return false;
	}
}

bool keyLastPressed(uint32_t keysym) {
	switch (getKeysymDev(keysym)) {
		case DEV_KEYBOARD:
			// TODO: does this need bounds checks?
			//       should never have untrusted input I think
			return keyboardLastPressed[getKeysymButton(keysym) % SDL_NUM_SCANCODES];

		case DEV_MOUSE:
			return mouseLastPressed & SDL_BUTTON(getKeysymButton(keysym));

		case DEV_CONTROLLER:
			// TODO: controllers

		default:
			return false;
	}
}

bool keyJustPressed(uint32_t keysym) {
	return !keyLastPressed(keysym) && keyIsPressed(keysym);
}

bool keyHeld(unsigned keysym) {
	return keyLastPressed(keysym) && keyIsPressed(keysym);
}

bool keyReleased(unsigned keysym) {
	return keyLastPressed(keysym) && !keyIsPressed(keysym);
}

float keyHoldTime(unsigned keysym) {
	switch (getKeysymDev(keysym)) {
		case DEV_KEYBOARD:
			return keyboardHoldTimes[getKeysymButton(keysym) % SDL_NUM_SCANCODES];

		case DEV_MOUSE:
			return mouseHoldTimes[getKeysymButton(keysym) % 32];

		case DEV_CONTROLLER:
			// TODO: controllers

		default:
			break;
	}
}

float keyReleaseTime(uint32_t keysym) {
	switch (getKeysymDev(keysym)) {
		case DEV_KEYBOARD:
			return keyboardReleaseTimes[getKeysymButton(keysym) % SDL_NUM_SCANCODES];

		case DEV_MOUSE:
			return mouseReleaseTimes[getKeysymButton(keysym) % 32];

		case DEV_CONTROLLER:
			// TODO: controllers

		default:
			break;
	}
}

/*
bool toggleGamepad(SDL_GameControllerButton n) {
	if (!Controller()) return false;

	bool pressed = SDL_GameControllerGetButton(Controller(), n);

	if (pressed && !bar[n]) {
		bar[n] = 1;
		return true;
	}

	else if (!pressed && bar[n]) {
		bar[n] = 0;
		return false;
	}

	return false;
}

bool edgeGamepad(SDL_GameControllerButton n) {
	if (!Controller()) return false;
	bool pressed = SDL_GameControllerGetButton(Controller(), n);

	return pressed && edgebar[n];
}
*/

// namespace grendx
}
