#version 450
#extension GL_EXT_draw_instanced : enable
layout(location = 0) out vec4 outColor;
layout (location = 0) in flat int fragInstanceID;
vec4 getRandomColor(int index) {
    // Convert index to float for hashing
    float hashed = float(index) * 0.61803398875;// Use the golden ratio for distribution
    hashed = fract(sin(hashed) * 43758.5453123);// Sin-based hash function

    // Generate RGB values from the hashed number
    float r = fract(hashed * 13.0);
    float g = fract(hashed * 17.0);
    float b = fract(hashed * 23.0);

    return vec4(r, g, b, 1.0);
}
void main() {
    vec4 color = getRandomColor(fragInstanceID);
    outColor = color;
}