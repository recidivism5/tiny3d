#include <tinymath.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>

int modulo(int i, int m){
	return (i % m + m) % m;
}

float deg_to_rad(float d){
	return d*(float)M_PI/180.0f;
}

float rad_to_deg(float r){
	return r*180.0f/(float)M_PI;
}

void ivec_add(int *a, int *b, int *o, int n){
	for (int i = 0; i < n; i++){
		o[i] = a[i] + b[i];
	}
}

void ivec_sub(int *a, int *b, int *o, int n){
	for (int i = 0; i < n; i++){
		o[i] = a[i] - b[i];
	}
}

int ivec_manhattan(int *a, int *b, int n){
	int s = 0;
	for (int i = 0; i < n; i++){
		s += abs(a[i]-b[i]);
	}
	return s;
}

void fvec_add(float *a, float *b, float *o, int n){
	for (int i = 0; i < n; i++){
		o[i] = a[i] + b[i];
	}
}

void fvec_sub(float *a, float *b, float *o, int n){
	for (int i = 0; i < n; i++){
		o[i] = a[i] - b[i];
	}
}

void fvec_negate(float *a, float *o, int n){
	for (int i = 0; i < n; i++){
		o[i] = -a[i];
	}
}

void fvec_mul(float *a, float *b, float *o, int n){
	for (int i = 0; i < n; i++){
		o[i] = a[i] * b[i];
	}
}

void fvec_div(float *a, float *b, float *o, int n){
	for (int i = 0; i < n; i++){
		o[i] = a[i] / b[i];
	}
}

void fvec_divs(float *a, float s, float *o, int n){
	s = 1.0f / s;
	for (int i = 0; i < n; i++){
		o[i] = a[i] * s;
	}
}

void fvec_scale(float *a, float t, float *o, int n){
	for (int i = 0; i < n; i++){
		o[i] = a[i] * t;
	}
}

void fvec_lerp(float *a, float *b, float t, float *o, int n){
	for (int i = 0; i < n; i++){
		o[i] = a[i] + (b[i] - a[i]) * t;
	}
}

float fvec_dot(float *a, float *b, int n){
	float s = 0.0f;
	for (int i = 0; i < n; i++){
		s += a[i] * b[i];
	}
	return s;
}

float fvec_length(float *a, int n){
	return sqrtf(fvec_dot(a,a,n));
}

void fvec_set_length(float *a, float l, float *o, int n){
	fvec_scale(a,l/fvec_length(a,n),o,n);
}

void fvec_clamp_length(float *a, float l, float *o, int n){
	float len = fvec_length(a,n);
	if (l < len){
		fvec_scale(a,l/len,o,n);
	}
}

void fvec_normalize(float *a, float *o, int n){
	fvec_scale(a,1.0f/fvec_length(a,n),o,n);
}

float fvec_distance(float *a, float *b, int n){
	float s = 0.0f;
	for (int i = 0; i < n; i++){
		float d = a[i] - b[i];
		s += d * d;
	}
	return sqrtf(s);
}

void fvec_midpoint(float *a, float *b, float *o, int n){
	fvec_sub(b,a,o,n);
	fvec_scale(o,0.5f,o,n);
	fvec_add(a,o,o,n);
}

float fvec2_det(float *a, float *b){
	return a[0]*b[1] - a[1]*b[0];
}

void fvec3_cross(float *a, float *b, float *dst){
	dst[0] = a[1]*b[2] - a[2]*b[1];
	dst[1] = a[2]*b[0] - a[0]*b[2];
	dst[2] = a[0]*b[1] - a[1]*b[0];
}

void fvec3_rotate(float *v, float *a, float r, float *o){
	float m[3*3];
	mat3_rotate(m,a,r);
	mat_vec_mul(m,a,o,3);
}

bool fvec_equal(float *a, float *b, int n){
	for (int i = 0; i < n; i++){
		if (a[i] != b[i]){
			return false;
		}
	}
	return true;
}

void quat_identity(float *q){
	q[0] = 0;
	q[1] = 0;
	q[2] = 0;
	q[3] = 1;
}

void quat_slerp(float *a, float *b, float t, float *o){
	float cosOmega = vec4_dot(a,b);
	bool flip = false;
	if (cosOmega < 0){
		flip = true;
		cosOmega = -cosOmega;
	}
	float s1,s2;
	if (cosOmega > (1 - 1e-6f)){
		s1 = 1 - t;
		s2 = flip ? -t : t;
	} else {
		float omega = acosf(cosOmega);
		float invSinOmega = 1 / sinf(omega);
		s1 = sinf((1-t)*omega)*invSinOmega;
		float sm = sinf(t*omega)*invSinOmega;
		s2 = flip ? -sm : sm;
	}
	for (int i = 0; i < 4; i++){
		o[i] = s1*a[i] + s2*b[i];
	}
}

