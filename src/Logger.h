#pragma once

#define FMT_HEADER_ONLY
#include "fmt/core.h"
#include "fmt/color.h"
#include <string>

class Logger {
public:
	void logInformation(std::string const& msg) const;
	void logSpecial(std::string const& msg) const;
	void logError(std::string const& msg) const;
};

