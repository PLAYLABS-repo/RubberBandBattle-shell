#pragma once
// =============================================================
// Mat4.h — 4x4 column-major float matrix
// Layout matches OpenGL: m[col*4 + row]
// =============================================================

#include "Vec3.h"
#include <cmath>
#include <cstring>

struct Vec4
{
    float x, y, z, w;
    Vec4() : x(0),y(0),z(0),w(0) {}
    Vec4(float x,float y,float z,float w) : x(x),y(y),z(z),w(w) {}
    const float* ptr() const { return &x; }
};

struct Mat4
{
    float m[16];   // column-major: m[col*4 + row]

    // ── Construction ─────────────────────────────────────────
    Mat4() { identity(); }

    // ── In-place identity ────────────────────────────────────
    void identity()
    {
        memset(m, 0, sizeof(m));
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    // ── Element access (row, col) ────────────────────────────
    float& operator()(int row, int col)       { return m[col*4+row]; }
    float  operator()(int row, int col) const { return m[col*4+row]; }

    // ── Multiplication ───────────────────────────────────────
    Mat4 operator*(const Mat4& b) const
    {
        Mat4 out;
        for (int col = 0; col < 4; ++col)
            for (int row = 0; row < 4; ++row)
            {
                float sum = 0;
                for (int k = 0; k < 4; ++k)
                    sum += m[k*4+row] * b.m[col*4+k];
                out.m[col*4+row] = sum;
            }
        return out;
    }
    Mat4& operator*=(const Mat4& b) { *this = *this * b; return *this; }

    // ── Transform a Vec4 ─────────────────────────────────────
    Vec4 operator*(const Vec4& v) const
    {
        return {
            m[0]*v.x + m[4]*v.y + m[ 8]*v.z + m[12]*v.w,
            m[1]*v.x + m[5]*v.y + m[ 9]*v.z + m[13]*v.w,
            m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14]*v.w,
            m[3]*v.x + m[7]*v.y + m[11]*v.z + m[15]*v.w
        };
    }

    /// Transform a position (w=1).
    Vec3 transformPoint(const Vec3& v) const
    {
        Vec4 r = *this * Vec4{v.x, v.y, v.z, 1.0f};
        if (fabsf(r.w) > 1e-8f) { r.x/=r.w; r.y/=r.w; r.z/=r.w; }
        return {r.x, r.y, r.z};
    }

    /// Transform a direction (w=0, ignores translation).
    Vec3 transformDir(const Vec3& v) const
    {
        return {
            m[0]*v.x + m[4]*v.y + m[ 8]*v.z,
            m[1]*v.x + m[5]*v.y + m[ 9]*v.z,
            m[2]*v.x + m[6]*v.y + m[10]*v.z
        };
    }

    // ── Transpose ────────────────────────────────────────────
    Mat4 transposed() const
    {
        Mat4 t;
        for (int c=0;c<4;++c)
            for (int r=0;r<4;++r)
                t.m[r*4+c] = m[c*4+r];
        return t;
    }

