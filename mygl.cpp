#include <cmath>
#include <limits>
#include <cstdlib>
#include "mygl.h"

Matrix ModelView;
Matrix Viewport;
Matrix Projection;

ShaderBase::~ShaderBase() {}

void viewport(int x, int y, int w, int h) 
{
    Viewport = Matrix::identity();
    Viewport[0][3] = x + w / 2.f;
    Viewport[1][3] = y + h / 2.f;
    Viewport[2][3] = 255.f / 2.f;
    Viewport[0][0] = w / 2.f;
    Viewport[1][1] = h / 2.f;
    Viewport[2][2] = 255.f / 2.f;
}

void projection(float coeff) 
{
    Projection = Matrix::identity();
    Projection[3][2] = coeff;
}

void lookat(Vec3f eye, Vec3f center, Vec3f up) 
{
    Vec3f z = (eye - center).normalize();
    Vec3f x = cross(up, z).normalize();
    Vec3f y = cross(z, x).normalize();
    ModelView = Matrix::identity();
    for (int i = 0; i < 3; i++) {
        ModelView[0][i] = x[i];
        ModelView[1][i] = y[i];
        ModelView[2][i] = z[i];
        ModelView[i][3] = -center[i];
    }
}

Vec3f getWeightInTriangle(Vec2f* pts, Vec2f point)
{
    // 对于透视投影的结果不准确，需要使用逆矩阵矫正
    Vec3f vec1{ pts[1].x - pts[0].x, pts[2].x - pts[0].x, point.x - pts[0].x };
    Vec3f vec2{ pts[1].y - pts[0].y, pts[2].y - pts[0].y, point.y - pts[0].y };

    Vec3f test = cross(vec1, vec2);
    if (test.z > 0) test = test * (-1);

    Vec3f Weight{-test.z - test.x - test.y, test.x, test.y};
    Weight = Weight * (1.0 / (-test.z));

    if (Weight.x < 0 || Weight.y < 0 || Weight.z < 0) return Vec3f{-1.0, -1.0, -1.0};
    return Weight;
}

void triangle(Vec4f* pts, ShaderBase& shader, TGAImage& image, TGAImage& zbuffer) {
    // 三角形的包围盒
    Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            bboxmin[j] = std::min(bboxmin[j], pts[i][j] / pts[i][3]);
            bboxmax[j] = std::max(bboxmax[j], pts[i][j] / pts[i][3]);
        }
    }
    Vec2i P;
    TGAColor color;
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            Vec2f points[3] = {proj<2>(pts[0] / pts[0][3]), proj<2>(pts[1] / pts[1][3]), proj<2>(pts[2] / pts[2][3])};
            Vec3f weight = getWeightInTriangle(points, proj<2>(P));
            float z = pts[0][2] * weight.x + pts[1][2] * weight.y + pts[2][2] * weight.z;
            float w = pts[0][3] * weight.x + pts[1][3] * weight.y + pts[2][3] * weight.z;
            int fragDepth = std::max(0, std::min(255, int(z / w + .5)));
            // zbuffer
            if(weight.x < 0 || weight.y < 0 || weight.z<0 || zbuffer.get(P.x, P.y)[0] > fragDepth) continue;
            bool discard = shader.fragmentShader(weight, color);
            if (!discard) {
                zbuffer.set(P.x, P.y, TGAColor(fragDepth));
                image.set(P.x, P.y, color);
            }
        }
    }
}

