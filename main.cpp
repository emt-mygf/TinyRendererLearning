#include <vector>
#include <cmath>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
const TGAColor white = TGAColor{255, 255, 255, 255};
const TGAColor red   = TGAColor{0, 0, 255, 255};

Model* model = nullptr;
const int width  = 800;
const int height = 800;
const float maxZ = -std::numeric_limits<float>::max();


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

Vec3f getWeightInTriangle(Vec2i *pts, Vec2i point)
{
    // 不准确，实际上应该用投影逆变换在世界坐标系下求
    Vec3i vec1{pts[1].x - pts[0].x, pts[2].x - pts[0].x, point.x - pts[0].x};
    Vec3i vec2{pts[1].y - pts[0].y, pts[2].y - pts[0].y, point.y - pts[0].y};

    Vec3i test = vec1 ^ vec2;
    if(test.z > 0) test = test * (-1);
    
    Vec3f Weight{static_cast<float>(-test.z - test.x - test.y), static_cast<float>(test.x), static_cast<float>(test.y)};
    Weight = Weight * (1.0 / (-test.z));

    if(Weight.x < 0 || Weight.y < 0 || Weight.z < 0) return Vec3f{-1.0, -1.0, -1.0};
    return Weight;
}

void triangleWithZbuffer(Vec2i *pts, std::vector<std::vector<float>>& zbuffer, TGAImage &image, TGAColor color, Vec3f zArray)
{
    // zbuffer为全局zbuffer
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
            Vec3f weight = getWeightInTriangle(pts, point);
            if(weight.x >= 0 && weight.y >= 0 && weight.z >= 0)
            {
                float z = weight * zArray;
                // 这里深度定义为z越大越好
                if(zbuffer[u][v] < z)
                {
                    zbuffer[u][v] = z;
                    image.set(u, v, color);
                }
            }
        }
    }
}

void triangleWithNorm(Vec2i *pts, std::vector<std::vector<float>>& zbuffer, TGAImage &image, int radiance, Vec3f zArray, Vec3f* normArray, Vec3f lightDirection)
{
    // zbuffer为全局zbuffer
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
            Vec3f weight = getWeightInTriangle(pts, point);
            if(weight.x >= 0 && weight.y >= 0 && weight.z >= 0)
            {
                float z = weight * zArray;
                // 这里深度定义为z越大越好
                if(zbuffer[u][v] < z)
                {
                    zbuffer[u][v] = z;
                    Vec3f norm{weight * normArray[0], weight * normArray[1], weight * normArray[2]};
                    norm.normalize();
                    float intensity = std::abs(norm * lightDirection);
                    uint8_t gray = intensity * radiance;
                    image.set(u, v, TGAColor{gray, gray, gray, 255});
                }
            }
        }
    }
}

void triangleWithTexture(Vec2i *pts, std::vector<std::vector<float>>& zbuffer, TGAImage &image, float intensity, Vec3f zArray, Vec3f* textureArray, TGAImage& textureImage)
{
    // zbuffer为全局zbuffer
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
            Vec3f weight = getWeightInTriangle(pts, point);
            if(weight.x >= 0 && weight.y >= 0 && weight.z >= 0)
            {
                float z = weight * zArray;
                // 这里深度定义为z越大越好
                if(zbuffer[u][v] < z)
                {
                    zbuffer[u][v] = z;
                    Vec2f texture{weight * textureArray[0], weight * textureArray[1]};
                    TGAColor textureColor = textureImage.get(static_cast<int>(texture.x * textureImage.width()), static_cast<int>(texture.y * textureImage.height()));
                    // TGAColor textureColor{255, 255, 255, 255};
                    TGAColor setColor{static_cast<uint8_t>(textureColor[0] * intensity), 
                                        static_cast<uint8_t>(textureColor[1] * intensity),
                                        static_cast<uint8_t>(textureColor[2] * intensity),
                                        textureColor[3]};
                    image.set(u, v, setColor);
                }
            }
        }
    }
}

