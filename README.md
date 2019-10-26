# Raytracer

A toy raytracer that produces a scene composed of four coloured spheres placed
on a blank surface. The recursive ray tracing algorithm supports multiple
lights, reflections and shadows.

- Blinn-Phong shading
- Materials with diffuse, specular and reflection parameters
- Supersampling anti-aliasing
- Gamma correction

# Instructions

## Prerequisites

- C compiler with support for C11
- Make

Tested with gcc 9.3.0, clang 10.0 and GNU make 4.2.1 on Ubuntu 20.04.3 LTS.

## Build

Compile, link, execute and write the image to `image.ppm` (24 MB).

```
make
```

The program writes image data to standard output. If you run the executable
directly, redirect the output to a file.

```
./raytracer > image.ppm
```

## Other `make` targets

| Target    | Description                                            |
|-----------|--------------------------------------------------------|
| clean     | Delete all build files, including the generated image. |
| deps      | Print header dependencies to update `Makefile`.        |
| raytracer | Compile and link, but don't generate the image.        |
