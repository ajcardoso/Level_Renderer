// an ultra simple hlsl pixel shader
#pragma pack_matrix(row_major)

struct My_Vert
{
    float3 position : POSITION;
    float3 uv : LOCATION;
    float3 normal : NORMAL;

};

struct ATTRIBUTES
{
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
    float4 lightDirc, lightColor; // lighting info
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

float4 main(OutputToRasterizer output) : SV_TARGET
{
    // normalize the normal and store in a variable
    float3 norm = normalize(output.normW);
    
    // direct light energy based on light type and surface normal/position
    float3 direct = saturate(dot(norm, -lightDirc.xyz)) * lightColor.xyz;
   
    // indirect light from ambient light attribute (attenuated if point/spot)
    float3 indirect = saturate(materials.Ka * sunAmbient.xyz);
   
    // diffuse reflectivity
    float3 diffuse = materials.Kd;
    
    //// VIEWDIR = NORMALIZE(CAMWORLDPOS– SURFACEPOS)
    float3 viewDir = normalize(cameraPos.xyz - output.posW);
   
    //// SPECULARINTENSITY = POW( CLAMP( DOT( REFELECTION, VIEWDIR)), SPEC EXPONENT)
    float specPower = (materials.Ns > 0) ? materials.Ns + 0.000001f : 90;
    float3 specular = pow(saturate(dot(norm, viewDir)), specPower) * materials.Ks;
   
    // emissive reflectivity
    float3 emissive = materials.Ke;
    
    //return saturate(totalDirect + totalIndirect) * diffuse + totalReflected + emissive
    float3 result = saturate(direct + indirect) * diffuse + specular + emissive;

    // dissolve (transparency) 
    return float4(result, materials.d);
    
    // VVVV Assignment 2 VVVV
    //// VIEWDIR = NORMALIZE(CAMWORLDPOS– SURFACEPOS)
    //float3 viewDir = normalize(cameraPos.xyz - output.posW);
    //// HALFVECTOR = NORMALIZE((-LIGHTDIR) + VIEWDIR)
    //float3 halfVector = normalize(-lightDirc.xyz + viewDir);
    //// INTENSITY = MAX( CLAMP( DOT( NORMAL, HALFVECTOR ))SPECULARPOWER , 0 )
    //float instensity = max(saturate(dot(norm, halfVector)), 0);
    //// diffuse 
    //float3 diffuse = materials.Kd * instensity;
    //// reflection
    //float3 _reflect = reflect(lightDirc.xyz, norm);
    //// SPECULARINTENSITY = POW( CLAMP( DOT( REFELECTION, VIEWDIR)), SPEC EXPONENT)
    //float3 specIntensity = pow(saturate(dot(norm, viewDir)), materials.Ns + 0.000001f) * materials.Ks;
    //// REFLECTEDLIGHT = LIGHTCOLOR * SPECULARINTENSITY * INTENSITY
    //float3 reflectedLight = (diffuse + specIntensity) * lightColor.xyz;
    //return float4(reflectedLight, 1.0f);
     
}