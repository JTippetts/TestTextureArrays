#define URHO3D_CUSTOM_MATERIAL_UNIFORMS
#define URHO3D_DISABLE_DIFFUSE_SAMPLING
#define URHO3D_DISABLE_NORMAL_SAMPLING
#define URHO3D_DISABLE_SPECULAR_SAMPLING
#define URHO3D_DISABLE_EMISSIVE_SAMPLING

#include "_Config.glsl"
#include "_Uniforms.glsl"

UNIFORM_BUFFER_BEGIN(4, Material)
    DEFAULT_MATERIAL_UNIFORMS
    UNIFORM(half2 cDetailTiling)
UNIFORM_BUFFER_END(4, Material)

#include "_Material.glsl"

SAMPLER(1, sampler2DArray sDetailMaps)

VERTEX_OUTPUT_HIGHP(vec2 vDetailTexCoord)

#ifdef URHO3D_VERTEX_SHADER
void main()
{
    VertexTransform vertexTransform = GetVertexTransform();
    FillVertexOutputs(vertexTransform);
    vDetailTexCoord = vTexCoord * cDetailTiling;
}
#endif

#ifdef URHO3D_PIXEL_SHADER
void main()
{
    SurfaceData surfaceData;

    FillSurfaceCommon(surfaceData);
    FillSurfaceNormal(surfaceData);
    FillSurfaceMetallicRoughnessOcclusion(surfaceData);
    FillSurfaceReflectionColor(surfaceData);
    FillSurfaceBackground(surfaceData);

    half3 weights = texture2D(sDiffMap, vTexCoord).rgb;
    half sumWeights = weights.r + weights.g + weights.b;
    weights /= sumWeights;
    surfaceData.albedo = cMatDiffColor * (
        weights.r * texture(sDetailMaps, vec3(vDetailTexCoord, 0)) +
        weights.g * texture(sDetailMaps, vec3(vDetailTexCoord, 1)) +
        weights.b * texture(sDetailMaps, vec3(vDetailTexCoord, 2))
    );
    surfaceData.albedo.rgb = GammaToLightSpace(surfaceData.albedo.rgb);

    surfaceData.specular = cMatSpecColor.rgb;
#ifdef URHO3D_SURFACE_NEED_AMBIENT
    surfaceData.emission = cMatEmissiveColor;
#endif

    half3 finalColor = GetFinalColor(surfaceData);
    gl_FragColor.rgb = ApplyFog(finalColor, surfaceData.fogFactor);
    gl_FragColor.a = GetFinalAlpha(surfaceData);
}
#endif
