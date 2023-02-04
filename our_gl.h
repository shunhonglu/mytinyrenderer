#ifndef OUR_GH_H
#define OUR_GH_H

#include "tgaimage.h"
#include "geometry.h"

extern Matrix ModelMat;
extern Matrix View;
extern Matrix Projection;
extern Matrix Viewport;

void set_model_mat(const float& angle, const Vec3f& scale, const Vec3f& translate);
void set_view_mat(Vec3f& camera_position, Vec3f& camera_dir, Vec3f& up);
void set_projection_matrix(const float& eye_fov, const float& aspect_ratio, const float& zNear, const float& zFar);
void set_viewport_mat(int x, int y, int w, int h);
Vec4f transformation(const Vec4f& vertex);

struct IShader {
    virtual ~IShader() = default;
    virtual Vec4f vertex(int iface, int nthvert) = 0;
    virtual bool fragment(Vec3f bar, TGAColor &color) = 0;
};

void triangle(Vec4f *pts, IShader &shader, TGAImage &image, TGAImage &zbuffer, const float& near, const float& far);

#endif