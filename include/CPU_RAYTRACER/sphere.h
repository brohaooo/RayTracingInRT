#ifndef CPU_RAYTRACER_SPHERE_H
#define CPU_RAYTRACER_SPHERE_H


#include "utils.h"

#include "hittable.h"

#include <glm/gtc/quaternion.hpp> 
#include <glm/gtx/quaternion.hpp>

namespace CPU_RAYTRACER {
  class sphere : public hittable {
    public:
      sphere(glm::vec3 _center, float _radius, shared_ptr<material> _material, glm::quat _rotation = glm::quat())
        : center(_center), radius(_radius), mat(_material), rotation(_rotation){
          // Create bounding box
      	box = AABB(center - glm::vec3(radius), center + glm::vec3(radius));
      }

      bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
          // oc is the vector from the center of the sphere to the origin of the ray
          glm::vec3 oc = r.origin() - center;
          // a is the squared length of the ray direction
          auto a = glm::length2(r.direction()) ;
          // b is the dot product of oc and the ray direction
          auto half_b = dot(oc, r.direction());
          auto c = glm::length2(oc) - radius*radius;

          auto discriminant = half_b*half_b - a*c;
          if (discriminant < 0)// if the discriminant is negative, there is no real root, return false
              return false;

          // Find the nearest root that lies in the acceptable range.
          auto sqrtd = sqrt(discriminant);
          // sloving the euation for t, there are two roots (t = (-b +- sqrt(discriminant))/a = root1, root2)
          auto root = (-half_b - sqrtd) / a;
          if (!ray_t.surrounds(root)) {
              root = (-half_b + sqrtd) / a;
              if (!ray_t.surrounds(root))
                  return false;// if both roots are not in the acceptable range, return false
          }

          rec.t = root;
          rec.p = r.at(rec.t);
          glm::vec3 outward_normal = (rec.p - center) / radius;
          rec.set_face_normal(r, outward_normal);
          rec.mat = mat;
          glm::vec3 hit_point_object_space = glm::inverse(rotation) * (outward_normal);
          get_sphere_uv(hit_point_object_space, rec.u, rec.v);

          return true;
      }

      AABB bounding_box() const override {
  		return box;
  	}

      void rotate(glm::quat rot){
  		rotation = rot * rotation;
  	}

    private:
      glm::vec3 center; // center of the sphere, defined in world space
      float radius; // radius of the sphere, defined in world space
      shared_ptr<material> mat; // the material pointer of the sphere
      AABB box;// the bounding box of the sphere
      glm::quat rotation;// the rotation of the sphere, used for texture mapping
      // the input p is a point on the UNIT sphere

      static void get_sphere_uv(const glm::vec3& p, float& u, float& v) {
  		auto theta = acos(p.y);
  		auto phi = atan2(-p.z, -p.x) + pi;

  		u = phi / (2*pi);
  		v = theta / pi;
  	}
  };
}

#endif
