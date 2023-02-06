#include <iostream>
#include <string>
#include <limits>
#include "our_gl.h"

Matrix ModelMat;
Matrix View;
Matrix Projection;
Matrix Viewport;

const float PI = atan(1)*4;

void print(const std::string msg, const Matrix& m) {
    std::cout << msg << ':' << std::endl;
    for(int i=0; i<4; i++) {
        std::cout << m[i][0] << '\t' << m[i][1] << '\t' << m[i][2] << '\t' << m[i][3] << std::endl;
    }
}

/**************************************** MVP BEG ****************************************/
void set_model_mat(const float &angle, const Vec3f& scale, const Vec3f& translate) {
    // To-do: rotation around any axis!
    Matrix rotation_mat = Matrix::identity();
    float arc = angle * PI / 180.f; 
    rotation_mat[0][0] = cos(arc), rotation_mat[0][2] = sin(arc);
    rotation_mat[2][0] = -sin(arc), rotation_mat[2][2] = cos(arc);

    Matrix scale_mat = Matrix::identity();
    for (int i=0; i<3; i++) {
        scale_mat[i][i] = scale[i];
    }
 
    Matrix translate_mat = Matrix::identity();
    for (int i=0; i<3; i++) {
        translate_mat[i][3] = translate[i];
    }

    ModelMat = translate_mat * scale_mat * rotation_mat;
    
    print("ModelMat", ModelMat);
}

// you should make sure that **camera_dir** and **up** are orthogonal
void set_view_mat(Vec3f& camera_position, Vec3f& camera_dir, Vec3f& up) {
    // reference GAMES101
    // camera_dir correspond to -z axis in fact! 
    Vec3f z = camera_dir.normalize();
    Vec3f y = up.normalize();
    // Attention
    Vec3f x = cross(z, y).normalize();
    Matrix Minv = Matrix::identity();
    Matrix Tr   = Matrix::identity();
    for (int i=0; i<3; i++) {
        Minv[0][i] = x[i];
        Minv[1][i] = y[i];
        Minv[2][i] = -z[i];
        Tr[i][3] = -camera_position[i];
    }
    View = Minv*Tr;

    print("View", View);
}

void set_projection_matrix(const float& eye_fov, const float& aspect_ratio, const float& zNear, const float& zFar)
{
    Projection = Matrix::identity();

    float eye_fov_arc = eye_fov / 180 * PI;

    Projection[0][0] = 1 / (aspect_ratio * tan(eye_fov_arc/2));
    Projection[1][1] = 1 / (tan(eye_fov_arc/2));
    Projection[2][2] = -(zFar + zNear) / (zFar - zNear), Projection[2][3] = -2*zNear*zFar / (zFar - zNear);
    Projection[3][2] = -1, Projection[3][3] = 0;

    print("Projection", Projection);
}

// We can easily get black-and-white images of the z-buffer for debugging!
void set_viewport_mat(int x, int y, int w, int h) {
    Viewport = Matrix::identity();
    Viewport[0][3] = x+w/2.f;
    Viewport[1][3] = y+h/2.f;
    Viewport[2][3] = 255.f/2.f;
    Viewport[0][0] = w/2.f;
    Viewport[1][1] = h/2.f;
    Viewport[2][2] = 255.f/2.f;

    print("Viewport", Viewport);
}

