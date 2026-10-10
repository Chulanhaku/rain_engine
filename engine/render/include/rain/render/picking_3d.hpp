#pragma once
#include <rain/core/math/vec2.hpp>
#include <rain/render/camera_3d.hpp>
#include <rain/runtime/spatial_query_3d.hpp>

namespace rain {
struct screen_viewport_3d { f32 x=0,y=0,width=0,height=0; };
// Continuous pixel coordinates, top-left origin, same units as viewport. Viewport
// offsets are supported; right/bottom edges are exclusive. Converts D3D [0,1]
// depth through the actual view_projection, including perspective/orthographic.
// The ray starts on the near plane and ends on the far plane. Query hit distance
// is measured from that near-plane origin. No hidden ECS or physics dependency.
// Invalid/outside/minimized/singular input => false, output reset to an empty ray.
[[nodiscard]] bool screen_point_to_ray_3d(const camera_3d_frame& camera,
    screen_viewport_3d viewport,vec2 point,ray_3d& ray);
}
