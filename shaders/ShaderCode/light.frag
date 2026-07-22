#version 450
#define PI 3.14158265359

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;

    return a2 / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    // 直接光照下的重映射参数 k
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness); // 视线方向遮蔽 (masking)
    float ggx1  = GeometrySchlickGGX(NdotL, roughness); // 光照方向遮蔽 (shadowing)
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 CookTorranceBRDF(vec3 N, vec3 V, vec3 L, vec3 albedo, float metallic, float roughness) 
{
    vec3 H = normalize(V + L); // 半角向量 (Half Vector)

    // 计算基础反射率 F0 (非金属约为0.04，金属则使用albedo颜色)
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // 计算三大核心项
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), F0);

    // 组合镜面反射 (Specular)
    vec3 nominator   = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // +0.0001 避免除零错误
    vec3 specular    = nominator / denominator;

    // 漫反射 (Diffuse) 与能量守恒
    vec3 kS = F;                   // 镜面反射比例
    vec3 kD = vec3(1.0) - kS;      // 漫反射比例
    kD *= (1.0 - metallic);        // 金属没有漫反射

    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = kD * albedo / PI; // 朗伯(Lambertian)漫反射模型

    // 最终输出：(漫反射 + 镜面反射) * 光照衰减(NdotL)
    return (diffuse + specular) * NdotL;
}

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput uAlbedo;
layout(input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput uNormal;
layout(input_attachment_index = 1, set = 0, binding = 2) uniform subpassInput uPos;

layout(location = 0) out vec4 outColor;
vec3 light = vec3(0.0, 0.0, 0.0);
vec3 lightColor = vec4(500.0).rgb;

float my_metallic = 1.0;
float my_roughness = 0.2;
void main()
{
    vec3 viewPos = subpassLoad(uPos).rgb;
    vec3 N = normalize(subpassLoad(uNormal).rgb);
    vec3 V = normalize(vec3(0.0)-viewPos);
    // vec3 albedo = subpassLoad(uAlbedo).rgb;
     vec3 albedo = subpassLoad(uAlbedo).rgb;


    vec3 Lo = vec3(0.0);

    vec3 L = normalize(light - viewPos);

    float _distance = length(light - viewPos);
    float atteneation = 1.0 / (_distance * _distance);
    vec3 radiance = lightColor * atteneation;

    vec3 brdfResult = CookTorranceBRDF(N, V, L, albedo, my_metallic, my_roughness);

    Lo += brdfResult * radiance;

    vec3 ambient = vec3(0.03) * albedo;
    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, 1.0);
}
