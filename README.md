# Graphics Programming 1 - Raytracer

This project contains the start code for the raytracer being built in the first 5 courseweeks of Graphics Programming 1. 

Each week further extensions are made to the raytracer to support objects, lighting, camera movement, ...

- WHAT DID I DO?
I implemented all of the course material up to the last chapter: Optimizations.
Additionally, I added reflections, BHV, Faster rays optimization structure and binning.
Furthermore, I implemented global illumination and soft shadows.


- COMMANDS
WASD -> lateral movement
EQ -> up/down movement
Hold MouseLeft -> camera movement

F2 -> toggle shadows
F3 -> toggle lighting mode
F4 -> toggle global illumination 
F5 -> toggle soft shadows


- PRECOMPILER DIRECTIVES
I didn't include this directives in the cmake because it would apparently slow down the program significantly.

In the main.cpp, #USE_SIMPLE_OUTPUT directive can be used to obtain a simpler fps logging.

In the Renderer.cpp, #USE_PARALLEL_EXECUTION directive can be use to toggle between multiple and single thread execution.
#MAX_RAY_BOUNCES specifies how many bounces a ray can do for indirect lighting and reflection sampling.
#INDIRECT_SAMPLING specifies how many samples are taken per hitPoint for global illumination.
#INDIRECT_LIGHTING_FACTOR specifies how much indirect samples will affect the final color.
#INDIRECT_MAX_DEVIATION specifies the scattering of the indirect lighting samples.
#SHADOW_SAMPLES specifies how many samples are taken for soft shadows.
#SHADOW_RADIUS specifies how widely samples can spread from the original hit point for soft shadows.

In the Material.h, #USE_REFLECTIONS directive can be used to enable or disable reflections.


- Details about my work
The first layer of optimization I did was the code restructuring. The Renderer especially, has a better encapsulated design that minimizes the pointers and references passed to functions. Additionally, I separated the ray processing from the render pixel in a recursive function to avoid rewriting code for the reflection bounces.

I completely deleted the slab-test implementation in the TriangleMesh to substitute it with the BVH.	At every transform, I create a BVHBuilder that will update my BVHNodes to make bounding boxes update with the mesh. For static meshes scenes.
I also implemented binning and faster BVH structure optimization to further enhance the bounding box search for the TriangleMesh_HitTest.

The global illumination through indirect lighting can be customized throughout the directives. A lot combination can be chosen. Keep in mind that while increasing the samples or the bounces, you should also decrease the lighting factor to avoid overexposure.
Increasing the max deviation will give better diffusion but also increase the noise. On the other end, reducing the max deviation will give a very clean end result but the indirect illumination will start to act like specular light.

Soft shadows will be smoother the more samples we take, but performances go down as well. Smaller spread radius makes the noise less noticeable.

This is everything for my ray tracer. My biggest achievement is that I was able to never compromise the code readability and structure to introduce any new concept.
