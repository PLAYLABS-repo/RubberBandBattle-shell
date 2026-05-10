#pragma once
// =============================================================
// Vec3.h — 3-component float vector
// Plain-old-data; no virtuals, no heap allocation.
// Matches the column-major convention used by PlaylabsGL.
// =============================================================

#include <cmath>

struct Vec3
{
    float x, y, z;

    // ── Construction ─────────────────────────────────────────
    Vec3()                         : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z): x(x), y(y), z(z) {}
    explicit Vec3(float s)         : x(s), y(s), z(s) {}

    // ── Arithmetic (vector) ──────────────────────────────────
    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(const Vec3& o) const { return {x*o.x, y*o.y, z*o.z}; }
    Vec3 operator/(const Vec3& o) const { return {x/o.x, y/o.y, z/o.z}; }

    Vec3& operator+=(const Vec3& o) { x+=o.x; y+=o.y; z+=o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x-=o.x; y-=o.y; z-=o.z; return *this; }
    Vec3& operator*=(const Vec3& o) { x*=o.x; y*=o.y; z*=o.z; return *this; }
    Vec3& operator/=(const Vec3& o) { x/=o.x; y/=o.y; z/=o.z; return *this; }

    // ── Arithmetic (scalar) ──────────────────────────────────
    Vec3 operator+(float s) const { return {x+s, y+s, z+s}; }
    Vec3 operator-(float s) const { return {x-s, y-s, z-s}; }
    Vec3 operator*(float s) const { return {x*s, y*s, z*s}; }
    Vec3 operator/(float s) const { float r=1.f/s; return {x*r, y*r, z*r}; }

    Vec3& operator+=(float s) { x+=s; y+=s; z+=s; return *this; }
    Vec3& operator-=(float s) { x-=s; y-=s; z-=s; return *this; }
    Vec3& operator*=(float s) { x*=s; y*=s; z*=s; return *this; }
    Vec3& operator/=(float s) { float r=1.f/s; x*=r; y*=r; z*=r; return *this; }

    // ── Unary ────────────────────────────────────────────────
    Vec3 operator-() const { return {-x, -y, -z}; }

    // ── Comparison ───────────────────────────────────────────
    bool operator==(const Vec3& o) const { return x==o.x && y==o.y && z==o.z; }
    bool operator!=(const Vec3& o) const { return !(*this == o); }

    // ── Length ───────────────────────────────────────────────
    float lengthSq() const { return x*x + y*y + z*z; }
    float length()   const { return sqrtf(lengthSq()); }

    // ── Normalisation ────────────────────────────────────────
    Vec3 normalized() const
    {
        float len = length();
        if (len < 1e-8f) return {};
        float inv = 1.0f / len;
        return {x*inv, y*inv, z*inv};
    }

    void normalize()
    {
        float len = length();
        if (len < 1e-8f) return;
        float inv = 1.0f / len;
        x *= inv; y *= inv; z *= inv;
    }

    bool isNormalized(float epsilon = 1e-4f) const
    {
        return fabsf(lengthSq() - 1.0f) < epsilon;
    }

    // ── Dot product ──────────────────────────────────────────
    float dot(const Vec3& o) const { return x*o.x + y*o.y + z*o.z; }

    // ── Cross product ────────────────────────────────────────
    Vec3 cross(const Vec3& o) const
    {
        return {
            y*o.z - z*o.y,
            z*o.x - x*o.z,
            x*o.y - y*o.x
        };
    }

    // ── Distance ─────────────────────────────────────────────
    float distanceTo(const Vec3& o)   const { return (*this - o).length(); }
    float distanceSqTo(const Vec3& o) const { return (*this - o).lengthSq(); }

    // ── Interpolation ────────────────────────────────────────
    /// Linear interpolation:  a.lerp(b, 0) == a,  a.lerp(b, 1) == b
    Vec3 lerp(const Vec3& b, float t) const
    {
        return {x + (b.x-x)*t,
                y + (b.y-y)*t,
                z + (b.z-z)*t};
    }

    /// Spherical linear interpolation between two unit vectors.
    Vec3 slerp(const Vec3& b, float t) const
    {
        float cosA = dot(b);
        // Clamp to avoid acos domain errors
        if (cosA >  0.9999f) return lerp(b, t).normalized();
        if (cosA < -0.9999f)
        {
            // 180° apart — any perpendicular axis works
            Vec3 perp = fabsf(x) < 0.9f
                ? Vec3(0,0,1).cross(*this).normalized()
                : Vec3(0,1,0).cross(*this).normalized();
            return lerp(perp, t * 2.0f).normalized();
        }
        float angle  = acosf(cosA);
        float sinA   = sinf(angle);
        float factorA = sinf((1.0f - t) * angle) / sinA;
        float factorB = sinf(t           * angle) / sinA;
        return {factorA*x + factorB*b.x,
                factorA*y + factorB*b.y,
                factorA*z + factorB*b.z};
    }

    // ── Reflection ───────────────────────────────────────────
    /// Reflect this vector around a surface normal n (n must be normalised).
    Vec3 reflect(const Vec3& n) const
    {
        return *this - n * (2.0f * dot(n));
    }

    // ── Project / reject ─────────────────────────────────────
    /// Component of this vector along axis (axis must be normalised).
    Vec3 projectOnto(const Vec3& axis) const
    {
        return axis * dot(axis);
    }

    /// Component of this vector perpendicular to axis.
    Vec3 rejectFrom(const Vec3& axis) const
    {
        return *this - projectOnto(axis);
    }

    // ── Component-wise helpers ───────────────────────────────
    float  min()       const { return x<y ? (x<z?x:z) : (y<z?y:z); }
    float  max()       const { return x>y ? (x>z?x:z) : (y>z?y:z); }
    Vec3   abs()       const { return {fabsf(x), fabsf(y), fabsf(z)}; }
    Vec3   floor()     const { return {floorf(x), floorf(y), floorf(z)}; }
    Vec3   ceil()      const { return {ceilf(x),  ceilf(y),  ceilf(z)}; }
    Vec3   clamp(float lo, float hi) const
    {
        auto cl = [lo,hi](float v){ return v<lo?lo:(v>hi?hi:v); };
        return {cl(x), cl(y), cl(z)};
    }

    // ── Raw pointer access (for OpenGL uploads) ──────────────
    const float* ptr() const { return &x; }
          float* ptr()       { return &x; }

    // ── Common constants ─────────────────────────────────────
    static Vec3 zero()    { return {0,0,0}; }
    static Vec3 one()     { return {1,1,1}; }
    static Vec3 right()   { return {1,0,0}; }
    static Vec3 up()      { return {0,1,0}; }
    static Vec3 forward() { return {0,0,-1}; }  // right-handed, -Z forward
};

// ── Free-function scalar-left overloads ──────────────────────
inline Vec3 operator+(float s, const Vec3& v) { return v + s; }
inline Vec3 operator*(float s, const Vec3& v) { return v * s; }

// ── Free-function dot / cross (alternative call style) ───────
inline float dot  (const Vec3& a, const Vec3& b) { return a.dot(b); }
inline Vec3  cross(const Vec3& a, const Vec3& b) { return a.cross(b); }
inline float length   (const Vec3& v) { return v.length(); }
inline Vec3  normalize(const Vec3& v) { return v.normalized(); }
inline Vec3  lerp(const Vec3& a, const Vec3& b, float t) { return a.lerp(b,t); }
