struct VertexAttributes {
    float2 inPosition;
    float4 inColour;
};

struct Vertex {
    float4 colour;
    float4 position : SV_Position;
};

[shader("vertex")]
Vertex vertexMain(VertexAttributes va) {
    Vertex vertex;

    vertex.position = float4(va.inPosition, 0.0, 1.0);
    vertex.colour = va.inColour;

    return vertex;
}

[shader("fragment")]
float4 fragmentMain(Vertex interpolatedVertex) : SV_Target {
    return interpolatedVertex.colour;
}