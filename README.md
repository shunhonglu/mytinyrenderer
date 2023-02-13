## 1. Transformation

### 1.1 Model Transformation

You can rotate your model around y-axis now, and you can also scale or translate your model. but remenber that the model file in obj format is in the cube $[-1, 1]^{3}$.

```cpp
void set_model_mat(const float &angle, const Vec3f& scale, const Vec3f& translate);
```

**you can add rotation around any axis!**

### 1.2 View Transformation

You can set the position of the camera, the direction of the camera, and the direction of the camera up. But remember that the direction of the camera and the direction of the camera up must be orthogonal. It's not convenient that you can imporve it.

```cpp
void set_view_mat(Vec3f& camera_position, Vec3f& camera_dir, Vec3f& up);
```

### 1.3 Projection Transformation

You can set the closest distance and the farthest distance you can see. And you also can set vertical field of view and aspect radio.

```cpp
void set_projection_matrix(const float& eye_fov, const float& aspect_ratio, const float& zNear, const float& zFar);
```

## 3. MSAA

when we only rasterize the triangle numbered 25, we can obviously observe the difference between MSAA and noMSAA. The result is as following.

```cpp
// we can only rasterize the triangle numbered 25!
for (int i=25; i<26; i++) {
    Vec4f screen_coords[3];
    for (int j=0; j<3; j++) {
        screen_coords[j] = s->vertex(i, j);
    }
    triangle(screen_coords, *s, image, zbuffer, sample_list, sample_list_color, near, far);
}
```

![MSAA](./result/gouraud_output_MSAA.png)

![noMSAA](./result/gouraud_output_noMSAA.png)
