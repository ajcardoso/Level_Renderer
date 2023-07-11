// an ultra simple hlsl vertex shader
#pragma pack_matrix(row_major)

struct My_Vert
{
    float3 position : POSITION; 
    float3 uv : LOCATION; 
    float3 normal : NORMAL;

};

struct ATTRIBUTES {
    float3 Kd;
    float d;
    float3 Ks;
    float Ns;
    float3 Ka;
    float sharpness;
    float3 Tf;
    float Ni;
    float3 Ke;
    uint illum;
};

cbuffer SceneData : register(b0)
{
    float4 sunAmbient, cameraPos;
    float4 lightDirc, lightColor;        // lighting info
    matrix viewMatrix, projectionMatrix; // viewing info
};

cbuffer MeshData : register(b1)
{
    matrix worldMatrix;
    ATTRIBUTES materials;
    
};

struct OutputToRasterizer
{
    float4 posH : SV_POSITION; // position in homogenous projection space
    float3 posW : WORLD;       // position in world space (for lighting)
    float3 normW : NORMAL;     // normal in world space (for lighting)
};


OutputToRasterizer main(My_Vert inputVertex)
{

    OutputToRasterizer _output = { float4(0.0f, 0.0f, 0.0f, 0.0f), float3(0.0f, 0.0f, 0.0f), float3(0.0f, 0.0f, 0.0f) };
   
    float4 worldOut = mul(float4(inputVertex.position, 1.0f), worldMatrix); 
    float4 viewOut = mul(worldOut, viewMatrix);
    float4 projectionOut = mul(viewOut, projectionMatrix);
   
    _output.posW = worldOut.xyz;
    
    _output.posH = projectionOut;
    
    float3 normalVal = mul(inputVertex.normal, (float3x3)worldMatrix);
   
    _output.normW = normalize(normalVal);

    return _output;
}