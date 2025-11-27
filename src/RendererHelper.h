#pragma once

#include "Renderer.h"

class RendererHelper {
	friend class Renderer;

private:
	bool verifyValidationLayers(Renderer& renderer) {
		std::vector<vk::LayerProperties> layerProperties = renderer.Vcontext.enumerateInstanceLayerProperties();
	}
};

