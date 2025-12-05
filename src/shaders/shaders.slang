static float2 positions[3] = float2[](
	float2(-0.5, 0.5),
	float2(0.5, 0.5),
	float2(0.0, -0.5)
);

static float4 colours[3] = float4[](
	float4(1.0, 0.0, 0.0, 1.0),
	float4(0.0, 1.0, 0.0, 1.0),
	float4(0.0, 0.0, 1.0, 1.0),
);

struct Vertex {
    float4 colour;
    float2 position : SV_Position;
};

[shader("vertex")]
Vertex vertexMain(uint id : SV_VertexID) {
    Vertex vertex;
    vertex.position = positions[id];
    vertex.colour = colours[id];
    return vertex;
}

[shader("fragment")]
float4 fragmentMain(Vertex interpolatedVertex) : SV_Target {
    return interpolatedVertex.colour;
}