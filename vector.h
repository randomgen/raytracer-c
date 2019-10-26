#ifndef VECTOR_H_INCLUDED
#define VECTOR_H_INCLUDED

struct vector {
    double x;
    double y;
    double z;
};

/* Dot product of vectors `a` and `b` */
double dotp(struct vector a, struct vector b);

/* Vector addition of `a` and `b` */
struct vector addv(struct vector a, struct vector b);

/* Vector subtraction of `a` by `b` */
struct vector subv(struct vector a, struct vector b);

/* Hadamard product (element-wise multiplication) of vectors `a` and `b` */
struct vector hadp(struct vector a, struct vector b);

/* Multiplication of vector `a` by a scalar `b` */
struct vector mulv(struct vector a, double b);

/* Division of vector `a` by a scalar `b` */
struct vector divv(struct vector a, double b);

/* Norm of vector `a` */
double norm(struct vector a);

/* Normalized form of vector `a` (norm = 1) */
struct vector normalize(struct vector a);

#endif