    // ── Inverse (general cofactor method) ────────────────────
    Mat4 inversed() const
    {
        float inv[16];
        const float* s = m;

        inv[ 0] =  s[5]*s[10]*s[15]-s[5]*s[11]*s[14]-s[9]*s[6]*s[15]+s[9]*s[7]*s[14]+s[13]*s[6]*s[11]-s[13]*s[7]*s[10];
        inv[ 4] = -s[4]*s[10]*s[15]+s[4]*s[11]*s[14]+s[8]*s[6]*s[15]-s[8]*s[7]*s[14]-s[12]*s[6]*s[11]+s[12]*s[7]*s[10];
        inv[ 8] =  s[4]*s[ 9]*s[15]-s[4]*s[11]*s[13]-s[8]*s[5]*s[15]+s[8]*s[7]*s[13]+s[12]*s[5]*s[11]-s[12]*s[7]*s[ 9];
        inv[12] = -s[4]*s[ 9]*s[14]+s[4]*s[10]*s[13]+s[8]*s[5]*s[14]-s[8]*s[6]*s[13]-s[12]*s[5]*s[10]+s[12]*s[6]*s[ 9];
        inv[ 1] = -s[1]*s[10]*s[15]+s[1]*s[11]*s[14]+s[9]*s[2]*s[15]-s[9]*s[3]*s[14]-s[13]*s[2]*s[11]+s[13]*s[3]*s[10];
        inv[ 5] =  s[0]*s[10]*s[15]-s[0]*s[11]*s[14]-s[8]*s[2]*s[15]+s[8]*s[3]*s[14]+s[12]*s[2]*s[11]-s[12]*s[3]*s[10];
        inv[ 9] = -s[0]*s[ 9]*s[15]+s[0]*s[11]*s[13]+s[8]*s[1]*s[15]-s[8]*s[3]*s[13]-s[12]*s[1]*s[11]+s[12]*s[3]*s[ 9];
        inv[13] =  s[0]*s[ 9]*s[14]-s[0]*s[10]*s[13]-s[8]*s[1]*s[14]+s[8]*s[2]*s[13]+s[12]*s[1]*s[10]-s[12]*s[2]*s[ 9];
        inv[ 2] =  s[1]*s[ 6]*s[15]-s[1]*s[ 7]*s[14]-s[5]*s[2]*s[15]+s[5]*s[3]*s[14]+s[13]*s[2]*s[ 7]-s[13]*s[3]*s[ 6];
        inv[ 6] = -s[0]*s[ 6]*s[15]+s[0]*s[ 7]*s[14]+s[4]*s[2]*s[15]-s[4]*s[3]*s[14]-s[12]*s[2]*s[ 7]+s[12]*s[3]*s[ 6];
        inv[10] =  s[0]*s[ 5]*s[15]-s[0]*s[ 7]*s[13]-s[4]*s[1]*s[15]+s[4]*s[3]*s[13]+s[12]*s[1]*s[ 7]-s[12]*s[3]*s[ 5];
        inv[14] = -s[0]*s[ 5]*s[14]+s[0]*s[ 6]*s[13]+s[4]*s[1]*s[14]-s[4]*s[2]*s[13]-s[12]*s[1]*s[ 6]+s[12]*s[2]*s[ 5];
        inv[ 3] = -s[1]*s[ 6]*s[11]+s[1]*s[ 7]*s[10]+s[5]*s[2]*s[11]-s[5]*s[3]*s[10]-s[ 9]*s[2]*s[ 7]+s[ 9]*s[3]*s[ 6];
        inv[ 7] =  s[0]*s[ 6]*s[11]-s[0]*s[ 7]*s[10]-s[4]*s[2]*s[11]+s[4]*s[3]*s[10]+s[ 8]*s[2]*s[ 7]-s[ 8]*s[3]*s[ 6];
        inv[11] = -s[0]*s[ 5]*s[11]+s[0]*s[ 7]*s[ 9]+s[4]*s[1]*s[11]-s[4]*s[3]*s[ 9]-s[ 8]*s[1]*s[ 7]+s[ 8]*s[3]*s[ 5];
        inv[15] =  s[0]*s[ 5]*s[10]-s[0]*s[ 6]*s[ 9]-s[4]*s[1]*s[10]+s[4]*s[2]*s[ 9]+s[ 8]*s[1]*s[ 6]-s[ 8]*s[2]*s[ 5];

        float det = s[0]*inv[0]+s[1]*inv[4]+s[2]*inv[8]+s[3]*inv[12];
        Mat4 result;
        if (fabsf(det) < 1e-10f) return result;
        float invDet = 1.0f / det;
        for (int i=0;i<16;++i) result.m[i] = inv[i]*invDet;
        return result;
    }

    // ── Raw pointer (glLoadMatrixf / glUniformMatrix4fv) ─────
    const float* ptr() const { return m; }
          float* ptr()       { return m; }

    // ── Factory methods ──────────────────────────────────────

    static Mat4 Identity()
    {
        Mat4 r; return r;
    }

    static Mat4 Translation(float tx, float ty, float tz)
    {
        Mat4 r;
        r.m[12]=tx; r.m[13]=ty; r.m[14]=tz;
        return r;
    }
    static Mat4 Translation(const Vec3& t)
    { return Translation(t.x, t.y, t.z); }

