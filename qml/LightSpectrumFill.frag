#version 440
#extension GL_ARB_shading_language_include : enable

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float minNm;
    float maxNm;
    bool useP3;
};

const vec3 cmf[] = {
    #include "../CMF10deg.inc"
};

const float cmfMinNm = 360.0;

#include "Helpers.glsl"

void main()
{
    int idx = int(round(qt_TexCoord0.x * (maxNm - minNm) - (cmfMinNm - minNm)));
    if (idx >= 0 && idx < cmf.length())
    {
        vec3 val = cmf[idx];
        val = E_TO_D65 * val;
        val = useP3 ? XYZ_D65_TO_P3 * val : XYZ_D65_TO_sRGB * val;
        val /= 1.5;
        val = clamp(val, 0.0, 1.0);
        apply_srgb_tf(val);
        fragColor = vec4(val, 1.0) * qt_Opacity;
    }
    else
    {
        fragColor = vec4(vec3(0.0), 1.0) * qt_Opacity;
    }
}
