#include "vector.h"

#include <math.h>

double dotp(struct vector a, struct vector b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

struct vector addv(struct vector a, struct vector b)
{
    return (struct vector) {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z
    };
}

struct vector subv(struct vector a, struct vector b)
{
    return (struct vector) {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z
    };
}

struct vector hadp(struct vector a, struct vector b)
{
    return (struct vector) {
        a.x * b.x,
        a.y * b.y,
        a.z * b.z
    };
}

struct vector mulv(struct vector a, double b)
{
    return (struct vector) {
        a.x * b,
        a.y * b,
        a.z * b
    };
}

struct vector divv(struct vector a, double b)
{
    return (struct vector) {
        a.x / b,
        a.y / b,
        a.z / b
    };
}

double norm(struct vector a)
{
    return sqrt(dotp(a, a));
}

struct vector normalize(struct vector a)
{
    return divv(a, norm(a));
}
