# Graphics Programming 1 - Raytracer

This project contains the start code for the raytracer being built in the first 5 courseweeks of Graphics Programming 1. 

Each week further extensions are made to the raytracer to support objects, lighting, camera movement, ...

- What did I do?
I implemented all of the course material up to the last chapter: Optimizations.
Additionally, I added reflections, BHV, Faster rays optimization structure and binning.


- PRECOMPILER DIRECTIVES
I didn't include this directives in the cmake because it would apparently slow down the program significantly.

In the main.cpp, #USE_SIMPLE_OUTPUT directive can be used to obtain a simpler fps logging.

In the Renderer.cpp, #USE_PARALLEL_EXECUTION directive can be use to toggle between multiple and single thread execution.

In the Material.h, #USE_REFLECTIONS directive can be used to enable or disable reflections and #MAX_REFLECTION_BOUNCES to specify how many bounces the reflection ray is going to make.

- Details about my work
The first layer of optimization I did was the code restructuring. The Renderer especially, has a better encapsulated design that minimizes the pointers and references passed to functions. Additionally, I separated the ray processing from the render pixel in a recursive function to avoid rewriting code for the reflection bounces.

I completely deleted the slab-test implementation in the TriangleMesh to substitute it with the BVH.	At every transform, I create a BVHBuilder that will update my BVHNodes to make bounding boxes update with the mesh. For static meshes scenes.
I also implemented binning and faster BVH structure optimization to further enhance the bounding box search for the TriangleMesh_HitTest.

My biggest achievement is that I was able to never compromise the code readability and structure to introduce any new concept to the ray-tracer.
