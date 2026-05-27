#version 440
#extension GL_ARB_shading_language_include : enable

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    bool useP3;
};

#include "Helpers.glsl"

void main()
{
    // The C++ code uses shape.width and shape.height mapping directly:
    // x = X/sum (goes from 0 to 1 basically)
    // y = 1.0 - Y/sum (goes from 0 to 1 basically, but inverted)

    // So our fragment UVs correspond exactly to x and (1.0 - y)
    float x = qt_TexCoord0.x;
    float y = 1.0 - qt_TexCoord0.y;

    // Use XYZ coordinates
    // We normalize to X+Y+Z = 1.0, which means X=x, Y=y, Z=1-x-y.
    vec3 val = vec3(x, y, 1.0 - x - y);
    val = clamp(useP3 ? XYZ_D65_TO_P3 * val : XYZ_D65_TO_sRGB * val, 0.0, 1.0);
    apply_srgb_tf(val);
    fragColor = vec4(val, 1.0) * qt_Opacity;
}
