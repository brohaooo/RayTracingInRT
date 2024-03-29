#ifndef CPU_RAYTRACER_RAY_H
#define CPU_RAYTRACER_RAY_H

#include "utils.h"

namespace CPU_RAYTRACER {
  class ray {
    public:
      ray() {}

      ray(const glm::vec3& origin, const glm::vec3& direction) : orig(origin), dir(direction) {}

      glm::vec3 origin() const  { return orig; }
      glm::vec3 direction() const { return dir; }

      glm::vec3 at(float t) const {
          return orig + dir*t;
      }

    private:
      glm::vec3 orig;
      glm::vec3 dir;
  };
}
#endif
