#pragma once

#include "RendererHelper.h"
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

class Renderer {
	friend class RendererHelper;

public:
	void run();

	Renderer(int const& width, int const& height, bool const& enable) : 
		WINDOW_WIDTH(width), WINDOW_HEIGHT(height), ENABLE_VALIDATION_LAYER(enable) {}
private:
	const RendererHelper helper{};

	const int WINDOW_WIDTH = ~0;
	const int WINDOW_HEIGHT = ~0;
	const bool ENABLE_VALIDATION_LAYER = true;

	const std::vector<const char*> VALIDATION_LAYERS = {
		"VK_LAYER_KHRONOS_validation"
	};

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