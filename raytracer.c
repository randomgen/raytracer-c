#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "vector.h"

#define PI 3.14159

/* Output image dimension and corresponding framebuffer */
#define WIDTH 1920
#define HEIGHT 1080

/* Supersampling anti-aliasing sub-pixel grid */
#define AA_GRID_WIDTH 4
#define AA_GRID_HEIGHT 4
#define AA_GRID_SIZE AA_GRID_WIDTH * AA_GRID_HEIGHT

/* Ray tracing parameters */
#define FOV PI / 4.0
#define REFLECTION_DEPTH 8

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static const char *ERROR_FRAMEBUFFER_MEM_ALLOCATION =
    "Error: can't allocate memory for image with dimension %dx%d.\n";

struct color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct gradient {
    struct vector start;
    struct vector end;
};

struct sphere {
    struct vector center;
    double radius;
};

struct light {
    struct vector origin;
    struct vector intensity;
};

struct material {
    struct vector diffuse;
    struct vector specular;
    struct vector reflection;
    double smoothness;
};

struct object {
    struct sphere shape;
    struct material material;
};

struct scene {
    struct vector camera;
    struct gradient background;
    struct light *lights_first;
    struct light *lights_last;
    struct object *objects_first;
    struct object *objects_last;
};

struct ray {
    struct vector origin;
    struct vector direction;
};

struct intersection {
    int intersected;
    double distance;
    struct vector point;
};

struct scene_intersection {
    struct intersection intersection;
    struct object object;
};

static struct light lights[] = {
    /*
     origin                             intensity
    */
    {{       8.0,      6.0,      0.0},  {0.3, 0.3, 0.2}},
    {{   -5000.0,  10000.0, -10000.0},  {0.6, 0.6, 0.6}},
};

static struct object objects[] = {
    /*
     shape:                                      material:
      origin                           radius     diffuse             specular            reflection          smooth.
    */
    {{{     -4.5,      0.8,     25.0},     0.8}, {{0.80, 0.80, 0.80}, {1.00, 1.00, 1.00}, {0.30, 0.30, 0.30}, 1024.0}},
    {{{      2.5,      0.8,     15.0},     0.8}, {{0.80, 0.80, 0.80}, {1.00, 1.00, 1.00}, {0.30, 0.30, 0.30}, 1024.0}},
    {{{     -1.0,      1.0,     14.0},     1.0}, {{0.80, 0.00, 0.00}, {0.15, 0.15, 0.15}, {0.00, 0.00, 0.00},   32.0}},
    {{{      0.0,      1.5,     17.0},     1.5}, {{0.00, 0.20, 0.80}, {0.30, 0.30, 0.30}, {0.05, 0.05, 0.05},  128.0}},
    {{{      0.0, -10000.0,      0.0}, 10000.0}, {{1.00, 1.00, 1.00}, {0.00, 0.00, 0.00}, {0.00, 0.00, 0.00},   32.0}},
};

static struct scene scene = {
    .camera = {0.0, 2.0, 0.0},
    .background = {
        .start = {0.7, 0.8, 1.0},
        .end   = {0.2, 0.3, 1.0}
    },
    .lights_first  = lights,
    .lights_last   = lights + ARRAY_SIZE(lights),
    .objects_first = objects,
    .objects_last  = objects + ARRAY_SIZE(objects)
};

static struct vector budge(struct vector point, struct vector direction)
{
    return addv(point, mulv(direction, 0.01));
}

static struct vector linear_interpolation(struct gradient gradient, double x)
{
    return addv(mulv(gradient.start, 1.0 - x), mulv(gradient.end, x));
}

static struct vector sphere_normal(struct sphere sphere, struct vector point)
{
    return normalize(subv(point, sphere.center));
}

static struct vector reflect(struct vector incident, struct vector normal)
{
    return subv(mulv(mulv(normal, dotp(incident, normal)), 2.0), incident);
}

static double irradiance(struct vector light, struct vector normal)
{
    return fmax(dotp(normal, light), 0.0);
}

static struct vector diffuse_illumination(struct vector light,
                                          struct vector normal,
                                          struct material material)
{
    double e = irradiance(light, normal);
    return mulv(material.diffuse, e);
}

static struct vector specular_illumination(struct vector light,
                                           struct vector view,
                                           struct vector normal,
                                           struct material material)
{
    struct vector half_vector = normalize(addv(light, view));
    double e = irradiance(half_vector, normal);
    return mulv(material.specular, powf(e, material.smoothness));
}

static struct vector blinn_phong(struct vector light,
                                 struct vector brightness,
                                 struct vector view,
                                 struct vector normal,
                                 struct material material)
{
    struct vector diff = diffuse_illumination(light, normal, material);
    struct vector spec = specular_illumination(light, view, normal, material);
    return hadp(addv(diff, spec), brightness);
}

/*
 * Ray/Sphere intersection optimized geometric algorithm (Real-Time Rendering
 * 3rd Ed. §16.6.2).
 */
static struct intersection ray_sphere_intersection(struct ray ray,
                                                   struct sphere sphere)
{
    struct vector l = subv(sphere.center, ray.origin);
    double p = dotp(l, ray.direction);
    double l2 = dotp(l, l);
    double r2 = sphere.radius * sphere.radius;
    int behind = p < 0;
    int outside = l2 > r2;

    if (behind && outside) {
        return (struct intersection) {
            .intersected = 0
        };
    }

    double m2 = l2 - p * p;
    int miss = m2 > r2;

    if (miss) {
        return (struct intersection) {
            .intersected = 0
        };
    }

