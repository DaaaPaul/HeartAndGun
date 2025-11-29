#include <iostream>
#include <stdexcept>
#include "Renderer.h"

int main() {
	try {
		Renderer renderer(800, 600, true);
		renderer.run();
	} catch(std::exception const& e) {
		std::cout << e.what() << '\n';
	}
}