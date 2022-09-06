#pragma once

namespace grendx {

template <typename T>
const char *getTypeName() {
	return typeid(T).name();
}

template <typename T>
const char *getTypeName(T& thing) {
	return typeid(T).name();
}

template <typename T, typename... Us>
const char *getFirstTypeName() {
	return getTypeName<T>();
}

template <typename... T>
std::array<const char *, sizeof...(T)> getTypeNames() {
	return { getTypeName<T>()... };
}

// namespace grendx
}
