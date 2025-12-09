#include "Renderer.h"
#include <iostream>
#include <limits>

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

	logger.logInformation("GLFW window created");
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

	logger.logInformation("-------------------------------------------------------------------------------------------------------\n"
						  "FINISHED VULKAN INITIALIZATION\n"
						  "-------------------------------------------------------------------------------------------------------");
}

void Renderer::drawFrame() {
	while (VlogicalDevice.waitForFences(*VcommandBufferCompleted[frameInFlight], vk::True, UINT64_MAX) == vk::Result::eTimeout);
	VlogicalDevice.resetFences(*VcommandBufferCompleted[frameInFlight]);

	std::pair<vk::Result, uint32_t> imagePair = Vswapchain.acquireNextImage(UINT64_MAX, *VrenderReady[semaphoreIndex], nullptr);

	VcommandBuffers[frameInFlight].reset();
	helper.recordCommandBuffer(imagePair.second, *this);

	vk::PipelineStageFlags waitMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
	vk::SubmitInfo submitCommandBuffer = {
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &*VrenderReady[semaphoreIndex],
		.pWaitDstStageMask = &waitMask,
		.commandBufferCount = 1,
		.pCommandBuffers = &*VcommandBuffers[frameInFlight],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &*VrenderDone[semaphoreIndex]
	};
	VgraphicsQueue.submit(submitCommandBuffer, *VcommandBufferCompleted[frameInFlight]);

	vk::PresentInfoKHR presentInfo = {
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &*VrenderDone[semaphoreIndex],
		.swapchainCount = 1,
		.pSwapchains = &*Vswapchain,
		.pImageIndices = &imagePair.second
	};
	VgraphicsQueue.presentKHR(presentInfo);

	frameInFlight = (frameInFlight + 1) % FRAMES_IN_FLIGHT;
	semaphoreIndex = (semaphoreIndex + 1) % Vswapchain.getImages().size();
}

void Renderer::mainLoop() {
	uint32_t nextSecond = 1;
	uint32_t framesInSecond = 0;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		drawFrame();

		if (glfwGetTime() <= nextSecond) {
			++framesInSecond;
		} else {
			std::cout << "FPS:" << framesInSecond << '\n';
			++nextSecond;
			framesInSecond = 0;
		}
	}
	VlogicalDevice.waitIdle();

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

	logger.logInformation("Vulkan instance created");
}

void Renderer::createVSurface() {
	VkSurfaceKHR tempSurface;

	glfwCreateWindowSurface(*Vinstance, window, nullptr, &tempSurface);

	Vsurface = vk::raii::SurfaceKHR(Vinstance, tempSurface);

	logger.logInformation("Vulkan surface created");
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
			logger.logInformation("Selected physical device (" + std::string(VphysicalDevice.getProperties().deviceName.data()) + ")");
			foundOne = true;
		}
	}

	if(!foundOne) {
		throw std::runtime_error("No appropriate GPU found on your computer");
	}
}

void Renderer::createVLogicalDevice() {
	vk::StructureChain<vk::PhysicalDeviceFeatures2,
		vk::PhysicalDeviceVulkan11Features,
		vk::PhysicalDeviceVulkan13Features,
		vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> enabledFeatures =
	{   {},
		{.shaderDrawParameters = true},
		{.synchronization2 = true, .dynamicRendering = true, },
		{.extendedDynamicState = true}	};

	std::vector<vk::QueueFamilyProperties> queueFamilyProperties = VphysicalDevice.getQueueFamilyProperties();
	graphicsFamilyIndex = helper.findGraphicsQueueFamilyIndex(queueFamilyProperties, *this);

	float arbitraryPriority = 0.5f;
	vk::DeviceQueueCreateInfo queueCreateInfo = {
		.queueFamilyIndex = graphicsFamilyIndex,
		.queueCount = 1,
		.pQueuePriorities = &arbitraryPriority
	};

	vk::DeviceCreateInfo logicalDeviceCreateInfo = {
		.pNext = &enabledFeatures.get<vk::PhysicalDeviceFeatures2>(),
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueCreateInfo,
		.enabledExtensionCount = static_cast<uint32_t>(REQUIRED_DEVICE_EXTENSIONS.size()),
		.ppEnabledExtensionNames = REQUIRED_DEVICE_EXTENSIONS.data()
	};

	VlogicalDevice = vk::raii::Device(VphysicalDevice, logicalDeviceCreateInfo);
	logger.logInformation("Created logical device");
}

