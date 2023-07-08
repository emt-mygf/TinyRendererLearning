#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

Model::Model(const char *filename) : verts_(), vertNorms_(), vertTextures_(), 
faces_(), faceNorms_(), faceTextures_() {
    std::ifstream in;
    in.open(filename, std::ifstream::in);
    if(in.fail()) return;
    std::string line;
    while(!in.eof()) 
    {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if(!line.compare(0, 2, "v ")) 
        {
            iss >> trash;
            Vec3f v;
            for (int i=0; i<3; i++) iss >> v.raw[i];
            verts_.push_back(v);
        }
        else if(!line.compare(0, 3, "vn ")) 
        {
            iss >> trash >> trash;
            Vec3f vn;
            for (int i=0; i<3; i++) iss >> vn.raw[i];
            vertNorms_.push_back(vn);
        }
        else if(!line.compare(0, 3, "vt ")) 
        {
            iss >> trash >> trash;
            Vec3f vt;
            for (int i=0; i<3; i++) iss >> vt.raw[i];
            vertTextures_.push_back(vt);
        }
        else if(!line.compare(0, 2, "f ")) 
        {
            std::vector<int> f;
            std::vector<int> fn;
            std::vector<int> ft;
            int idx, norm, texture;
            iss >> trash;
            while (iss >> idx >> trash >> texture >> trash >> norm) {
                idx--; // in wavefront obj all indices start at 1, not zero
                texture--;
                norm--;
                f.push_back(idx);
                ft.push_back(texture);
                fn.push_back(norm);
            }
            faces_.push_back(f);
            faceNorms_.push_back(fn);
            faceTextures_.push_back(ft);
        }
    }
    std::cerr << "# v# " << verts_.size() << " f# "  << faces_.size() << std::endl;
}

Model::~Model() {
}

int Model::nverts() {
    return (int)verts_.size();
}

int Model::nvertNorms() {
    return (int)vertNorms_.size();
}

int Model::nvertTextures() {
    return (int)vertTextures_.size();
}

int Model::nfaces() {
    return (int)faces_.size();
}

std::vector<int> Model::face(int idx) {
    return faces_[idx];
}

Vec3f Model::vert(int i) {
    return verts_[i];
}

std::vector<int> Model::faceNorm(int idx) {
    return faceNorms_[idx];
}

Vec3f Model::vertNorm(int i) {
    return vertNorms_[i];
}

std::vector<int> Model::faceTexture(int idx) {
    return faceTextures_[idx];
}

Vec3f Model::vertTexture(int i) {
    return vertTextures_[i];
}
