#include "Renderer.h"

void Renderer::run() {
	createWindow();
	initializeVulkan();
	mainLoop();
	clean();
}

void Renderer::createWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Heart And Gun", nullptr, nullptr);
}

void Renderer::initializeVulkan() {
	createVInstance();
	createVSurface();
	selectVPhysicalDevice();
	createVLogicalDevice();
	createVQueue();
	createVSwapchain();
	createVImageViews();
	createVGraphicsPipeline();
	createVCommandPool();
	createVCommandBuffers();
	createVSyncObjects();
}

void Renderer::clean() {
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Renderer::drawFrame() {

}

void Renderer::mainLoop() {

}

void Renderer::createVInstance() {
	const vk::InstanceCreateInfo instanceCreateInfo = {

	};
}

void Renderer::createVSurface() {

}

void Renderer::selectVPhysicalDevice() {

}

void Renderer::createVLogicalDevice() {

}

void Renderer::createVQueue() {

}

void Renderer::createVSwapchain() {

}

void Renderer::createVImageViews() {

}

void Renderer::createVGraphicsPipeline() {

}

void Renderer::createVCommandPool() {

}

void Renderer::createVCommandBuffers() {

}

void Renderer::createVSyncObjects() {

}
