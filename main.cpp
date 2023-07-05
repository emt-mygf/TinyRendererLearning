#include <vector>
#include <cmath>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include <iostream>
const TGAColor white = TGAColor{255, 255, 255, 255};
const TGAColor red   = TGAColor{0, 0, 255, 255};

Model* model = nullptr;
const int width  = 800;
const int height = 800;

void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color)
{
    bool bFlip = false; 
    if(std::abs(x0 - x1)<std::abs(y0 - y1)) 
    {
        std::swap(x0, y0); 
        std::swap(x1, y1); 
        bFlip = true; 
    } 
    
    if(x0>x1) 
    {
        std::swap(x0, x1); 
        std::swap(y0, y1); 
    }

    int Dy = std::abs(y1 - y0) >> 1;
    int Dx = std::abs(x1 - x0) >> 1;
    int Delta = 0;

    int y = y0;
    for(int x = x0; x <= x1; ++x) 
    { 
        if(bFlip) image.set(y, x, color);
        else image.set(x, y, color);

        Delta += Dy;
        if(Delta > Dx)
        {
            if(y1 > y0) ++y;
            else --y;

            Delta -= Dx;
        }
    } 
}

bool bInTriangle(Vec2i *pts, Vec2i point)
{
    // 判断点在三角形内
    Vec3i vec1{pts[1].x - pts[0].x, pts[2].x - pts[0].x, point.x - pts[0].x};
    Vec3i vec2{pts[1].y - pts[0].y, pts[2].y - pts[0].y, point.y - pts[0].y};

    Vec3i test = vec1 ^ vec2;
    if(test.z > 0) test = test * (-1);
    
    return (test.x >= 0) && (test.x + test.z <= 0) && (test.y >= 0) && (test.y + test.z <= 0) && (test.x + test.y + test.z <= 0);
}

void triangle(Vec2i *pts, TGAImage &image, TGAColor color)
{
    // 划定BoundingBox
    int minX = INT_MAX;
    int minY = INT_MAX;
    int maxX = INT_MIN;
    int maxY = INT_MIN;
    for(int i = 0; i < 3; ++i)
    {
        minX = std::min(minX, pts[i].x);
        minY = std::min(minY, pts[i].y);
        maxX = std::max(maxX, pts[i].x);
        maxY = std::max(maxY, pts[i].y);
    }

    for(int u = minX; u <= maxX; ++u)
    {
        for(int v = minY; v <= maxY; ++v)
        {
            Vec2i point{u, v};
            if(bInTriangle(pts, point))
            {
                image.set(u, v, color);
            }
        }
    }
}

int main(int argc, char** argv) {
    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("../obj/african_head.obj");
    }

    TGAImage image(width, height, TGAImage::RGB);
    
    Vec3f light_dir(0,0,-1);
    for (int i=0; i<model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        Vec2i screenVerts[3];
        Vec3f verts[3];
        for (int j=0; j<3; j++) {
            Vec3f v = model->vert(face[j]);
            verts[j] = v;
            // 投影到屏幕坐标系
            int x = (v.x+1.)*width/2.;
            int y = (-v.y+1.)*height/2.;
            screenVerts[j] = Vec2i{x, y};
        }
        // 世界坐标系：法线，求灰度
        Vec3f n = (verts[2] - verts[0]) ^ (verts[1] - verts[0]);
        n.normalize();
        float intensity = n * light_dir;
        uint8_t gray = intensity * 255;
        if(intensity > 0)
        {
            triangle(screenVerts, image, TGAColor{gray, gray, gray, 255});
        }
    }

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output.tga");
    delete model;
    return 0;
}