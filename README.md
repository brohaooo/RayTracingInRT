# RayTracingInRT

## About it
Still coding for this. It's an openGL rendering application which combines traditional rasterization pipeline (openGL), software ray-tracing (adapted from Ray-tracing in one weekend series), and real-time ray-tracing using compute shader as accelerations.

It uses two-layered BVH acceleration for ray-object hit calculation (TLAS & BLAS), it supports meshes which can be loaded from \.obj and textures from \.png or \.jpg. It also supports dynamic scenes with moving objects (by only updating TLAS while BLAS keeps static).

## Controls
You can press 1/2/3 to switch between default rasterization/ CPU ray-tracing/ GPU accelerated ray-tracing.

![](resource/examples/sample_5.png)
![](resource/examples/sample_6.png)
![](resource/examples/sample_4.png)


