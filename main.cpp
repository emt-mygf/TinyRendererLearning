#include <vector>
#include <iostream>

#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "mygl.h"

Model* model = nullptr;
const int width = 800;
const int height = 800;

Vec3f lightDir(1, 1, 1);
Vec3f eye(1, 0, 3);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

struct MyShader : public ShaderBase
{
    Vec3f varyingIntensity; // written by vertex shader, read by fragment shader

    virtual Vec4f vertexShader(int iface, int nthvert) 
    {
        Vec4f glVertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
        glVertex = Viewport * Projection * ModelView * glVertex;     // transform it to screen coordinates
        varyingIntensity[nthvert] = std::max(0.f, model->normal(iface, nthvert) * lightDir); // get diffuse lighting intensity
        return glVertex;
    }

    virtual bool fragmentShader(Vec3f bar, TGAColor& color) {
        float intensity = varyingIntensity * bar;   // interpolate intensity for the current pixel
        color = TGAColor(255, 255, 255) * intensity; // well duh
        return false;                              // no, we do not discard this pixel
    }
};

int main(int argc, char** argv) {
    if(2==argc) 
    {
        model = new Model(argv[1]);
    } 
    else 
    {
        model = new Model("../../../obj/african_head/african_head.obj");
    }

    lookat(eye, center, up);
    viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
    projection(-1.f / (eye - center).norm());
    lightDir.normalize();

    TGAImage image(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

    MyShader shader;
    for (int i = 0; i < model->nfaces(); i++) {
        Vec4f screenCoords[3];
        for (int j = 0; j < 3; j++) {
            screenCoords[j] = shader.vertexShader(i, j);
        }
        triangle(screenCoords, shader, image, zbuffer);
    }

    image.flip_vertically(); // to place the origin in the bottom left corner of the image
    zbuffer.flip_vertically();
    image.write_tga_file("output.tga");
    zbuffer.write_tga_file("zbuffer.tga");

    delete model;
    return 0;
}