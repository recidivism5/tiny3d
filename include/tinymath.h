#include <stdbool.h>

typedef int ivec2[2];
typedef int ivec3[3];
typedef int ivec4[4];

typedef float vec2[2];
typedef float vec3[3];
typedef float vec4[4];

typedef double dvec3[3];

typedef vec4 mat4[4];

void ivec2_copy(ivec2 src, ivec2 dst);
int ivec2_manhattan(ivec2 a, ivec2 b);

void vec2_copy(vec2 src, vec2 dst);
void vec2_add(vec2 a, vec2 b, vec2 dst);
void vec2_sub(vec2 a, vec2 b, vec2 dst);
void vec2_scale(vec2 a, float t, vec2 dst);
void vec2_lerp(vec2 a, vec2 b, float t, vec2 dst);
float vec2_dot(vec2 a, vec2 b);
float vec2_length(vec2 a);
void vec2_set_length(vec2 v, float l, vec2 dst);
void vec2_clamp_length(vec2 a, float l, vec2 dst);

bool vec3_equal(vec3 a, vec3 b);
void vec3_copy(vec3 src, vec3 dst);
void vec3_add(vec3 a, vec3 b, vec3 dst);
void vec3_sub(vec3 a, vec3 b, vec3 dst);
void vec3_negate(vec3 v, vec3 dst);
void vec3_scale(vec3 v, float s, vec3 dst);
void vec3_mul(vec3 a, vec3 b, vec3 dst);
void vec3_div(vec3 a, vec3 b, vec3 dst);
void vec3_divs(vec3, float s, vec3 dst);
float vec3_dot(vec3 a, vec3 b);
void vec3_cross(vec3 a, vec3 b, vec3 dst);
float vec3_length(vec3 v);
float vec3_distance(vec3 a, vec3 b);
void vec3_midpoint(vec3 a, vec3 b, vec3 dst);
void vec3_normalize(vec3 v, vec3 dst);
void vec3_set_length(vec3 v, float l, vec3 dst);
void vec3_lerp(vec3 a, vec3 b, float t, vec3 dst);

void euler_wrap(vec3 e, vec3 dst);

void vec4_copy(vec4 src, vec4 dst);
void vec4_add(vec4 a, vec4 b, vec4 dst);
void vec4_sub(vec4 a, vec4 b, vec4 dst);
void vec4_negate(vec4 v, vec4 dst);
void vec4_scale(vec4 v, float s, vec4 dst);
void vec4_mul(vec4 a, vec4 b, vec4 dst);
void vec4_div(vec4 a, vec4 b, vec4 dst);
void vec4_lerp(vec4 a, vec4 b, float t, vec4 dst);
float vec4_dot(vec4 a, vec4 b);
float vec4_length(vec4 v);

void quat_identity(vec4 q);
void quat_slerp(vec4 a, vec4 b, float t, vec4 dst);
void quat_to_mat4(vec4 q, mat4 dst);

void mat4_copy(mat4 src, mat4 dst);
void mat4_mul(mat4 a, mat4 b, mat4 dst);
void mat4_mul_vec4(mat4 m, vec4 v, vec4 dst);
void mat4_mul_vec3_pos(mat4 m, vec3 v, vec4 dst);
void mat4_mul_vec3_dir(mat4 m, vec3 v, vec3 dst);
void mat4_identity(mat4 m);
void mat4_scale_3(mat4 m, vec3 s);
void mat4_translate(mat4 m, vec3 t);
void mat4_rotate(mat4 m, vec3 a, float r);
void mat4_euler_yxz(mat4 m, vec3 e);
void mat4_rotate_x(mat4 m, float r);
void mat4_rotate_y(mat4 m, float r);
void mat4_rotate_z(mat4 m, float r);

void mat4_ortho_rh_no(mat4 m, float left, float right, float bottom, float top, float nearZ, float farZ);
void mat4_ortho_lh_zo(mat4 m, float left, float right, float bottom, float top, float nearZ, float farZ);
void mat4_persp_rh_no(mat4 m, float fovy, float aspect, float nearZ, float farZ);
void mat4_persp_lh_zo(mat4 m, float fovy, float aspect, float nearZ, float farZ);

void vec3_rotate_deg(vec3 in, vec3 axis, float angle, vec3 out);