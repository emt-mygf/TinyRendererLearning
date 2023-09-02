#ifndef __MYGL_H__
#define __MYGL_H__

#include "tgaimage.h"
#include "geometry.h"

extern Matrix ModelView;
extern Matrix Viewport;
extern Matrix Projection;

void viewport(int x, int y, int w, int h);
void projection(float coeff = 0.f); // coeff = -1/c
void lookat(Vec3f eye, Vec3f center, Vec3f up);
const float depth = 2000.f;

struct ShaderBase {
    virtual ~ShaderBase();
    virtual Vec4f vertexShader(int iface, int nthvert) = 0;
    virtual bool fragmentShader(Vec3f bar, TGAColor& color) = 0;
};

void triangle(Vec4f* pts, ShaderBase& shader, TGAImage& image, TGAImage& zbuffer);

void triangleShadow(Vec4f* pts, ShaderBase& shader, TGAImage& zbuffer);

#endif //__MYGL_H__