void triangleWithNormTexture(Vec2i *pts, std::vector<std::vector<float>>& zbuffer, TGAImage &image, Vec3f zArray, Vec3f* normArray, Vec3f lightDirection, Vec3f* textureArray, TGAImage& textureImage)
{
    // zbuffer为全局zbuffer
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
            Vec3f weight = getWeightInTriangle(pts, point);
            if(weight.x >= 0 && weight.y >= 0 && weight.z >= 0)
            {
                float z = weight * zArray;
                // 这里深度定义为z越大越好
                if(zbuffer[u][v] < z)
                {
                    zbuffer[u][v] = z;
                    Vec3f norm{weight * normArray[0], weight * normArray[1], weight * normArray[2]};
                    norm.normalize();
                    float intensity = std::abs(norm * lightDirection);
                    Vec2f texture{weight * textureArray[0], weight * textureArray[1]};
                    TGAColor textureColor = textureImage.get(static_cast<int>(texture.x * textureImage.width()), static_cast<int>(texture.y * textureImage.height()));
                    TGAColor setColor{static_cast<uint8_t>(textureColor[0] * intensity), 
                                        static_cast<uint8_t>(textureColor[1] * intensity),
                                        static_cast<uint8_t>(textureColor[2] * intensity),
                                        textureColor[3]};
                    // std::cout<<(int)(textureColor[0])<<", "<<(int)(setColor[0])<<std::endl;
                    image.set(u, v, setColor);
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    if(2==argc) 
    {
        model = new Model(argv[1]);
    } 
    else 
    {
        model = new Model("../obj/african_head/african_head.obj");
    }

    TGAImage textureImage(width, height, TGAImage::RGB);
    textureImage.read_tga_file("../obj/african_head/african_head_diffuse.tga");
    textureImage.flip_vertically();

    TGAImage image(width, height, TGAImage::RGB);
    std::vector<std::vector<float>> zbuffer(width, std::vector<float> (height, maxZ));
    
    Vec3f light_dir(0,0,-1);
    for (int i=0; i<model->nfaces(); i++) 
    {
        std::vector<int> face = model->face(i);
        std::vector<int> faceTexture = model->faceTexture(i);
        std::vector<int> faceNorm = model->faceNorm(i);
        Vec2i screenVerts[3];
        Vec3f verts[3];
        Vec3f vertTextures[3];
        Vec3f vertNorms[3];
        for(int j=0; j<3; j++) 
        {
            Vec3f v = model->vert(face[j]);
            verts[j] = v;
            Vec3f vt = model->vertTexture(faceTexture[j]);
            vertTextures[j] = vt;
            Vec3f vn = model->vertNorm(faceNorm[j]);
            vertNorms[j] = vn;
            // 投影到屏幕坐标系
            int x = (v.x+1.)*width/2.;
            int y = (-v.y+1.)*height/2.;
            screenVerts[j] = Vec2i{x, y};
        }
        Vec3f zArray{verts[0].z, verts[1].z, verts[2].z};
        Vec3f textureArray[2]{{vertTextures[0].x, vertTextures[1].x, vertTextures[2].x}, 
                            {vertTextures[0].y, vertTextures[1].y, vertTextures[2].y}};
        Vec3f normArray[3]{{vertNorms[0].x, vertNorms[1].x, vertNorms[2].x}, 
                            {vertNorms[0].y, vertNorms[1].y, vertNorms[2].y},
                            {vertNorms[0].z, vertNorms[1].z, vertNorms[2].z}};
        // triangleWithNorm(screenVerts, zbuffer, image, 255, zArray, normArray, light_dir);
        triangleWithNormTexture(screenVerts, zbuffer, image, zArray, normArray, light_dir, textureArray, textureImage);

        // 世界坐标系：法线，求灰度
        // Vec3f n = (verts[2] - verts[0]) ^ (verts[1] - verts[0]);
        // n.normalize();
        // float intensity = std::abs(n * light_dir);
        // // uint8_t gray = intensity * 255;
        // if(intensity > 0)
        // {
        //     // triangle(screenVerts, image, TGAColor{gray, gray, gray, 255});
        //     // triangleWithZbuffer(screenVerts, zbuffer, image, TGAColor{gray, gray, gray, 255}, zArray);
        //     // triangleWithTexture(screenVerts, zbuffer, image, intensity, zArray, textureArray, textureImage);
        // }
    }

    image.flip_vertically(); 
    image.write_tga_file("output.tga");
    delete model;
    return 0;
}