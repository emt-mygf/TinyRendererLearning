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
Vec3f right(0, 0, 1);
TGAImage ShadowMap(width, height, TGAImage::GRAYSCALE);

struct MyShader : public ShaderBase
{
    Vec3f varyingIntensity; // written by vertex shader, read by fragment shader
    mat<2, 3, float> varyingUV;
    mat<3, 3, float> varyingNorm;
    mat<4, 3, float> varyingTri;
    mat<3, 3, float> varyingTriNormalized;
    Matrix MVP;
    Matrix MVP_INVERSE;
    Matrix M_SHADOW;

    virtual Vec4f vertexShader(int iface, int nthvert) 
    {
        Vec4f glVertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
        glVertex = Viewport * Projection * ModelView * glVertex;     // transform it to screen coordinates
        varyingIntensity[nthvert] = std::max(0.f, model->normal(iface, nthvert) * lightDir); // get diffuse lighting intensity
        varyingUV.set_col(nthvert, model->uv(iface, nthvert));
        varyingNorm.set_col(nthvert, model->normal(iface, nthvert));
        varyingTri.set_col(nthvert, glVertex);
        varyingTriNormalized.set_col(nthvert, proj<3>(glVertex / glVertex[3]));
        return glVertex;
    }

    virtual bool fragmentShader(Vec3f bar, TGAColor& color) {
        float intensity = varyingIntensity * bar;   // interpolate intensity for the current pixel
        Vec2f uv = varyingUV * bar;

        //// 层次感，只有6个值
        //if (intensity > .90) intensity = 1;
        //else if (intensity > .70) intensity = .80;
        //else if (intensity > .50) intensity = .60;
        //else if (intensity > .30) intensity = .40;
        //else if (intensity > .10) intensity = .20;
        //else intensity = 0;

        // 应用Shadow Mapping
        Vec4f ShadowMappingPoint = M_SHADOW * embed<4>(varyingTri * bar);
        ShadowMappingPoint = ShadowMappingPoint / ShadowMappingPoint[3];
        Vec2i ShadowMappingPosition = proj<2>(ShadowMappingPoint);
        float shadow = .3 + .7 * (ShadowMap.get(ShadowMappingPosition.x, ShadowMappingPosition.y)[0] < static_cast<int>(ShadowMappingPoint[2]));

        // 应用法线贴图（tangent法线贴图）（顶点法线插值计算像素点法线->计算像素点光强，过去为直接顶点光强插值计算像素点光强）
        Vec3f normal = (varyingNorm * bar).normalize();
        mat<3, 3, float> A;
        A[0] = varyingTriNormalized.col(1) - varyingTriNormalized.col(0);
        A[1] = varyingTriNormalized.col(2) - varyingTriNormalized.col(0);
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

        // specular
        Vec3f reflect = (newNormal * (newNormal * lightDir * 2.f) - lightDir).normalize();   // reflected light, in world space
        float spec = pow(std::max(reflect.z, 0.0f), model->specular(uv));
        TGAColor c = model->diffuse(uv);
        for (int i = 0; i < 3; i++) color[i] = std::min<float>(20 + c[i]  * (1.2 * shadow * newIntensity + .6 * spec), 255);

        // color = model->diffuse(uv) * intensity; 
        // color = model->diffuse(uv) * newIntensity;
        return false;
    }
};

struct ShadowShader : public ShaderBase
{
    mat<4, 3, float> varyingTri;

    virtual Vec4f vertexShader(int iface, int nthvert)
    {
        Vec4f glVertex = embed<4>(model->vert(iface, nthvert));
        glVertex = Viewport * Projection * ModelView * glVertex;
        varyingTri.set_col(nthvert, glVertex);
        return glVertex;
    }

    virtual bool fragmentShader(Vec3f bar, TGAColor& color) {
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
        model = new Model("../../../obj/diablo3_pose/diablo3_pose.obj");
    }

    {
        lookat(lightDir, center, right);
        viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
        projection(0);

        ShadowShader sdshader;
        for (int i = 0; i < model->nfaces(); i++) {
            Vec4f screenCoords[3];
            for (int j = 0; j < 3; j++) {
                screenCoords[j] = sdshader.vertexShader(i, j);
            }
            triangleShadow(screenCoords, sdshader, ShadowMap);
        }

        ShadowMap.write_tga_file("ShadowMap.tga");
    }

    Matrix ShadowTransform = Viewport * Projection * ModelView;

    {
        lookat(eye, center, up);
        viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
        projection(-1.f / (eye - center).norm());
        lightDir.normalize();

        TGAImage image(width, height, TGAImage::RGB);
        TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

        MyShader shader;
        shader.MVP = Projection * ModelView;
        shader.MVP_INVERSE = (Projection * ModelView).invert_transpose();
        shader.M_SHADOW = ShadowTransform * (Viewport * Projection * ModelView).invert();
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
    }

    delete model;
    return 0;
}