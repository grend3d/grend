#include <grend/modalSDLInput.hpp>
#include <grend/sdlContext.hpp>
#include <iostream>

using namespace grendx;

// giving a mode of -1 will bind for all modes
void modalSDLInput::bind(int mode, bindFunc func) {
	bindings[mode].push_back({func, maskNone});
}

void modalSDLInput::bind(int mode, bindFunc func, maskFunc mask) {
	bindings[mode].push_back({func, mask});
}

void modalSDLInput::bind(int mode, bindEntry entry) {
	bindings[mode].push_back(entry);
}

void modalSDLInput::setMode(int m) {
	mode = m;
	sanityCheck();
}

void modalSDLInput::pushMode(int m) {
	modestack.push_back(mode);
	mode = m;
}

void modalSDLInput::popMode(void) {
	if (!modestack.empty()) {
		mode = modestack.back();
		modestack.pop_back();
	}
}

bool modalSDLInput::sanityCheck(void) {
	if (bindings.find(mode) == bindings.end()) {
		std::cerr << "modalSDLInput::sanityCheck(): mode '"
			+ std::to_string(mode) + "' doesn't have any bindings!" << std::endl;
		return false;
	}

	return true;
}

bool modalSDLInput::maskNone(void) {
	return false;
}

int modalSDLInput::dispatch(const SDL_Event& ev) {
	// keep track of control/shift state outside of the bindings
	if (ev.type == SDL_KEYDOWN) {
		switch (ev.key.keysym.sym) {
			case SDLK_LCTRL:
			case SDLK_RCTRL:  flags |= bindFlags::Control; break;
			case SDLK_LSHIFT:
			case SDLK_RSHIFT: flags |= bindFlags::Shift;   break;
			default: break;
		}

	} else if (ev.type == SDL_KEYUP) {
		switch (ev.key.keysym.sym) {
			// TODO: this will give wrong results if the user has both
			//       left and right control/shift pressed, maybe don't do that
			case SDLK_LCTRL:
			case SDLK_RCTRL:  flags &= ~bindFlags::Control; break;
			case SDLK_LSHIFT:
			case SDLK_RSHIFT: flags &= ~bindFlags::Shift;   break;
			default: break;
		}
	}

	// binds for a mode of -1 run for every event
	for (int m : {MODAL_ALL_MODES, mode}) {
		if (bindings.find(m) == bindings.end())
			continue;

		for (auto& [bind, mask] : bindings[m]) {
			if (!mask()) {
				int retmode = bind(ev, flags);

				if (retmode != MODAL_NO_CHANGE && retmode != m) {
					// if this binding set a new mode, then stop processing
					// bindings and set the mode to the new one
					mode = retmode;
					sanityCheck();
					return mode;
				}
			}
		}
	}

	return mode;
}