void Renderer::createVQueue() {
	VgraphicsQueue = vk::raii::Queue(VlogicalDevice, graphicsFamilyIndex, 0);
}

void Renderer::createVSwapchain() {
	surfaceFormat = helper.getSurfaceFormat(VphysicalDevice.getSurfaceFormatsKHR(Vsurface), *this);
	presentMode = helper.getPresentMode(VphysicalDevice.getSurfacePresentModesKHR(Vsurface), *this);
	extent = helper.getSurfaceExtent(VphysicalDevice.getSurfaceCapabilitiesKHR(Vsurface), *this);
	swapchainImageCount = helper.getSwapchainImageCount(VphysicalDevice.getSurfaceCapabilitiesKHR(Vsurface), *this);

	vk::SwapchainCreateInfoKHR swapchainCreateInfo = {
		.surface = Vsurface,
		.minImageCount = swapchainImageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
		.imageSharingMode = vk::SharingMode::eExclusive,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &graphicsFamilyIndex,
		.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity,
		.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
		.presentMode = presentMode,
		.clipped = true
	};

	Vswapchain = vk::raii::SwapchainKHR(VlogicalDevice, swapchainCreateInfo);
	logger.logInformation("Created swapchain with " + std::to_string(Vswapchain.getImages().size()) + " images");
}

void Renderer::createVImageViews() {
	std::vector<vk::Image> images = Vswapchain.getImages();

	vk::ImageViewCreateInfo imageViewCreateInfo = {
		.image = images[0],
		.viewType = vk::ImageViewType::e2D,
		.format = surfaceFormat.format,
		.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
	};
	for(vk::Image const& image : images) {
		imageViewCreateInfo.image = image;

		VimageViews.push_back(vk::raii::ImageView(VlogicalDevice, imageViewCreateInfo));
	}
	logger.logInformation("Created image views");
}