Vec4f transformation(const Vec4f& vertex) {
    Vec4f proj_coord = Projection*View*ModelMat*vertex;

    Vec4f homo_coord; 
    for(int i=0; i<4; ++i) {
        homo_coord[i] = proj_coord[i]/proj_coord[3];
    }

    Vec4f screen_coord = Viewport*homo_coord;
    screen_coord[3] = proj_coord[3];

    return screen_coord;
}
/**************************************** MVP END ****************************************/

Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P) {
    Vec3f s[2];
    for (int i=2; i--; ) {
        s[i][0] = C[i]-A[i];
        s[i][1] = B[i]-A[i];
        s[i][2] = A[i]-P[i];
    }
    Vec3f u = cross(s[0], s[1]);
    if (std::abs(u[2])>1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
        return Vec3f(1.f-(u.x+u.y)/u.z, u.y/u.z, u.x/u.z);
    return Vec3f(-1,1,1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
}

void triangle(Vec4f *pts, IShader &shader, TGAImage &image, TGAImage &zbuffer,
    vector<vector<float>>& sample_list, vector<vector<TGAColor>>& sample_list_color, const float& near, const float& far) {
    // bounding box
    Vec2f bboxmin( std::numeric_limits<float>::max(),  std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            bboxmin[j] = std::min(bboxmin[j], pts[i][j]);
            bboxmax[j] = std::max(bboxmax[j], pts[i][j]);
        }
    }
    Vec2i P;
    TGAColor color;

    bool flag = false;
    if(!sample_list.empty() && !sample_list_color.empty()) {
        flag = true;
    }
    
    std::vector<std::vector<float>> offset = {
        {0.25f, 0.25f},
        {0.25f, 0.75f},
        {0.75f, 0.25f},
        {0.75f, 0.75f}
    };

    for (P.x=bboxmin.x; P.x<=bboxmax.x; P.x++) {
        for (P.y=bboxmin.y; P.y<=bboxmax.y; P.y++) {
            if(flag) {
                int cnt = 0;
                int index = 0;
                int No = P.x + P.y * image.get_width();
                for(auto& vec:offset) {
                    Vec3f c = barycentric(proj<2>(pts[0]), proj<2>(pts[1]), proj<2>(pts[2]), Vec2f(P.x+vec[0], P.y+vec[1]));
                    float alpha = c.x / pts[0][3], beta = c.y / pts[1][3], gamma = c.z / pts[2][3];
                    float z_interpolated = 1 / (alpha + beta + gamma);
                    if (c.x>=0 && c.y>=0 && c.z>=0 && z_interpolated<=sample_list[No][index]) {
                        ++cnt;
                        bool discard = shader.fragment(Vec3f(alpha, beta, gamma), color);
                        if (!discard) {
                            sample_list[No][index] = z_interpolated; 
                            sample_list_color[No][index] = color;
                        }
                    }
                    ++index;
                }

                if(cnt){
                    float z_interpolated_result = 0;
                    Vec4f result;
                    for(auto&v : sample_list[No]) {
                        z_interpolated_result += v;
                    }
                    for(auto&v : sample_list_color[No]) {
                        result[0] += v[0];
                        result[1] += v[1];
                        result[2] += v[2];
                        result[3] += v[3];
                    }
                    
                    float z_interpolated_grayscale = (z_interpolated_result/4.f - near)/(far-near)*255.f;
                    zbuffer.set(P.x, P.y, TGAColor(z_interpolated_grayscale));
                    image.set(P.x, P.y, TGAColor(result[0]/4, result[1]/4, result[2]/4, result[3]/4));
                }
            }
            else {
                Vec3f c = barycentric(proj<2>(pts[0]), proj<2>(pts[1]), proj<2>(pts[2]), Vec2f(P.x+0.5f, P.y+0.5f));
                float alpha = c.x / pts[0][3], beta = c.y / pts[1][3], gamma = c.z / pts[2][3];
                float z_interpolated = 1 / (alpha + beta + gamma);
                float z_interpolated_grayscale = (z_interpolated - near)/(far-near)*255.f;
                if (c.x>=0 && c.y>=0 && c.z>=0 && z_interpolated_grayscale<=zbuffer.get(P.x, P.y)[0]) {
                    // it's **Vec3f(alpha, beta, gamma)** but not **c** because of perspective interpolation correction!
                    bool discard = shader.fragment(Vec3f(alpha, beta, gamma), color);
                    if (!discard) {
                        zbuffer.set(P.x, P.y, TGAColor(z_interpolated_grayscale));
                        image.set(P.x, P.y, color);
                    }
                }
            }
        }
    }
}

Vec4f toVec4f(const Vec3f& v, float f) {
    Vec4f v4;
    for(int i=0; i<3; ++i) {
        v4[i] = v[i];
    }
    v4[3] = f;
    return v4;
}
