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

int main(int argc, char** argv) {
    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("../obj/african_head.obj");
    }

    TGAImage image(width, height, TGAImage::RGB);
    // std::cout<<model->nfaces()<<std::endl;
    for (int i=0; i<model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        for (int j=0; j<3; j++) {
            Vec3f v0 = model->vert(face[j]);
            Vec3f v1 = model->vert(face[(j+1)%3]);
            int x0 = (v0.x+1.)*width/2.;
            int y0 = (-v0.y+1.)*height/2.;
            int x1 = (v1.x+1.)*width/2.;
            int y1 = (-v1.y+1.)*height/2.;
            // std::cout<<x0<<", "<<x1<<", "<<y0<<", "<<y1<<std::endl;
            line(x0, y0, x1, y1, image, white);
        }
    }

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output.tga");
    delete model;
    return 0;
}