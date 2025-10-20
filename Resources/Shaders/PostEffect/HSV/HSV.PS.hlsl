#include "../PostEffect.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    float4 color;
    float hue;
    float saturation;
    float value;
};
ConstantBuffer<Material> gMaterial : register(b1);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};


float3 ToHSV(float3 rgb)
{
    float r = rgb.r;
    float g = rgb.g;
    float b = rgb.b;

    // Find the maximum and minimum values among r, g, and b
    float maxVal = max(r, max(g, b));
    float minVal = min(r, min(g, b));
    float h = maxVal - minVal;

    // Initialize HSV components
    float h_out = 0.0f;
    float s_out = 0.0f;
    float v = maxVal;

    if (h > 0.0f)
    {
        if (maxVal == r)
        {
            h_out = (g - b) / h;
            if (h_out < 0.0f)
            {
                h_out += 6.0f;
            }
        }
        else if (maxVal == g)
        {
            h_out = 2.0f + (b - r) / h;
        }
        else
        {
            h_out = 4.0f + (r - g) / h;
        }
        h_out /= 6.0f;
    }

    s_out = (maxVal - minVal);
    if (maxVal != 0.0f)
    {
        s_out /= maxVal;
    }

    return float3(h_out, s_out, v);
}
float3 ToRGB(float3 hsv)
{
    float h = hsv.r;
    float s = hsv.g;
    float v = hsv.b;

    float r = v;
    float g = v;
    float b = v;

    if (s > 0.0f)
    {
        h *= 6.0f; // Convert hue to [0, 6)
        int i = (int) h;
        float f = h - (float) i;
        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));

        switch (i)
        {
            case 0:
                g = t;
                b = p;
                break;
            case 1:
                r = q;
                b = p;
                break;
            case 2:
                r = p;
                b = t;
                break;
            case 3:
                r = p;
                g = q;
                break;
            case 4:
                r = t;
                g = p;
                break;
            case 5:
                g = p;
                b = q;
                break;
            default:
                // In case h is not in the range [0, 6) due to floating-point precision issues
                break;
        }
    }

    return float3(r, g, b);
}
float WrapValue(float value, float minRange, float maxRange)
{
    float range = maxRange - minRange;
    float modValue = fmod(value - minRange, range);
    
    if (modValue < 0)
    {
        modValue += range;
    }
    return minRange + modValue;
}


PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    float3 hsv = ToHSV(textureColor.rgb);
    
    
    // ‚±‚±‚Å’²®‚·‚é
    hsv.x += gMaterial.hue;
    hsv.y += gMaterial.saturation;
    hsv.z += gMaterial.value;
    
    hsv.x = WrapValue(hsv.x, 0.0f, 1.0f);
    hsv.y = saturate(hsv.y);
    hsv.z = saturate(hsv.z);
    
    float3 rgb = ToRGB(hsv);
    
    
    output.color.rgb = rgb;
    output.color.a = textureColor.a;
    
    return output;
}