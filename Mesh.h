
#pragma once
// =============================================================
// Mesh.h — GPU mesh (VAO / VBO / EBO)
// Vertex layout:  position(xyz)  normal(xyz)  uv(uv)
// Filled by Playlabs_LoadMesh(); drawn by Playlabs_DrawModel().
// =============================================================

#include <GL/gl.h>

// One interleaved vertex — matches attrib layout in PlaylabsGL.cpp
//   location 0 : position  (px, py, pz)
//   location 1 : normal    (nx, ny, nz)
//   location 2 : texcoord  (u,  v)
struct Vertex3D
{
    float px, py, pz;   // position
    float nx, ny, nz;   // normal
    float u,  v;        // UV
};

struct Mesh
{
    GLuint  vao        = 0;
    GLuint  vbo        = 0;
    GLuint  ebo        = 0;
    GLsizei indexCount = 0;
};
