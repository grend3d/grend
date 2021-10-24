#pragma once
#include <functional>
#include <grend/sdlContext.hpp>
#include <map>
#include <vector>
#include <utility> // std::pair

namespace grendx {

enum bindFlags {
	None    = 0,
	Control = (1 << 0),
	Shift   = (1 << 1),
};

typedef std::function<int(SDL_Event& ev, unsigned flags)> bindFunc;
typedef std::function<bool(void)> maskFunc;
typedef std::pair<bindFunc, maskFunc> bindEntry;

#define MODAL_ALL_MODES (-1)
#define MODAL_NO_CHANGE (-1)

class modalSDLInput {
	public:
		modalSDLInput() {};

		// giving a mode of -1 will bind for all modes
		void bind(int mode, bindFunc func /* default mask is no mask */);
		void bind(int mode, bindFunc func, maskFunc mask);
		void bind(int mode, bindEntry entry);
		void setMode(int m);
		void pushMode(int m);
		void popMode(void);

		bool sanityCheck(void);
		int dispatch(SDL_Event& ev);
		static bool maskNone(void);

		std::map<int, std::vector<bindEntry>> bindings;
		std::vector<int> modestack;
		unsigned flags = bindFlags::None;
		int mode = -1;
};

// namespace grendx
}
