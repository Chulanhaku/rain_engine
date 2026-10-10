#include <rain/runtime/spatial_query_3d.hpp>
#include "physics_geometry_3d.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>

namespace rain {
namespace {
using triple = std::array<double,3>;
triple values(vec3 v) { return {v.x,v.y,v.z}; }
vec3 vector(const triple& v) { return {static_cast<f32>(v[0]),static_cast<f32>(v[1]),static_cast<f32>(v[2])}; }
bool finite(vec3 v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }
bool nonnegative(vec3 v) { return finite(v) && v.x>=0 && v.y>=0 && v.z>=0; }
bool entity_less(entity_id a,entity_id b) {
    return a.index<b.index || (a.index==b.index && a.generation<b.generation);
}
bool hit_less(const spatial_query_hit_3d& a,const spatial_query_hit_3d& b) {
    return a.distance<b.distance || (a.distance==b.distance && entity_less(a.entity,b.entity));
}
vec3 closest_point(vec3 p,vec3 center,vec3 half) {
    const auto a=values(p),c=values(center),h=values(half);
    triple out;
    for (usize i=0;i<3;++i) out[i]=std::clamp(a[i],c[i]-h[i],c[i]+h[i]);
    return vector(out);
}
vec3 advance(vec3 origin,vec3 direction,double t) {
    return {static_cast<f32>(origin.x+direction.x*t),static_cast<f32>(origin.y+direction.y*t),
        static_cast<f32>(origin.z+direction.z*t)};
}
// Slabs are conservative candidate tests only, never the rounded-box narrowphase.
bool intersects_bounds(const physics_aabb_3d& bounds,vec3 origin,vec3 direction,
    vec3 extent,double max_distance) {
    const auto lo=values(bounds.minimum),hi=values(bounds.maximum),o=values(origin),
        d=values(direction),e=values(extent);
    double enter=0,exit=max_distance;
    for (usize i=0;i<3;++i) {
        if (d[i]==0) {
            if (o[i]<lo[i]-e[i] || o[i]>hi[i]+e[i]) return false;
        } else {
            double a=(lo[i]-e[i]-o[i])/d[i],b=(hi[i]+e[i]-o[i])/d[i];
            if (a>b) std::swap(a,b);
            enter=std::max(enter,a);exit=std::min(exit,b);
            if (enter>exit) return false;
        }
    }
    return true;
}
struct geometry_hit { double distance; vec3 normal; };

// Exact point cast against box Minkowski-summed with a sphere. Distance to an
// AABB is piecewise quadratic: split only at its six slab crossings, then solve
// each interval analytically. This covers faces, rounded edges and corners,
// tangencies, thin objects and long sweeps without sampling/tunneling.
std::optional<geometry_hit> rounded_box_cast(vec3 origin,vec3 direction,vec3 center,
    const triple& h,double radius,double max_distance) {
    const auto source=values(origin),c=values(center),d=values(direction);
    triple o;
    for (usize i=0;i<3;++i) o[i]=source[i]-c[i];
    const auto distance_squared=[&](double t) {
        double sum=0;
        for (usize i=0;i<3;++i) {
            const double x=o[i]+d[i]*t;
            const double delta=x-std::clamp(x,-h[i],h[i]);
            sum+=delta*delta;
        }
        return sum;
    };
    const auto result=[&](double t) {
        triple p,n;
        double size=0;
        for (usize i=0;i<3;++i) {
            p[i]=o[i]+d[i]*t;
            n[i]=p[i]-std::clamp(p[i],-h[i],h[i]);size+=n[i]*n[i];
        }
        if (size>1e-24) {
            const double len=std::sqrt(size);
            for (double& v:n) v/=len;
        } else {
            usize axis=0;
            for (usize i=1;i<3;++i) if (h[i]-std::abs(p[i])<h[axis]-std::abs(p[axis])) axis=i;
            n={0,0,0};
            n[axis]=p[axis]<0 ? -1.0 : (p[axis]>0 ? 1.0 : (d[axis]>0 ? -1.0 : 1.0));
        }
        return geometry_hit{t,vector(n)};
    };
    const double radius_squared=radius*radius;
    if (distance_squared(0)<=radius_squared) return result(0);
    if (max_distance==0) return std::nullopt;
    std::array<double,8> cuts{};
    usize count=0;cuts[count++]=0;cuts[count++]=max_distance;
    for (usize i=0;i<3;++i) if (d[i]!=0) for (double face:{-h[i],h[i]}) {
        const double t=(face-o[i])/d[i];
        if (t>0 && t<max_distance) cuts[count++]=t;
    }
    std::sort(cuts.begin(),cuts.begin()+static_cast<std::ptrdiff_t>(count));
    for (usize interval=0;interval+1<count;++interval) {
        const double begin=cuts[interval],end=cuts[interval+1];
        if (begin==end) continue;
        const double middle=begin+(end-begin)*0.5;
        // Solve in local interval coordinates to reduce cancellation far from origin.
        double a=0,b=0,c0=-radius_squared;
        for (usize i=0;i<3;++i) {
            const double m=o[i]+d[i]*middle;
            if (m>=-h[i] && m<=h[i]) continue;
            const double delta=o[i]+d[i]*begin-(m<0 ? -h[i] : h[i]);
            a+=d[i]*d[i];b+=delta*d[i];c0+=delta*delta;
        }
        if (c0<=0) return result(begin);
        if (a==0) continue;
        double disc=b*b-a*c0;
        const double tolerance=1e-13*std::max({b*b,std::abs(a*c0),1e-30});
        if (disc< -tolerance) continue;
        disc=std::max(disc,0.0);
        const double q=-b-std::copysign(std::sqrt(disc),b);
        double first=q/a,second=q==0 ? first : c0/q;
        if (first>second) std::swap(first,second);
        const double root=first>=0 ? first : second;
        const double eps=1e-12*std::max(1.0,end-begin);
        if (root>=-eps && root<=end-begin+eps)
            return result(std::clamp(begin+root,begin,end));
    }
    return std::nullopt;
}
}

void spatial_query_3d::clear() {
    colliders_.clear();indices_.clear();nodes_.clear();++revision_;
}
void spatial_query_3d::sync(const world& w) {
    clear();
    const auto* pool=w.try_get_component_pool<collider_3d_component>();
    if (!pool) return;
    colliders_.reserve(pool->size());
    for (usize i=0;i<pool->size();++i) {
        const auto entity=pool->entity_at(i);
        if (!w.is_entity_active(entity) || !w.has_component<transform_3d_component>(entity) ||
            w.has_tag_in_hierarchy(entity,tag_id{"physics.disabled"})) continue;
        const auto& collider=pool->get(entity);
        if (!finite(collider.center)) continue;
        const auto matrix=detail::current_world_matrix(w,entity);
        bool valid=true;
        for (const auto& row:matrix.values) for (f32 v:row) valid &= std::isfinite(v);
        if (!valid) continue;
        collider_record record{};
        record.entity=entity;record.shape=collider.shape;
        if (collider.shape==collider_shape_3d::box && finite(collider.half_extents)) {
            const auto box=detail::make_world_aabb(matrix,collider);
            record.center=box.center;record.half_extents=box.half_extents;
        } else if (collider.shape==collider_shape_3d::sphere && std::isfinite(collider.radius)) {
            const auto sphere=detail::make_world_sphere(matrix,collider);
            record.center=sphere.center;record.radius=sphere.radius;
            record.half_extents={sphere.radius,sphere.radius,sphere.radius};
        } else continue;
        record.bounds={record.center-record.half_extents,record.center+record.half_extents};
        if (!finite(record.center) || !nonnegative(record.half_extents) ||
            !finite(record.bounds.minimum) || !finite(record.bounds.maximum)) continue;
        // Float center +/- extent may round inward. Expand one representable unit
        // so a true boundary hit is never lost before the double-precision narrowphase.
        const auto outward=[](f32 value,f32 direction) {
            const f32 expanded=std::nextafter(value,direction);
            return std::isfinite(expanded) ? expanded : value;
        };
        const f32 infinity=std::numeric_limits<f32>::infinity();
        auto& lo=record.bounds.minimum;auto& hi=record.bounds.maximum;
        lo={outward(lo.x,-infinity),outward(lo.y,-infinity),outward(lo.z,-infinity)};
        hi={outward(hi.x,infinity),outward(hi.y,infinity),outward(hi.z,infinity)};
        if (const auto* filter=w.try_get_component<collision_filter_3d_component>(entity)) record.filter=*filter;
        record.trigger=w.has_tag_in_hierarchy(entity,tag_id{"physics.trigger"});
        if (const auto* tags=w.try_get_tags(entity)) {
            record.tags=*tags;record.tags.clear_pending_events();
        }
        colliders_.push_back(std::move(record));
    }
    indices_.resize(colliders_.size());
    std::iota(indices_.begin(),indices_.end(),0u);
    nodes_.reserve(colliders_.size()*2);
    if (!indices_.empty()) build_node(0,static_cast<u32>(indices_.size()));
}
u32 spatial_query_3d::build_node(u32 begin,u32 end) {
    const u32 index=static_cast<u32>(nodes_.size());
    nodes_.push_back({});
    auto bounds=colliders_[indices_[begin]].bounds;
    for (u32 i=begin+1;i<end;++i) {
        const auto& b=colliders_[indices_[i]].bounds;
        bounds.minimum={std::min(bounds.minimum.x,b.minimum.x),std::min(bounds.minimum.y,b.minimum.y),std::min(bounds.minimum.z,b.minimum.z)};
        bounds.maximum={std::max(bounds.maximum.x,b.maximum.x),std::max(bounds.maximum.y,b.maximum.y),std::max(bounds.maximum.z,b.maximum.z)};
    }
    nodes_[index].bounds=bounds;
    if (end-begin<=4) { nodes_[index].begin=begin;nodes_[index].count=end-begin;return index; }
    const auto lo=values(bounds.minimum),hi=values(bounds.maximum);
    usize axis=0;
    for (usize i=1;i<3;++i) if (hi[i]-lo[i]>hi[axis]-lo[axis]) axis=i;
    const u32 middle=begin+(end-begin)/2;
    std::nth_element(indices_.begin()+begin,indices_.begin()+middle,indices_.begin()+end,[&](u32 a,u32 b) {
        const auto ca=values(colliders_[a].center)[axis],cb=values(colliders_[b].center)[axis];
        return ca<cb || (ca==cb && entity_less(colliders_[a].entity,colliders_[b].entity));
    });
    const auto left=build_node(begin,middle),right=build_node(middle,end);
    nodes_[index].left=left;nodes_[index].right=right;
    return index;
}

std::optional<spatial_query_hit_3d> spatial_query_3d::cast(collider_shape_3d shape,vec3 center,
    vec3 half,f32 radius,vec3 direction,f32 distance,const spatial_query_filter_3d& filter,
    std::vector<spatial_query_hit_3d>* all,bool any) const {
    if (all) all->clear();
    if (!finite(center) || !nonnegative(half) || !std::isfinite(radius) || radius<0 ||
        !finite(direction) || !std::isfinite(distance) || distance<0 || nodes_.empty()) return std::nullopt;
    const double direction_length=std::hypot(static_cast<double>(direction.x),direction.y,direction.z);
    if (distance>0 && direction_length==0) return std::nullopt;
    if (direction_length>0) direction={static_cast<f32>(direction.x/direction_length),
        static_cast<f32>(direction.y/direction_length),static_cast<f32>(direction.z/direction_length)};
    const vec3 extent=shape==collider_shape_3d::sphere ? vec3{radius,radius,radius} : half;
    std::optional<spatial_query_hit_3d> best;
    const auto visit=[&](auto&& self,u32 index)->bool {
        const auto& node=nodes_[index];
        if (!intersects_bounds(node.bounds,center,direction,extent,distance)) return false;
        if (node.count==0) return self(self,node.left) || self(self,node.right);
        for (u32 i=node.begin;i<node.begin+node.count;++i) {
            const auto& target=colliders_[indices_[i]];
            if ((filter.layer_mask & target.filter.layer)==0 ||
                (filter.source_layer!=0 && (filter.source_layer & target.filter.mask)==0) ||
                (filter.triggers==query_trigger_mode_3d::exclude && target.trigger) ||
                (filter.triggers==query_trigger_mode_3d::only && !target.trigger) ||
                std::find(filter.ignored_entities.begin(),filter.ignored_entities.end(),target.entity)!=filter.ignored_entities.end() ||
                !filter.tags.matches(target.tags)) continue;
            if (!intersects_bounds(target.bounds,center,direction,extent,distance)) continue;
            std::optional<geometry_hit> geometry;
            vec3 normal,point;
            if (shape==collider_shape_3d::sphere) {
                const bool sphere=target.shape==collider_shape_3d::sphere;
                geometry=rounded_box_cast(center,direction,target.center,
                    sphere ? triple{} : values(target.half_extents),
                    static_cast<double>(radius)+(sphere ? target.radius : 0),distance);
                if (!geometry) continue;
                normal=geometry->normal;
                if (sphere) point=target.center+normal*target.radius;
                else {
                    point=closest_point(advance(center,direction,geometry->distance),target.center,target.half_extents);
                    // A ray/sphere center inside a box has no unique witness: use nearest face.
                    if (geometry->distance==0 && length_squared(point-center)==0) {
                        const auto h=values(target.half_extents),c=values(target.center),n=values(normal);
                        auto p=values(point);
                        for (usize j=0;j<3;++j) if (n[j]!=0) p[j]=c[j]+n[j]*h[j];
                        point=vector(p);
                    }
                }
            } else if (target.shape==collider_shape_3d::box) {
                auto h=values(half);const auto th=values(target.half_extents);
                for (usize j=0;j<3;++j) h[j]+=th[j];
                geometry=rounded_box_cast(center,direction,target.center,h,0,distance);
                if (!geometry) continue;
                normal=geometry->normal;
                point=closest_point(advance(center,direction,geometry->distance),target.center,target.half_extents);
                auto p=values(point);const auto c=values(target.center),n=values(normal);
                for (usize j=0;j<3;++j) if (n[j]!=0) p[j]=c[j]+n[j]*th[j];
                point=vector(p);
            } else {
                // Relative motion: sweep target sphere backwards against the stationary query box.
                geometry=rounded_box_cast(target.center,-direction,center,values(half),target.radius,distance);
                if (!geometry) continue;
                normal=-geometry->normal;point=target.center+normal*target.radius;
            }
            const f32 t=static_cast<f32>(geometry->distance);
            spatial_query_hit_3d hit{.entity=target.entity,.point=point,.normal=normal,
                .distance=t,.fraction=distance>0 ? t/distance : 0,.started_overlapping=geometry->distance==0,
                .trigger=target.trigger};
            if (!best || hit_less(hit,*best)) best=hit;
            if (all) all->push_back(hit);
            if (any) return true;
        }
        return false;
    };
    visit(visit,0);
    if (all) std::sort(all->begin(),all->end(),hit_less);
    return best;
}
std::optional<spatial_query_hit_3d> spatial_query_3d::raycast(const ray_3d& ray,const spatial_query_filter_3d& filter) const {
    return cast(collider_shape_3d::sphere,ray.origin,{},0,ray.direction,ray.max_distance,filter,nullptr,false);
}
bool spatial_query_3d::raycast_any(const ray_3d& ray,const spatial_query_filter_3d& filter) const {
    return cast(collider_shape_3d::sphere,ray.origin,{},0,ray.direction,ray.max_distance,filter,nullptr,true).has_value();
}
void spatial_query_3d::raycast_all(const ray_3d& ray,std::vector<spatial_query_hit_3d>& hits,const spatial_query_filter_3d& filter) const {
    cast(collider_shape_3d::sphere,ray.origin,{},0,ray.direction,ray.max_distance,filter,&hits,false);
}
std::optional<spatial_query_hit_3d> spatial_query_3d::sweep_sphere(const query_sphere_3d& sphere,vec3 direction,f32 distance,const spatial_query_filter_3d& filter) const {
    return cast(collider_shape_3d::sphere,sphere.center,{},sphere.radius,direction,distance,filter,nullptr,false);
}
bool spatial_query_3d::sweep_sphere_any(const query_sphere_3d& sphere,vec3 direction,f32 distance,const spatial_query_filter_3d& filter) const {
    return cast(collider_shape_3d::sphere,sphere.center,{},sphere.radius,direction,distance,filter,nullptr,true).has_value();
}
void spatial_query_3d::sweep_sphere_all(const query_sphere_3d& sphere,vec3 direction,f32 distance,std::vector<spatial_query_hit_3d>& hits,const spatial_query_filter_3d& filter) const {
    cast(collider_shape_3d::sphere,sphere.center,{},sphere.radius,direction,distance,filter,&hits,false);
}
std::optional<spatial_query_hit_3d> spatial_query_3d::sweep_box(const query_box_3d& box,vec3 direction,f32 distance,const spatial_query_filter_3d& filter) const {
    return cast(collider_shape_3d::box,box.center,box.half_extents,0,direction,distance,filter,nullptr,false);
}
bool spatial_query_3d::sweep_box_any(const query_box_3d& box,vec3 direction,f32 distance,const spatial_query_filter_3d& filter) const {
    return cast(collider_shape_3d::box,box.center,box.half_extents,0,direction,distance,filter,nullptr,true).has_value();
}
void spatial_query_3d::sweep_box_all(const query_box_3d& box,vec3 direction,f32 distance,std::vector<spatial_query_hit_3d>& hits,const spatial_query_filter_3d& filter) const {
    cast(collider_shape_3d::box,box.center,box.half_extents,0,direction,distance,filter,&hits,false);
}
bool spatial_query_3d::overlap_sphere_any(const query_sphere_3d& sphere,const spatial_query_filter_3d& filter) const {
    return sweep_sphere_any(sphere,{},0,filter);
}
bool spatial_query_3d::overlap_box_any(const query_box_3d& box,const spatial_query_filter_3d& filter) const {
    return sweep_box_any(box,{},0,filter);
}
void spatial_query_3d::overlap_sphere(const query_sphere_3d& sphere,std::vector<spatial_query_hit_3d>& hits,const spatial_query_filter_3d& filter) const {
    sweep_sphere_all(sphere,{},0,hits,filter);
}
void spatial_query_3d::overlap_box(const query_box_3d& box,std::vector<spatial_query_hit_3d>& hits,const spatial_query_filter_3d& filter) const {
    sweep_box_all(box,{},0,hits,filter);
}
}