//// 光栅化相关
//// 早期代码, api变化失效
//void line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color)
//{
//    bool bFlip = false;
//    if (std::abs(x0 - x1) < std::abs(y0 - y1))
//    {
//        std::swap(x0, y0);
//        std::swap(x1, y1);
//        bFlip = true;
//    }
//
//    if (x0 > x1)
//    {
//        std::swap(x0, x1);
//        std::swap(y0, y1);
//    }
//
//    int Dy = std::abs(y1 - y0) >> 1;
//    int Dx = std::abs(x1 - x0) >> 1;
//    int Delta = 0;
//
//    int y = y0;
//    for (int x = x0; x <= x1; ++x)
//    {
//        if (bFlip) image.set(y, x, color);
//        else image.set(x, y, color);
//
//        Delta += Dy;
//        if (Delta > Dx)
//        {
//            if (y1 > y0) ++y;
//            else --y;
//
//            Delta -= Dx;
//        }
//    }
//}
//
//bool bInTriangle(Vec2i* pts, Vec2i point)
//{
//    // 判断点在三角形内
//    Vec3i vec1{ pts[1].x - pts[0].x, pts[2].x - pts[0].x, point.x - pts[0].x };
//    Vec3i vec2{ pts[1].y - pts[0].y, pts[2].y - pts[0].y, point.y - pts[0].y };
//
//    Vec3i test = vec1 ^ vec2;
//    if (test.z > 0) test = test * (-1);
//
//    return (test.x >= 0) && (test.x + test.z <= 0) && (test.y >= 0) && (test.y + test.z <= 0) && (test.x + test.y + test.z <= 0);
//}
//
//void triangle(Vec2i* pts, TGAImage& image, TGAColor color)
//{
//    // 划定BoundingBox
//    int minX = INT_MAX;
//    int minY = INT_MAX;
//    int maxX = INT_MIN;
//    int maxY = INT_MIN;
//    for (int i = 0; i < 3; ++i)
//    {
//        minX = std::min(minX, pts[i].x);
//        minY = std::min(minY, pts[i].y);
//        maxX = std::max(maxX, pts[i].x);
//        maxY = std::max(maxY, pts[i].y);
//    }
//
//    for (int u = minX; u <= maxX; ++u)
//    {
//        for (int v = minY; v <= maxY; ++v)
//        {
//            Vec2i point{ u, v };
//            if (bInTriangle(pts, point))
//            {
//                image.set(u, v, color);
//            }
//        }
//    }
//}
//
//Vec3f getWeightInTriangle(Vec2i* pts, Vec2i point)
//{
//    // 对于透视投影的结果不准确
//    Vec3i vec1{ pts[1].x - pts[0].x, pts[2].x - pts[0].x, point.x - pts[0].x };
//    Vec3i vec2{ pts[1].y - pts[0].y, pts[2].y - pts[0].y, point.y - pts[0].y };
//
//    Vec3i test = vec1 ^ vec2;
//    if (test.z > 0) test = test * (-1);
//
//    Vec3f Weight{ static_cast<float>(-test.z - test.x - test.y), static_cast<float>(test.x), static_cast<float>(test.y) };
//    Weight = Weight * (1.0 / (-test.z));
//
//    if (Weight.x < 0 || Weight.y < 0 || Weight.z < 0) return Vec3f{ -1.0, -1.0, -1.0 };
//    return Weight;
//}
//
//Vec3f getWeightInTrianglePerspective(Vec3f* pts, Vec2i point, Matrix viewport, Vec3f cameraLoc) // pts透视投影前坐标
//{
//    // 像素点逆向求权重
//
//    // 获取视口变换前，透视投影后的坐标
//    float x = (static_cast<float>(point.x) - viewport[0][3]) / viewport[0][0];
//    float y = (static_cast<float>(height - 1 - point.y) - viewport[1][3]) / viewport[1][1]; // 纵向翻转过，需要翻转回来
//
//    // (x, y, 0)与cameraLoc连线，求连线与透视投影前的三角形交点对应的权重（公式推导结果）
//    float param0 = (cameraLoc.x - x) * (pts[1].z - pts[0].z) - cameraLoc.z * (pts[1].x - pts[0].x);
//    float param1 = (cameraLoc.x - x) * (pts[2].z - pts[0].z) - cameraLoc.z * (pts[2].x - pts[0].x);
//    float param2 = (cameraLoc.x - x) * pts[0].z - cameraLoc.z * (pts[0].x - x);
//    float param3 = (cameraLoc.y - y) * (pts[1].z - pts[0].z) - cameraLoc.z * (pts[1].y - pts[0].y);
//    float param4 = (cameraLoc.y - y) * (pts[2].z - pts[0].z) - cameraLoc.z * (pts[2].y - pts[0].y);
//    float param5 = (cameraLoc.y - y) * pts[0].z - cameraLoc.z * (pts[0].y - y);
//    /*float param0 = -x * (pts[1].z - pts[0].z) - cameraLoc.z * (pts[1].x - pts[0].x);
//    float param1 = -x * (pts[2].z - pts[0].z) - cameraLoc.z * (pts[2].x - pts[0].x);
//    float param2 = -x * pts[0].z - cameraLoc.z * (pts[0].x - x - cameraLoc.x);
//    float param3 = -y * (pts[1].z - pts[0].z) - cameraLoc.z * (pts[1].y - pts[0].y);
//    float param4 = -y * (pts[2].z - pts[0].z) - cameraLoc.z * (pts[2].y - pts[0].y);
//    float param5 = -y * pts[0].z - cameraLoc.z * (pts[0].y - y - cameraLoc.y);*/
//    Vec3f vec1{ param0, param1, param2 };
//    Vec3f vec2{ param3, param4, param5 };
//
//    Vec3f test = vec1 ^ vec2;
//    if (test.z < 0.0) test = test * (-1.0);
//
//    Vec3f Weight{ test.z - test.x - test.y, test.x, test.y };
//    Weight = Weight * (1.0 / test.z);
//
//    if (Weight.x < 0 || Weight.y < 0 || Weight.z < 0) return Vec3f{ -1.0, -1.0, -1.0 };
//    return Weight;
//}
//
//void triangleWithZbuffer(Vec2i* pts, std::vector<std::vector<float>>& zbuffer, TGAImage& image, TGAColor color, Vec3f zArray)
//{
//    // zbuffer为全局zbuffer
//    // 划定BoundingBox
//    int minX = INT_MAX;
//    int minY = INT_MAX;
//    int maxX = INT_MIN;
//    int maxY = INT_MIN;
//    for (int i = 0; i < 3; ++i)
//    {
//        minX = std::min(minX, pts[i].x);
//        minY = std::min(minY, pts[i].y);
//        maxX = std::max(maxX, pts[i].x);
//        maxY = std::max(maxY, pts[i].y);
//    }
//    int m = zbuffer.size() - 1;
//    int n = zbuffer[0].size() - 1;
//    minX = std::max(0, minX);
//    minY = std::max(0, minY);
//    maxX = std::min(m, maxX);
//    maxY = std::min(n, maxY);
//
//    for (int u = minX; u <= maxX; ++u)
//    {
//        for (int v = minY; v <= maxY; ++v)
//        {
//            Vec2i point{ u, v };
//            Vec3f weight = getWeightInTriangle(pts, point);
//            if (weight.x >= 0 && weight.y >= 0 && weight.z >= 0)
//            {
//                float z = weight * zArray;
//                // 这里深度定义为z越大越好
//                if (zbuffer[u][v] < z)
//                {
//                    zbuffer[u][v] = z;
//                    image.set(u, v, color);
//                }
//            }
//        }
//    }
//}
//
//void triangleWithNorm(Vec2i* pts, std::vector<std::vector<float>>& zbuffer, TGAImage& image, int radiance, Vec3f zArray, Vec3f* normArray, Vec3f lightDirection)
//{
//    // zbuffer为全局zbuffer
//    // 划定BoundingBox
//    int minX = INT_MAX;
//    int minY = INT_MAX;
//    int maxX = INT_MIN;
//    int maxY = INT_MIN;
//    for (int i = 0; i < 3; ++i)
//    {
//        minX = std::min(minX, pts[i].x);
//        minY = std::min(minY, pts[i].y);
//        maxX = std::max(maxX, pts[i].x);
//        maxY = std::max(maxY, pts[i].y);
//    }
//    int m = zbuffer.size() - 1;
//    int n = zbuffer[0].size() - 1;
//    minX = std::max(0, minX);
//    minY = std::max(0, minY);
//    maxX = std::min(m, maxX);
//    maxY = std::min(n, maxY);
//
//    for (int u = minX; u <= maxX; ++u)
//    {
//        for (int v = minY; v <= maxY; ++v)
//        {
//            Vec2i point{ u, v };
//            Vec3f weight = getWeightInTriangle(pts, point);
//            if (weight.x >= 0 && weight.y >= 0 && weight.z >= 0)
//            {
//                float z = weight * zArray;
//                // 这里深度定义为z越大越好
//                if (zbuffer[u][v] < z)
//                {
//                    zbuffer[u][v] = z;
//                    Vec3f norm{ weight * normArray[0], weight * normArray[1], weight * normArray[2] };
//                    norm.normalize();
//                    float intensity = std::abs(norm * lightDirection);
//                    uint8_t gray = intensity * radiance;
//                    image.set(u, v, TGAColor{ gray, gray, gray, 255 });
//                }
//            }
//        }
//    }
//}
//
//void triangleWithTexture(Vec2i* pts, std::vector<std::vector<float>>& zbuffer, TGAImage& image, float intensity, Vec3f zArray, Vec3f* textureArray, TGAImage& textureImage)
//{
//    // zbuffer为全局zbuffer
//    // 划定BoundingBox
//    int minX = INT_MAX;
//    int minY = INT_MAX;
//    int maxX = INT_MIN;
//    int maxY = INT_MIN;
//    for (int i = 0; i < 3; ++i)
//    {
//        minX = std::min(minX, pts[i].x);
//        minY = std::min(minY, pts[i].y);
//        maxX = std::max(maxX, pts[i].x);
//        maxY = std::max(maxY, pts[i].y);
//    }
//    int m = zbuffer.size() - 1;
//    int n = zbuffer[0].size() - 1;
//    minX = std::max(0, minX);
//    minY = std::max(0, minY);
//    maxX = std::min(m, maxX);
//    maxY = std::min(n, maxY);
//
//    for (int u = minX; u <= maxX; ++u)
//    {
//        for (int v = minY; v <= maxY; ++v)
//        {
//            Vec2i point{ u, v };
//            Vec3f weight = getWeightInTriangle(pts, point);
//            if (weight.x >= 0 && weight.y >= 0 && weight.z >= 0)
//            {
//                float z = weight * zArray;
//                // 这里深度定义为z越大越好
//                if (zbuffer[u][v] < z)
//                {
//                    zbuffer[u][v] = z;
//                    Vec2f texture{ weight * textureArray[0], weight * textureArray[1] };
//                    TGAColor textureColor = textureImage.get(static_cast<int>(texture.x * textureImage.width()), static_cast<int>(texture.y * textureImage.height()));
//                    // TGAColor textureColor{255, 255, 255, 255};
//                    TGAColor setColor{ static_cast<uint8_t>(textureColor[0] * intensity),
//                                        static_cast<uint8_t>(textureColor[1] * intensity),
//                                        static_cast<uint8_t>(textureColor[2] * intensity),
//                                        textureColor[3] };
//                    image.set(u, v, setColor);
//                }
//            }
//        }
//    }
//}
//
//void triangleWithNormTexture(Vec2i* pts, std::vector<std::vector<float>>& zbuffer, TGAImage& image, Vec3f zArray, Vec3f* normArray, Vec3f lightDirection, Vec3f* textureArray, TGAImage& textureImage)
//{
//    // zbuffer为全局zbuffer
//    // 划定BoundingBox
//    int minX = INT_MAX;
//    int minY = INT_MAX;
//    int maxX = INT_MIN;
//    int maxY = INT_MIN;
//    for (int i = 0; i < 3; ++i)
//    {
//        minX = std::min(minX, pts[i].x);
//        minY = std::min(minY, pts[i].y);
//        maxX = std::max(maxX, pts[i].x);
//        maxY = std::max(maxY, pts[i].y);
//    }
//    int m = zbuffer.size() - 1;
//    int n = zbuffer[0].size() - 1;
//    minX = std::max(0, minX);
//    minY = std::max(0, minY);
//    maxX = std::min(m, maxX);
//    maxY = std::min(n, maxY);
//
//    for (int u = minX; u <= maxX; ++u)
//    {
//        for (int v = minY; v <= maxY; ++v)
//        {
//            Vec2i point{ u, v };
//            Vec3f weight = getWeightInTriangle(pts, point);
//            if (weight.x >= 0 && weight.y >= 0 && weight.z >= 0)
//            {
//                float z = weight * zArray;
//                // 这里深度定义为z越大越好
//                if (zbuffer[u][v] < z)
//                {
//                    zbuffer[u][v] = z;
//                    Vec3f norm{ weight * normArray[0], weight * normArray[1], weight * normArray[2] };
//                    norm.normalize();
//                    float intensity = std::abs(norm * lightDirection);
//                    Vec2f texture{ weight * textureArray[0], weight * textureArray[1] };
//                    TGAColor textureColor = textureImage.get(static_cast<int>(texture.x * textureImage.width()), static_cast<int>(texture.y * textureImage.height()));
//                    TGAColor setColor{ static_cast<uint8_t>(textureColor[0] * intensity),
//                                        static_cast<uint8_t>(textureColor[1] * intensity),
//                                        static_cast<uint8_t>(textureColor[2] * intensity),
//                                        textureColor[3] };
//                    // std::cout<<(int)(textureColor[0])<<", "<<(int)(setColor[0])<<std::endl;
//                    image.set(u, v, setColor);
//                }
//            }
//        }
//    }
//}
//
//void triangleWithNormTexturePerspective(Vec2i* pts, Vec3f* verts, Matrix viewport, Vec3f cameraLoc, std::vector<std::vector<float>>& zbuffer, TGAImage& image, Vec3f zArray, Vec3f* normArray, Vec3f lightDirection, Vec3f* textureArray, TGAImage& textureImage)
//{
//    // 对透视投影的结果，需要调整权重的计算
//
//    // zbuffer为全局zbuffer
//    // 划定BoundingBox
//    int minX = INT_MAX;
//    int minY = INT_MAX;
//    int maxX = INT_MIN;
//    int maxY = INT_MIN;
//    for (int i = 0; i < 3; ++i)
//    {
//        minX = std::min(minX, pts[i].x);
//        minY = std::min(minY, pts[i].y);
//        maxX = std::max(maxX, pts[i].x);
//        maxY = std::max(maxY, pts[i].y);
//    }
//    int m = zbuffer.size() - 1;
//    int n = zbuffer[0].size() - 1;
//    minX = std::max(0, minX);
//    minY = std::max(0, minY);
//    maxX = std::min(m, maxX);
//    maxY = std::min(n, maxY);
//
//    for (int u = minX; u <= maxX; ++u)
//    {
//        for (int v = minY; v <= maxY; ++v)
//        {
//            Vec2i point{ u, v };
//            Vec3f weight = getWeightInTrianglePerspective(verts, point, viewport, cameraLoc);
//            // Vec3f debugWeight = getWeightInTriangle(pts, point);
//            if (weight.x >= 0 && weight.y >= 0 && weight.z >= 0)
//            {
//                float z = weight * zArray;
//                // 这里深度定义为z越大越好
//                if (zbuffer[u][v] < z)
//                {
//                    zbuffer[u][v] = z;
//                    Vec3f norm{ weight * normArray[0], weight * normArray[1], weight * normArray[2] };
//                    norm.normalize();
//                    float intensity = std::abs(norm * lightDirection);
//                    Vec2f texture{ weight * textureArray[0], weight * textureArray[1] };
//                    TGAColor textureColor = textureImage.get(static_cast<int>(texture.x * textureImage.width()), static_cast<int>(texture.y * textureImage.height()));
//                    TGAColor setColor{ static_cast<uint8_t>(textureColor[0] * intensity),
//                                        static_cast<uint8_t>(textureColor[1] * intensity),
//                                        static_cast<uint8_t>(textureColor[2] * intensity),
//                                        textureColor[3] };
//                    // std::cout<<(int)(textureColor[0])<<", "<<(int)(setColor[0])<<std::endl;
//                    image.set(u, v, setColor);
//                }
//            }
//        }
//    }
//}