void quat_to_mat4(float *q, float *o){
    float xx = q[0] * q[0];
    float yy = q[1] * q[1];
    float zz = q[2] * q[2];
    float xy = q[0] * q[1];
    float xz = q[0] * q[2];
    float yz = q[1] * q[2];
    float wx = q[3] * q[0];
    float wy = q[3] * q[1];
    float wz = q[3] * q[2];
    o[0]  = 1.0f - 2.0f * (yy + zz);
    o[1]  = 2.0f * (xy - wz);
    o[2]  = 2.0f * (xz + wy);
    o[4]  = 2.0f * (xy + wz);
    o[5]  = 1.0f - 2.0f * (xx + zz);
    o[6]  = 2.0f * (yz - wx);
    o[8]  = 2.0f * (xz - wy);
    o[9]  = 2.0f * (yz + wx);
    o[10] = 1.0f - 2.0f * (xx + yy);
	o[3] = o[7] = o[11] = o[12] = o[13] = o[14] = 0.0f;
    o[15] = 1.0f;
}

void mat_mul(float *a, float *b, float *o, int n){
    for (int i = 0; i < n; ++i){
        for (int j = 0; j < n; ++j){
            float s = 0.0f;
            for (int k = 0; k < n; ++k){
                s += a[i * n + k] * b[k * n + j];
            }
            o[i * n + j] = s;
        }
    }
}

void mat_vec_mul(float *m, float *v, float *o, int n){
    for (int i = 0; i < n; ++i){
        float sum = 0.0f;
        for (int j = 0; j < n; ++j){
            sum += m[i * n + j] * v[j];
        }
        o[i] = sum;
    }
}

void mat_identity(float *m, int n){
    for (int i = 0; i < n; ++i){
        for (int j = 0; j < n; ++j){
            m[i * n + j] = (i == j) ? 1.0f : 0.0f;
        }
    }
}

void mat4_translate(float *m, float x, float y, float z){
	mat_identity(m,4);
	m[12] = x;
	m[13] = y;
	m[14] = z;
}

void mat3_rotate(float *m, float *a, float r){
	fvec_normalize(a,a,3);
    float c = cos(r);
    float s = sin(r);
    float t = 1.0f - c;
    float x = a[0], y = a[1], z = a[2];
    float tx = t * x, ty = t * y;
    float tz = t * z, sx = s * x, sy = s * y, sz = s * z;
    m[0] = tx * x + c;    m[3] = tx * y + sz;   m[6] = tx * z - sy;
    m[1] = tx * y - sz;   m[4] = ty * y + c;    m[7] = ty * z + sx;
    m[2] = tx * z + sy;   m[5] = ty * z - sx;   m[8] = tz * z + c;
}

void mat4_rotate(float *m, float *a, float r){
	fvec_normalize(a,a,3);
    float c = cos(r);
    float s = sin(r);
    float t = 1.0f - c;
    float x = a[0], y = a[1], z = a[2];
    float tx = t * x, ty = t * y;
    float tz = t * z, sx = s * x, sy = s * y, sz = s * z;
    m[0]  = tx * x + c;   m[4] = tx * y + sz;  m[8]  = tx * z - sy; m[12] = 0.0f;
    m[1]  = tx * y - sz;  m[5] = ty * y + c;   m[9]  = ty * z + sx; m[13] = 0.0f;
    m[2]  = tx * z + sy;  m[6] = ty * z - sx;  m[10] = tz * z + c;  m[14] = 0.0f;
    m[3]  = 0.0f;         m[7] = 0.0f;         m[11] = 0.0f;        m[15] = 1.0f;
}

void mat4_ortho_rh_zo(float *m, float left, float right, float bottom, float top, float near, float far){
    float rl = 1.0f / (right - left);
    float tb = 1.0f / (top - bottom);
    float fn = 1.0f / (far - near);
    // Fill the matrix in column-major order
    memset(m, 0, 16 * sizeof(float)); // Initialize all elements to 0
    m[0] = 2.0f * rl;          // Scale X
    m[5] = 2.0f * tb;          // Scale Y
    m[10] = fn;                // Scale Z (note: z-depth range 0-1 for OpenGL)
    m[12] = -(right + left) * rl;  // Translation X
    m[13] = -(top + bottom) * tb;  // Translation Y
    m[14] = -near * fn;        // Translation Z
    m[15] = 1.0f;              // Homogeneous coordinate
}

void mat4_persp_rh_zo(float *m, float fovy, float aspect, float near, float far){
    float f = 1.0f / tan(fovy / 2.0f);
    float nf = 1.0f / (near - far);
    // Fill the matrix in column-major order
    memset(m, 0, 16 * sizeof(float)); // Initialize all elements to 0
    m[0] = f / aspect;  // Scale X
    m[5] = f;           // Scale Y
    m[10] = far * nf;   // Scale Z
    m[11] = -1.0f;      // Set for perspective divide
    m[14] = near * far * nf; // Translation Z
    // m[15] is already 0 due to memset, so no need to set explicitly
}