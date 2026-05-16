module Lighting;

static const float PI = 3.14159265358979323846f;

// Blinn-Phong
// ---------------------------------------------------------------

public float RoughnessToShininess(float roughness)
{
    return max(0.001f, (2.0f / (roughness * roughness)) - 2.0f);
}

// Blinn-Phong model with some modifications to make both diffuse 
// and specular lighting have better energy conservation (Cook-Torrance).
public void GetBlinnPhongBRDF(in float3 N,
                       in float3 V,
                       in float3 L,
                       in float3 base_color,
                       in float3 specular_color,
                       in float  shininess,
                       in float3 light_color,
                       out float3 diffuse,
                       out float3 specular)
{
    float3 H = normalize(V + L);

    float NdotL = saturate(dot(N, L));
    float NdotH = saturate(dot(N, H));

    // Diffuse
    diffuse = base_color * (NdotL / PI) * light_color;

    // Specular
    float spec = pow(NdotH, shininess);
    float normalization = (shininess + 2.0f) / (2.0f * PI);

    specular = specular_color * spec * normalization * NdotL * light_color;
}

// Disney BRDF
// ---------------------------------------------------------------

float SchlickFresnel(float u)
{
    float m = clamp(1.0f - u, 0.0f, 1.0f);
    float m2 = m * m;
    return m2 * m2 * m; // pow(m,5)
}

float Gtr1(float NdotH, float alpha)
{
    alpha = min(alpha, 0.999f);

    float a2 = alpha * alpha;
    float t = 1.0f + (a2 - 1.0f) * NdotH * NdotH;
    return (a2 - 1.0f) / max(PI * log(a2) * t, 1e-5f);
}

float Gtr2(float NdotH, float alpha)
{
    float a2 = alpha * alpha;
    float t = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
    return a2 / (PI * t * t);
}

float SmithGGX(float NdotV, float alpha)
{
    float a2 = alpha * alpha;
    float b = NdotV * NdotV;
    return 1.0f / (NdotV + sqrt(a2 + (1.0f - a2) * b));
}

float SmithGGXAnisotropy(float NdotV, float ax)
{
    return 1.0f / (NdotV + sqrt(ax * ax * (1 - NdotV * NdotV) + NdotV * NdotV));
}

float GeometrySpecAA(float3 N, float roughness)
{
    // The higher the more aggressive the filtering, but blurrier reflections
    const float k = 0.3f;

    float3 dndu = ddx(N);
    float3 dndv = ddy(N);

    float variance = max(dot(dndu, dndu), dot(dndv, dndv));

    float r2 = roughness * roughness;
    r2 += variance * k;

    return saturate(sqrt(r2));
}

public void GetDisneyBRDF(in float3 N,
                   in float3 V,
                   in float3 L,
                   in float3 base_color,
                   in float3 specular_color,
                   in float  roughness,
                   in float  clearcoat_strength,
                   in float3 light_color,
                   out float3 diffuse,
                   out float3 specular)
{
    float3 H = normalize(V + L);

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    float NdotH = saturate(dot(N, H));
    float LdotH = saturate(dot(L, H));

    // Diffuse (Disney Burley)
    float FL = SchlickFresnel(NdotL);
    float FV = SchlickFresnel(NdotV);
    float Fss90 = LdotH * LdotH * roughness;
    float Fss = lerp(1.0f, Fss90, FL) * lerp(1.0f, Fss90, FV);
    float diffuse_term = (1.0f / PI) * Fss;

    diffuse = base_color * diffuse_term * NdotL * light_color;

    float filtered_roughness = GeometrySpecAA(N, roughness);

    // Specular (GGX Cook-Torrance)
    float alpha = max(0.001f, filtered_roughness * filtered_roughness);
    float D = Gtr2(NdotH, alpha);
    float FH = SchlickFresnel(LdotH);
    float3 F = lerp(specular_color, 1.0f.xxx, FH);
    float G = SmithGGXAnisotropy(NdotL, alpha) * SmithGGXAnisotropy(NdotV, alpha);
    float3 spec_term = (D * G * F) / max(4.0f * NdotV * NdotL, 1e-5f);

    specular = spec_term * NdotL * light_color;

    // Clearcoat
    float Dr = Gtr1(NdotH, 0.01f);
    float Fr = lerp(0.04f, 1.0f, FH);
    float Gr = SmithGGX(NdotL, 0.25f) * SmithGGX(NdotV, 0.25f);
    float clearcoat = clearcoat_strength * 0.25f * Dr * Fr * Gr / max(4.0f * NdotV * NdotL, 1e-5f);

    specular += clearcoat * NdotL * light_color;
}