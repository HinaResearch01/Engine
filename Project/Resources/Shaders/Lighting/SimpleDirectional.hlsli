float3 EvalDirectionalLight(
    float3 albedo,
    float3 N,
    float3 V,
    float3 L,
    float3 radiance,
    float refl
)
{
    float NdotL = saturate(dot(N, L));
    float3 diffuse = albedo * radiance * NdotL;

    float3 H = normalize(L + V);
    float spec = pow(saturate(dot(N, H)), 32.0f) * refl;

    return diffuse + radiance * spec;
}