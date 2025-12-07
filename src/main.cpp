#include <iostream>
#include <stdexcept>
#include "Renderer.h"

int main() {
	try {
		Renderer renderer(800, 600, true, 2);
		renderer.run();
	} catch(std::exception const& e) {
		Logger().logError(e.what());
	}
}