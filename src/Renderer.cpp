#include "Renderer.h"
#include <iostream>

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

	if (window == NULL) throw std::runtime_error("Window creation failed!");

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

	vk::ApplicationInfo appInfo = {
	.apiVersion = vk::ApiVersion14
	};

	if(ENABLE_VALIDATION_LAYER) {
		const std::vector<const char*> validationLayers = helper.verifyValidationLayers(*this);

		vk::InstanceCreateInfo instanceCreateInfo = {
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
			.ppEnabledLayerNames = validationLayers.data(),
			.enabledExtensionCount = requiredGlfwExtensionsCount,
			.ppEnabledExtensionNames = requiredGlfwExtensions,
		};

		Vinstance = vk::raii::Instance(Vcontext, instanceCreateInfo);
	} else {
		vk::InstanceCreateInfo instanceCreateInfo = {
			.pApplicationInfo = &appInfo,
			.enabledExtensionCount = requiredGlfwExtensionsCount,
			.ppEnabledExtensionNames = requiredGlfwExtensions
		};

		Vinstance = vk::raii::Instance(Vcontext, instanceCreateInfo);
	}

	std::cout << "Vulkan instance created\n";
}

void Renderer::createVSurface() {
	VkSurfaceKHR tempSurface;

	glfwCreateWindowSurface(*Vinstance, window, nullptr, &tempSurface);

	Vsurface = vk::raii::SurfaceKHR(Vinstance, tempSurface);

	std::cout << "Vulkan surface created\n";
}

void Renderer::selectVPhysicalDevice() {
	std::vector<uint32_t> physicalDeviceAbilitiesInFour = helper.grokPhysicalDevices(Vinstance.enumeratePhysicalDevices(), *this);

	bool foundOne = false;
	for(uint32_t i = 0; i < physicalDeviceAbilitiesInFour.size(); i += 4) {
		if( physicalDeviceAbilitiesInFour[i] &&
		    physicalDeviceAbilitiesInFour[i + 1] &&
		    physicalDeviceAbilitiesInFour[i + 2] &&
			physicalDeviceAbilitiesInFour[i + 3]) {
			VphysicalDevice = Vinstance.enumeratePhysicalDevices()[i / 4];
			std::cout << "Selected physical device (" << VphysicalDevice.getProperties().deviceName << ")\n";
			foundOne = true;
		}
	}

	if(!foundOne) {
		throw std::runtime_error("No appropriate GPU found on your computer");
	}
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