    double q = sqrt(r2 - m2);
    double t = outside ? p - q : p + q;

    return (struct intersection) {
        .intersected = 1,
        .distance = t,
        .point = addv(ray.origin, mulv(ray.direction, t))
    };
}

static struct scene_intersection ray_scene_intersection(struct ray ray,
                                                        struct scene scene)
{
    struct scene_intersection nearest = {0};
    nearest.intersection.distance = INFINITY;

    struct object *it;
    for (it = scene.objects_first; it != scene.objects_last; ++it) {
        struct intersection intx =
            ray_sphere_intersection(ray, it->shape);

        if (!intx.intersected)
            continue;

        if (intx.distance < nearest.intersection.distance) {
            nearest.intersection = intx;
            nearest.object = *it;
        }
    }

    return nearest;
}

static int is_light_blocked(struct scene scene,
                            struct vector point,
                            struct light *light)
{
    double light_distance = norm(subv(light->origin, point));
    struct vector light_vector = normalize(subv(light->origin, point));
    struct ray light_ray = {point, light_vector};

    struct intersection intx =
        ray_scene_intersection(light_ray, scene).intersection;

    return intx.intersected && intx.distance < light_distance;
}

static struct vector ray_tracing(struct ray ray, struct scene scene, int depth)
{
    struct scene_intersection intx = ray_scene_intersection(ray, scene);
    if (depth < 0 || !intx.intersection.intersected) {
        return linear_interpolation(scene.background, fabs(ray.direction.y));
    }

    struct vector view = mulv(ray.direction, -1.0);
    struct vector n = sphere_normal(intx.object.shape, intx.intersection.point);
    struct vector point = budge(intx.intersection.point, n);
    struct material *mat = &intx.object.material;

    struct vector illum = {0.0, 0.0, 0.0};
    for (struct light *it = scene.lights_first; it != scene.lights_last; ++it) {
        if (is_light_blocked(scene, point, it))
            continue;

        struct vector l = normalize(subv(it->origin, point));
        struct vector il = blinn_phong(l, it->intensity, view, n, *mat);
        illum = addv(illum, il);
    }

    struct ray reflect_ray = {point, reflect(view, n)};
    struct vector reflect_illum = ray_tracing(reflect_ray, scene, --depth);

    return addv(illum, hadp(reflect_illum, mat->reflection));
}

static struct vector screen_coordinates(struct vector sub_pixel, int w, int h)
{
    double aspect_ratio = (double) h / w;
    return (struct vector) {
        .x =  (2.0 * sub_pixel.x / w - 1.0) * tan(FOV / 2.0),
        .y = -(2.0 * sub_pixel.y / h - 1.0) * tan(FOV / 2.0) * aspect_ratio,
        .z = 1.0
    };
}

static struct color render_pixel(struct scene scene,
                                 struct vector aa_grid[static AA_GRID_SIZE],
                                 int x,
                                 int y,
                                 int w,
                                 int h)
{
    struct vector illum = {0.0, 0.0, 0.0};

    /* Supersampling anti-aliasing */
    for (int i = 0; i < AA_GRID_SIZE; ++i) {
        struct vector sub_pixel = {x + aa_grid[i].x, y + aa_grid[i].y, 0.0};

        /* Where the ray hits the screen */
        struct vector s = screen_coordinates(sub_pixel, w, h);

        struct ray r = {scene.camera, normalize(s)};
        struct vector il = ray_tracing(r, scene, REFLECTION_DEPTH);
        illum = addv(illum, il);
    }

    illum = divv(illum, AA_GRID_SIZE);

    /* Gamma correction (2) using square root */
    return (struct color) {
        .r = 255.0 * sqrt(fmin(illum.x, 1.0)),
        .g = 255.0 * sqrt(fmin(illum.y, 1.0)),
        .b = 255.0 * sqrt(fmin(illum.z, 1.0))
    };
}

static void init_anti_aliasing_grid(struct vector grid[static AA_GRID_SIZE])
{
    double dx = 1.0 / (1.0 + AA_GRID_WIDTH);
    double dy = 1.0 / (1.0 + AA_GRID_HEIGHT);

    for (int i = 0; i < AA_GRID_SIZE; ++i) {
        grid[i] = (struct vector) {
            dx + dx * (i % AA_GRID_WIDTH),
            dy + dy * (i / AA_GRID_WIDTH),
            0.0
        };
    }
}

static void render(struct scene scene,
                   struct vector aa_grid[static AA_GRID_SIZE],
                   struct color fb[],
                   int w,
                   int h)
{
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            int i = x + y * w;
            fb[i] = render_pixel(scene, aa_grid, x, y, w, h);
        }
    }
}

static void ppm_write(struct color fb[], int w, int h, FILE *out)
{
    fprintf(out, "P3\n%d %d\n255\n", w, h);
    for (int i = 0; i < w * h; ++i)
        fprintf(out, "%d %d %d\n", fb[i].r, fb[i].g, fb[i].b);
    fflush(out);
}

int main()
{
    struct color *framebuffer = calloc(WIDTH * HEIGHT, sizeof(struct color));
    if (!framebuffer) {
        fprintf(stderr, ERROR_FRAMEBUFFER_MEM_ALLOCATION, WIDTH, HEIGHT);
        return EXIT_FAILURE;
    }

    struct vector aa_grid[AA_GRID_SIZE];
    init_anti_aliasing_grid(aa_grid);
    render(scene, aa_grid, framebuffer, WIDTH, HEIGHT);
    ppm_write(framebuffer, WIDTH, HEIGHT, stdout);

    free(framebuffer);
    return EXIT_SUCCESS;
}
