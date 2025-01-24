#ifndef HISTORY_COUNT
#define HISTORY_COUNT 16
#endif
#include "../raytracing/data_structures.glsl"
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout (binding = 0, set = 0, rgba32f) readonly uniform image2D historyAlbedo[HISTORY_COUNT];
layout (binding = 1, set = 0, rgba32f) readonly uniform image2D historyNormalDepth[HISTORY_COUNT];
layout (binding = 2, set = 0, rgba32f) readonly uniform image2D historyMotion[HISTORY_COUNT];
layout (binding = 3, set = 0, rgba32f) readonly uniform image2D historyPosition[HISTORY_COUNT];

layout(binding = 4, set = 0, rgba32f) readonly uniform image2D inputImage;
layout(binding = 5, set = 0, rgba32f) writeonly uniform image2D outputImage;
layout (binding = 6, set = 0) uniform FrameUBO {
    Frame data;
} frame;
float gaussianWeight(int x, float sigma) {
    return exp(-(x * x) / (2.0 * sigma * sigma));
}
const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
//const float weights[2] = float[](0.33, 0.33);
void blur(ivec2 offsetDirection)
{
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    int index = int(floor(mod(frame.data.index, HISTORY_COUNT)));
    vec4 normalDepth = imageLoad(historyNormalDepth[index], coord);
    vec4 color = vec4(0.0);
    float additionalWeight = 0.0;
    for (int i = 1; i <= 4; ++i) {

        ivec2 offset = offsetDirection * i;

        //positive offset
        vec4 normalDepthPositive = imageLoad(historyNormalDepth[index], coord + offset);
        if (dot(normalDepthPositive.rgb, normalDepth.rgb) > 0.90 && abs(normalDepthPositive.w -normalDepth.w) < 0.1)
        {
            color += imageLoad(inputImage, coord + offset) * weights[i];
        }
        else {
            //color+= vec4(1, 0, 0, 1);
            additionalWeight += weights[i];
        }


        //negative offset
        vec4 normalDepthNegative = imageLoad(historyNormalDepth[index], coord - offset);
        if (dot(normalDepthNegative.rgb, normalDepth.rgb) > 0.90 && abs(normalDepthNegative.w -normalDepth.w) < 0.1)
        {
            color += imageLoad(inputImage, coord - offset) * weights[i];

        }
        else {
            //color+= vec4(0, 1, 0, 1);
            additionalWeight += weights[i];
        }
    }

    color +=imageLoad(inputImage, coord) * (weights[0] + additionalWeight);
    imageStore(outputImage, coord, color);
}
