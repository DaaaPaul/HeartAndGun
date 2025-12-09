#include "Logger.h"

void Logger::logInformation(std::string const& msg) const {
	fmt::print(fg(fmt::color::green) | fmt::emphasis::faint, "{}\n", msg);
}

void Logger::logSpecial(std::string const& msg) const {
	fmt::print(fg(fmt::color::antique_white) | fmt::emphasis::bold, "{}\n", msg);
}

void Logger::logError(std::string const& msg) const {
	fmt::print(fg(fmt::color::red) | fmt::emphasis::italic, "[ERROR] {}\n", msg);
}