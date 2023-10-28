/*struct VertexInput
{
    float3 position : POSITION;
    float4 color    : COLOR;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};

VertexOutput mainVS(VertexInput input)
{
    VertexOutput output;
    output.position = float4(input.position, 1.0f);
    output.color = input.color;
    return output;
}

// Pixel Shader
struct PixelInput
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};*/

/*float4 mainPS(PixelInput input) : SV_TARGET
{
    return input.color;
}*/