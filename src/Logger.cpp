#include "Logger.h"

void Logger::logInformation(std::string const& msg) const {
	fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "{}\n", msg);
}

void Logger::logError(std::string const& msg) const {
	fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "[ERROR] {}\n", msg);
}