void Renderer::createVGraphicsPipeline() {
	// configurable pipeline stages (vertex and fragment shader)
	std::vector<char> sprivBytes = helper.readSprivFileBytes("src/shaders/shaders.spv", *this);
	vk::ShaderModuleCreateInfo shaderModuleInfo = {
		.codeSize = sprivBytes.size() * sizeof(char),
		.pCode = reinterpret_cast<uint32_t*>(sprivBytes.data())
	};
	vk::raii::ShaderModule module(VlogicalDevice, shaderModuleInfo);

	vk::PipelineShaderStageCreateInfo vertexShaderInfo = {
		.stage = vk::ShaderStageFlagBits::eVertex,
		.module = module,
		.pName = "vertexMain",
	};
	vk::PipelineShaderStageCreateInfo fragmentShaderInfo = {
		.stage = vk::ShaderStageFlagBits::eFragment,
		.module = module,
		.pName = "fragmentMain",
	};
	vk::PipelineShaderStageCreateInfo shaderStagesInfo[] = { vertexShaderInfo , fragmentShaderInfo };

	// vertex input
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};

	// input assembly
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
		.topology = vk::PrimitiveTopology::eTriangleList,
		.primitiveRestartEnable = vk::False
	};

	// tessellation
	vk::PipelineTessellationStateCreateInfo tessellationInfo{};

	// viewport and scissor
	vk::Viewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = static_cast<float>(extent.width),
		.height = static_cast<float>(extent.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};
	vk::Rect2D scissor = {
		.offset = vk::Offset2D(0, 0),
		.extent = extent
	};
	vk::PipelineViewportStateCreateInfo viewportInfo = {
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	// rasterization
	vk::PipelineRasterizationStateCreateInfo rasterizationInfo = {
		.depthClampEnable = vk::False,
		.rasterizerDiscardEnable = vk::False,
		.polygonMode = vk::PolygonMode::eFill,
		.cullMode = vk::CullModeFlagBits::eBack,
		.frontFace = vk::FrontFace::eClockwise,
		.depthBiasEnable = vk::False,
		.depthBiasSlopeFactor = 1.0f,
		.lineWidth = 1.0f
	};

	// multisampling and blending
	vk::PipelineMultisampleStateCreateInfo multisamplingInfo = {
		.rasterizationSamples = vk::SampleCountFlagBits::e1,
		.sampleShadingEnable = vk::False
	};

	vk::PipelineColorBlendAttachmentState colorBlendAttachmentInfo = {
		.blendEnable = vk::False,
		.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
	};

	vk::PipelineColorBlendStateCreateInfo colorBlendingInfo = {
		.logicOpEnable = vk::False,
		.logicOp = vk::LogicOp::eCopy,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachmentInfo
	};

	// dynamic rendering attachment info
	vk::PipelineRenderingCreateInfo attachmentInfo = {
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = &surfaceFormat.format
	};

	// null layout info
	vk::raii::PipelineLayout pipelineLayout = nullptr;
	vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {
		.setLayoutCount = 0,
		.pushConstantRangeCount = 0
	};
	pipelineLayout = vk::raii::PipelineLayout(VlogicalDevice, pipelineLayoutInfo);

	vk::GraphicsPipelineCreateInfo pipelineInfo = {
		.pNext = &attachmentInfo,
		.stageCount = 2,
		.pStages = shaderStagesInfo,
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssemblyInfo,
		.pViewportState = &viewportInfo,
		.pRasterizationState = &rasterizationInfo,
		.pMultisampleState = &multisamplingInfo,
		.pColorBlendState = &colorBlendingInfo,
		.layout = pipelineLayout,
		.renderPass = nullptr
	};

	VgraphicsPipeline = vk::raii::Pipeline(VlogicalDevice, nullptr, pipelineInfo);
	logger.logInformation("Pipeline created");
}

void Renderer::createVCommandPool() {
	vk::CommandPoolCreateInfo commandPoolCreateInfo = {
		.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		.queueFamilyIndex = graphicsFamilyIndex
	};

	VcommandPool = vk::raii::CommandPool(VlogicalDevice, commandPoolCreateInfo);
}

void Renderer::createVCommandBuffers() {
	vk::CommandBufferAllocateInfo commandBufferAllocateInfo = {
		.commandPool = VcommandPool,
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = FRAMES_IN_FLIGHT
	};

	VcommandBuffers = vk::raii::CommandBuffers(VlogicalDevice, commandBufferAllocateInfo);
}

void Renderer::createVSyncObjects() {
	for(uint32_t i = 0; i < Vswapchain.getImages().size(); ++i) {
		VrenderReady.push_back(vk::raii::Semaphore(VlogicalDevice, vk::SemaphoreCreateInfo()));
		VrenderDone.push_back(vk::raii::Semaphore(VlogicalDevice, vk::SemaphoreCreateInfo()));
	}

	for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
		VcommandBufferCompleted.push_back(vk::raii::Fence(VlogicalDevice, { .flags = vk::FenceCreateFlagBits::eSignaled }));
	}

	logger.logInformation(std::string("Command pool, command buffers, and sync objects created (Frames in flight:") + 
		std::to_string(FRAMES_IN_FLIGHT) + ", semaphores:" + std::to_string(VrenderDone.size()) + ")");
}
