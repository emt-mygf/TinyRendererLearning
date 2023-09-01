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
    mat<2, 3, float> varyingUV;
    mat<3, 3, float> varyingNorm;
    mat<4, 3, float> varyingTri;
    mat<3, 3, float> varyingTriNorm;
    Matrix MVP;
    Matrix MVP_INVERSE;

    virtual Vec4f vertexShader(int iface, int nthvert) 
    {
        Vec4f glVertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
        glVertex = Viewport * Projection * ModelView * glVertex;     // transform it to screen coordinates
        varyingIntensity[nthvert] = std::max(0.f, model->normal(iface, nthvert) * lightDir); // get diffuse lighting intensity
        varyingUV.set_col(nthvert, model->uv(iface, nthvert));
        varyingNorm.set_col(nthvert, proj<3>(MVP_INVERSE * embed<4>(model->normal(iface, nthvert), 0.f)));
        varyingTri.set_col(nthvert, glVertex);
        varyingTriNorm.set_col(nthvert, proj<3>(glVertex / glVertex[3]));
        return glVertex;
    }

    virtual bool fragmentShader(Vec3f bar, TGAColor& color) {
        float intensity = varyingIntensity * bar;   // interpolate intensity for the current pixel
        Vec2f uv = varyingUV * bar;

        //// ��θУ�ֻ��6��ֵ
        //if (intensity > .90) intensity = 1;
        //else if (intensity > .70) intensity = .80;
        //else if (intensity > .50) intensity = .60;
        //else if (intensity > .30) intensity = .40;
        //else if (intensity > .10) intensity = .20;
        //else intensity = 0;

        // Ӧ�÷�����ͼ��tangent������ͼ�������㷨�߲�ֵ�������ص㷨��->�������ص��ǿ����ȥΪֱ�Ӷ����ǿ��ֵ�������ص��ǿ��
        Vec3f normal = (varyingNorm * bar).normalize();
        mat<3, 3, float> A;
        A[0] = varyingTriNorm.col(1) - varyingTriNorm.col(0);
        A[1] = varyingTriNorm.col(2) - varyingTriNorm.col(0);
        A[2] = normal;
        mat<3, 3, float> AI = A.invert();
        Vec3f i = AI * Vec3f(varyingUV[0][1] - varyingUV[0][0], varyingUV[0][2] - varyingUV[0][0], 0);
        Vec3f j = AI * Vec3f(varyingUV[1][1] - varyingUV[1][0], varyingUV[1][2] - varyingUV[1][0], 0);
        mat<3, 3, float> B;
        B.set_col(0, i.normalize());
        B.set_col(1, j.normalize());
        B.set_col(2, normal);
        Vec3f newNormal = B * model->normal(uv);
        float newIntensity = std::max(0.f, newNormal * lightDir);

        // color = model->diffuse(uv) * intensity; 
        color = model->diffuse(uv) * newIntensity;
        return false;
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
    shader.MVP = Projection * ModelView;
    shader.MVP_INVERSE = (Projection * ModelView).invert_transpose();
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