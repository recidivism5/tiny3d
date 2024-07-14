#include <stdbool.h>

int modulo(int i, int m);
float deg_to_rad(float d);
float rad_to_deg(float r);

void ivec_add(int *a, int *b, int *o, int n);
void ivec_sub(int *a, int *b, int *o, int n);
int ivec_manhattan(int *a, int *b, int n);

void fvec_add(float *a, float *b, float *o, int n);
void fvec_sub(float *a, float *b, float *o, int n);
void fvec_negate(float *a, float *o, int n);
void fvec_mul(float *a, float *b, float *o, int n);
void fvec_div(float *a, float *b, float *o, int n);
void fvec_divs(float *a, float s, float *o, int n);
void fvec_scale(float *a, float t, float *o, int n);
void fvec_lerp(float *a, float *b, float t, float *o, int n);
float fvec_dot(float *a, float *b, int n);
float fvec_length(float *a, int n);
void fvec_set_length(float *a, float l, float *o, int n);
void fvec_clamp_length(float *a, float l, float *o, int n);
void fvec_normalize(float *a, float *o, int n);
float fvec_distance(float *a, float *b, int n);
void fvec_midpoint(float *a, float *b, float *o, int n);
float fvec2_det(float *a, float *b);
void fvec3_cross(float *a, float *b, float *dst);
void fvec3_rotate(float *v, float *a, float r, float *o);
bool fvec_equal(float *a, float *b, int n);

void quat_identity(float *q);
void quat_slerp(float *a, float *b, float t, float *o);
void quat_to_mat4(float *q, float *o);

void mat_mul(float *a, float *b, float *o, int n);
void mat_vec_mul(float *m, float *v, float *o, int n);
void mat_identity(float *m, int n);
void mat4_translate(float *m, float x, float y, float z);
void mat3_rotate(float *m, float *a, float r);
void mat4_rotate(float *m, float *a, float r);
void mat4_ortho_rh_zo(float *m, float left, float right, float bottom, float top, float near, float far);
void mat4_persp_rh_zo(float *m, float fovy, float aspect, float near, float far);