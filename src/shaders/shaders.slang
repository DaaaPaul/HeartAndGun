struct VertexAttributes {
    float2 inPosition;
    float4 inColour;
};

struct Vertex {
    float4 colour;
    float4 position : SV_Position;
};

struct UniformBuffer {
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};
ConstantBuffer<UniformBuffer> ubo;

[shader("vertex")]
Vertex vertexMain(VertexAttributes va) {
    Vertex vertex;

    vertex.position = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(va.inPosition, 0.0, 1.0))));
    vertex.colour = va.inColour;

    return vertex;
}

[shader("fragment")]
float4 fragmentMain(Vertex interpolatedVertex) : SV_Target {
    return interpolatedVertex.colour;
}