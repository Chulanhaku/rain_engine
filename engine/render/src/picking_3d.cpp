#include <rain/render/picking_3d.hpp>
#include <algorithm>
#include <cmath>

namespace rain {
bool screen_point_to_ray_3d(const camera_3d_frame& camera,screen_viewport_3d viewport,vec2 point,ray_3d& ray) {
    ray={.direction={},.max_distance=0};
    if (!std::isfinite(viewport.x) || !std::isfinite(viewport.y) || !std::isfinite(viewport.width) ||
        !std::isfinite(viewport.height) || viewport.width<=0 || viewport.height<=0 ||
        !std::isfinite(point.x) || !std::isfinite(point.y)) return false;
    const double u=(static_cast<double>(point.x)-viewport.x)/viewport.width;
    const double v=(static_cast<double>(point.y)-viewport.y)/viewport.height;
    if (u<0 || v<0 || u>=1 || v>=1) return false;
    double augmented[4][8]{};
    for (usize row=0;row<4;++row) for (usize column=0;column<4;++column) {
        const double value=camera.view_projection.values[row][column];
        if (!std::isfinite(value)) return false;
        augmented[row][column]=value;augmented[row][column+4]=row==column ? 1.0 : 0.0;
    }
    for (usize column=0;column<4;++column) {
        usize pivot=column;
        for (usize row=column+1;row<4;++row)
            if (std::abs(augmented[row][column])>std::abs(augmented[pivot][column])) pivot=row;
        if (augmented[pivot][column]==0) return false;
        if (pivot!=column) for (usize i=0;i<8;++i) std::swap(augmented[column][i],augmented[pivot][i]);
        const double scale=augmented[column][column];
        for (double& value:augmented[column]) value/=scale;
        for (usize row=0;row<4;++row) if (row!=column) {
            const double factor=augmented[row][column];
            for (usize i=0;i<8;++i) augmented[row][i]-=factor*augmented[column][i];
        }
    }
    double points[2][3]{};
    for (usize depth=0;depth<2;++depth) {
        const double ndc[4]{u*2-1,1-v*2,static_cast<double>(depth),1};
        double world[4]{};
        for (usize column=0;column<4;++column) for (usize row=0;row<4;++row)
            world[column]+=ndc[row]*augmented[row][column+4];
        if (!std::isfinite(world[3]) || world[3]==0) return false;
        for (usize i=0;i<3;++i) {
            points[depth][i]=world[i]/world[3];
            if (!std::isfinite(points[depth][i])) return false;
        }
    }
    const double dx=points[1][0]-points[0][0],dy=points[1][1]-points[0][1],dz=points[1][2]-points[0][2];
    const double distance=std::hypot(dx,dy,dz);
    if (!std::isfinite(distance) || distance<=0 || !std::isfinite(static_cast<f32>(distance))) return false;
    ray_3d result{.origin={static_cast<f32>(points[0][0]),static_cast<f32>(points[0][1]),static_cast<f32>(points[0][2])},
        .direction={static_cast<f32>(dx/distance),static_cast<f32>(dy/distance),static_cast<f32>(dz/distance)},
        .max_distance=static_cast<f32>(distance)};
    if (!std::isfinite(result.origin.x) || !std::isfinite(result.origin.y) || !std::isfinite(result.origin.z)) return false;
    ray=result;return true;
}
}
