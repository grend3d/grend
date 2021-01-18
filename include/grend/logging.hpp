#pragma once
#include <stdio.h>

namespace grendx {

// convention: user might see anything from 'info' and lower,
// 'verbose' and 'spam' for dev stuff
enum logLevel {
	LOG_NONE,
	LOG_ERROR,
	LOG_WARNING,
	LOG_INFO,
	LOG_VERBOSE,
	LOG_SPAM,
};

static inline const char *logLevelStrings[] = {
	"None (...?)",
	"! Error",
	"Warning",
	"   Info",
	"    dbg",
	"   spam"
};

#ifndef GREND_LOG_LEVEL
#define GREND_LOG_LEVEL LOG_VERBOSE
#endif

// TODO: __android_log_print
#define GR_LOG(LEVEL, ...) \
	if (LEVEL <= GREND_LOG_LEVEL) { \
		fprintf(stderr, " %s > ", logLevelStrings[LEVEL]); \
		fprintf(stderr, __VA_ARGS__); \
		fprintf(stderr, "\n"); \
	}

// namespace grendx
}
