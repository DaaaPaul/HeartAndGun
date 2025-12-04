#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <cstring>
#include <map>

class Renderer {
public:
	void run();

	Renderer(int const& width, int const& height, bool const& enable) : 
		WINDOW_WIDTH(width), WINDOW_HEIGHT(height), ENABLE_VALIDATION_LAYER(enable) {}

private:
	class Helper {
	public:
		const std::vector<const char*> verifyValidationLayers(Renderer const& renderer) const;
		const char** verifyGlfwExtensionsPresent(Renderer const& renderer) const;
		std::vector<uint32_t> grokPhysicalDevices(std::vector<vk::raii::PhysicalDevice> const& physicalDevices, Renderer const& renderer) const;
		uint32_t findGraphicsQueueFamilyIndex(std::vector<vk::QueueFamilyProperties> const& queueFamilyProperties, Renderer const& renderer) const;
		vk::SurfaceFormatKHR getSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& surfaceFormats, Renderer const& renderer) const;
		vk::PresentModeKHR getPresentMode(std::vector<vk::PresentModeKHR> const& presentModes, Renderer const& renderer) const;
		vk::Extent2D getSurfaceExtent(vk::SurfaceCapabilitiesKHR const& capabilities, Renderer const& renderer) const;
		uint32_t getSwapchainImageCount(vk::SurfaceCapabilitiesKHR const& capabilities, Renderer const& renderer) const;
	};

	const Helper helper{};

	const int WINDOW_WIDTH = ~0;
	const int WINDOW_HEIGHT = ~0;
	const bool ENABLE_VALIDATION_LAYER = true;

	const std::vector<const char*> VALIDATION_LAYERS = {
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> REQUIRED_DEVICE_EXTENSIONS = {
		vk::KHRSwapchainExtensionName,
		vk::KHRSpirv14ExtensionName,
		vk::KHRSynchronization2ExtensionName,
		vk::KHRCreateRenderpass2ExtensionName
	};

	uint32_t graphicsFamilyIndex = ~0;

	vk::SurfaceFormatKHR surfaceFormat{};
	vk::Extent2D extent{};
	vk::PresentModeKHR presentMode{};
	uint32_t swapchainImageCount = ~0;

	GLFWwindow* window = nullptr;
	vk::raii::Context Vcontext{};
	vk::raii::Instance Vinstance = nullptr;
	vk::raii::SurfaceKHR Vsurface = nullptr;
	vk::raii::PhysicalDevice VphysicalDevice = nullptr;
	vk::raii::Device VlogicalDevice = nullptr;
	vk::raii::Queue VgraphicsQueue = nullptr;
	vk::raii::SwapchainKHR Vswapchain = nullptr;
	std::vector<vk::raii::ImageView> VimageViews{};
	vk::raii::Pipeline VgraphicsPipeline = nullptr;
	vk::raii::CommandPool VcommandPool = nullptr;
	std::vector<vk::raii::CommandBuffer> VcommandBuffers{};
	std::vector<vk::raii::Semaphore> VrenderReady{};
	std::vector<vk::raii::Semaphore> VrenderDone{};
	std::vector<vk::raii::Fence> VcommandBufferCompleted{};

	void createWindow();
	void initializeVulkan();
	void clean();
	void mainLoop();

	void createVInstance();
	void createVSurface();
	void selectVPhysicalDevice();
	void createVLogicalDevice();
	void createVQueue();
	void createVSwapchain();
	void createVImageViews();
	void createVGraphicsPipeline();
	void createVCommandPool();
	void createVCommandBuffers();
	void createVSyncObjects();

	void drawFrame();
};