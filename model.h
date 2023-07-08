#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"

class Model {
private:
	std::vector<Vec3f> verts_;
	std::vector<Vec3f> vertNorms_;
	std::vector<Vec3f> vertTextures_;
	std::vector<std::vector<int> > faces_;
	std::vector<std::vector<int> > faceNorms_;
	std::vector<std::vector<int> > faceTextures_;
public:
	Model(const char *filename);
	~Model();
	int nverts();
	int nvertNorms();
	int nvertTextures();
	int nfaces();
	Vec3f vert(int i);
	Vec3f vertNorm(int i);
	Vec3f vertTexture(int i);
	std::vector<int> face(int idx);
	std::vector<int> faceNorm(int idx);
	std::vector<int> faceTexture(int idx);
};

#endif //__MODEL_H__
