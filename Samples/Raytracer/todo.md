

Raytracing Pipeline Overview 
0 - build acceleration structure
1 - ray trace primary rays (create albedo/normal/depth)
2 - low resolution edge detection
3 - ray trace secondary rays
    3.1 - Adaptive sampling use variance map & edge detection to decide how many rays per pixels. (from 0.25 for flat surfaces to 4 rays per pixels for penumbra)
    3.2 - create light map
    3.3 - update variance map for current history
4 - denoising 
    4.1 - temporal re projection + use edge detection to apply taa
    4.2 - blur light map
5 - bloom / tone mapping / post processings...

Parameters:
- Frame:
  - Frame number
  - max global illumination bounce
  - max global illumination samples
  - max lights samples
- Camera matrices History
- Albedo History
- NormalDepth History
- 
TODO:

-Importance sampling:
    - [ ]Variance map
    - [ ]Fractional sampling per pixels (1/2 frames,1/4 frames) separated by thread group id
    - [ ]Sample count texture
- Pbr lighting equation
- Bloom post process
- Low resolution edge detection
- Denoising :
  - [ ] separate raygen & denoising shader
  - [ ] separate albedo/light texture to avoid blending textures
  - [ ] separate specular/roughness texture
  - [x] temporal Re projection
  - [ ] edge detection + taa
- Multiple light sources support