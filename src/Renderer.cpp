#include "Renderer.h"
#include <iostream>

const std::vector<const char*> Renderer::Helper::verifyValidationLayers(Renderer const& renderer) const {
	std::vector<vk::LayerProperties> layerProperties = renderer.Vcontext.enumerateInstanceLayerProperties();

	bool layerFound = false;
	for (uint32_t i = 0; i < renderer.VALIDATION_LAYERS.size(); ++i) {
		layerFound = false;

		for (vk::LayerProperties const& property : layerProperties) {
			if (strcmp(property.layerName, renderer.VALIDATION_LAYERS[i]) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) throw std::runtime_error("Required validation layer not supported:" + std::string(renderer.VALIDATION_LAYERS[i]));
		else std::cout << "Required validation layer supported:" << renderer.VALIDATION_LAYERS[i] << '\n';
	}

	return renderer.VALIDATION_LAYERS;
}

const char** Renderer::Helper::verifyGlfwExtensionsPresent(Renderer const& renderer) const {
	uint32_t requiredExtensionsCount = ~0;
	const char** requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionsCount);

	std::vector<vk::ExtensionProperties> extensionProperties = renderer.Vcontext.enumerateInstanceExtensionProperties();

	bool extensionFound = false;
	for(uint32_t i = 0; i < requiredExtensionsCount; ++i) {
		extensionFound = false;

		for(vk::ExtensionProperties const& property : extensionProperties) {
			if (strcmp(property.extensionName, requiredExtensions[i]) == 0) {
				extensionFound = true;
				break;
			}
		}

		if (!extensionFound)  throw std::runtime_error("Required extension by glfw not supported:" + std::string(requiredExtensions[i]));
		else std::cout << "Required extension by glfw supported:" << requiredExtensions[i] << '\n';
	}

	return requiredExtensions;
}

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

	std::cout << "GLFW window created\n";
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

void Renderer::drawFrame() {

}

void Renderer::mainLoop() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		drawFrame();
	}

	clean();
}

void Renderer::clean() {
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Renderer::createVInstance() {
	const char** requiredGlfwExtensions = helper.verifyGlfwExtensionsPresent(*this);
	uint32_t requiredGlfwExtensionsCount = ~0;
	glfwGetRequiredInstanceExtensions(&requiredGlfwExtensionsCount);

	if(ENABLE_VALIDATION_LAYER) {
		const std::vector<const char*> validationLayers = helper.verifyValidationLayers(*this);

		vk::InstanceCreateInfo instanceCreateInfo = {
			.enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
			.ppEnabledLayerNames = validationLayers.data(),
			.enabledExtensionCount = requiredGlfwExtensionsCount,
			.ppEnabledExtensionNames = requiredGlfwExtensions
		};

		Vinstance = vk::raii::Instance(Vcontext, instanceCreateInfo);
	} else {
		vk::InstanceCreateInfo instanceCreateInfo = {
			.enabledExtensionCount = requiredGlfwExtensionsCount,
			.ppEnabledExtensionNames = requiredGlfwExtensions
		};

		Vinstance = vk::raii::Instance(Vcontext, instanceCreateInfo);
	}

	std::cout << "Vulkan instance created\n";
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