    static Mat4 Scale(float sx, float sy, float sz)
    {
        Mat4 r;
        r.m[0]=sx; r.m[5]=sy; r.m[10]=sz;
        return r;
    }
    static Mat4 Scale(const Vec3& s) { return Scale(s.x,s.y,s.z); }
    static Mat4 Scale(float s)       { return Scale(s,s,s); }

    /// Rotation around an arbitrary axis (normalises axis internally).
    static Mat4 Rotation(float ax, float ay, float az, float angleRad)
    {
        float len = sqrtf(ax*ax+ay*ay+az*az);
        if (len < 1e-8f) return Identity();
        ax/=len; ay/=len; az/=len;
        float c=cosf(angleRad), s=sinf(angleRad), t=1-c;
        Mat4 r;
        r.m[ 0]=t*ax*ax+c;    r.m[ 4]=t*ax*ay-s*az; r.m[ 8]=t*ax*az+s*ay;
        r.m[ 1]=t*ax*ay+s*az; r.m[ 5]=t*ay*ay+c;    r.m[ 9]=t*ay*az-s*ax;
        r.m[ 2]=t*ax*az-s*ay; r.m[ 6]=t*ay*az+s*ax; r.m[10]=t*az*az+c;
        r.m[3]=r.m[7]=r.m[11]=r.m[12]=r.m[13]=r.m[14]=0; r.m[15]=1;
        return r;
    }
    static Mat4 Rotation(const Vec3& axis, float angleRad)
    { return Rotation(axis.x,axis.y,axis.z,angleRad); }

    static Mat4 RotationX(float rad)
    {
        float c=cosf(rad),s=sinf(rad); Mat4 r;
        r.m[5]=c; r.m[9]=-s; r.m[6]=s; r.m[10]=c; return r;
    }
    static Mat4 RotationY(float rad)
    {
        float c=cosf(rad),s=sinf(rad); Mat4 r;
        r.m[0]=c; r.m[8]=s; r.m[2]=-s; r.m[10]=c; return r;
    }
    static Mat4 RotationZ(float rad)
    {
        float c=cosf(rad),s=sinf(rad); Mat4 r;
        r.m[0]=c; r.m[4]=-s; r.m[1]=s; r.m[5]=c; return r;
    }

    static Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& worldUp)
    {
        Vec3 f = (eye - target).normalized();
        Vec3 r = worldUp.cross(f).normalized();
        Vec3 u = f.cross(r);
        Mat4 mat;
        mat.m[ 0]=r.x; mat.m[ 4]=r.y; mat.m[ 8]=r.z;
        mat.m[ 1]=u.x; mat.m[ 5]=u.y; mat.m[ 9]=u.z;
        mat.m[ 2]=f.x; mat.m[ 6]=f.y; mat.m[10]=f.z;
        mat.m[12]=-r.dot(eye);
        mat.m[13]=-u.dot(eye);
        mat.m[14]=-f.dot(eye);
        return mat;
    }

    static Mat4 Perspective(float fovYRad, float aspect, float nearZ, float farZ)
    {
        float f = 1.0f / tanf(fovYRad * 0.5f);
        Mat4 r; memset(r.m,0,sizeof(r.m));
        r.m[ 0]=f/aspect; r.m[5]=f;
        r.m[10]=(farZ+nearZ)/(nearZ-farZ);
        r.m[11]=-1.0f;
        r.m[14]=(2.0f*farZ*nearZ)/(nearZ-farZ);
        return r;
    }

    static Mat4 Ortho(float left, float right,
                      float bottom, float top,
                      float nearZ, float farZ)
    {
        Mat4 r;
        r.m[ 0]=2.0f/(right-left);
        r.m[ 5]=2.0f/(top-bottom);
        r.m[10]=-2.0f/(farZ-nearZ);
        r.m[12]=-(right+left)/(right-left);
        r.m[13]=-(top+bottom)/(top-bottom);
        r.m[14]=-(farZ+nearZ)/(farZ-nearZ);
        return r;
    }

    /// Compose T * R * S in one call.
    static Mat4 TRS(const Vec3& t, const Vec3& axis, float angleRad, const Vec3& s)
    {
        return Translation(t) * Rotation(axis,angleRad) * Scale(s);
    }
};
