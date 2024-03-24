#ifndef CPU_RAYTRACER_HITTABLE_LIST_H
#define CPU_RAYTRACER_HITTABLE_LIST_H


#include "utils.h"

#include "hittable.h"

#include <memory>
#include <vector>

namespace CPU_RAYTRACER {
    class hittable_list : public hittable {
      public:
        std::vector<shared_ptr<hittable>> objects;

        hittable_list() {}
        hittable_list(shared_ptr<hittable> object) { add(object); }

        void clear() { objects.clear(); }

        void add(shared_ptr<hittable> object) {
            objects.push_back(object);
            box = AABB(box, object->bounding_box());
        }

        bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
            hit_record temp_rec;
            auto hit_anything = false;
            auto closest_so_far = ray_t.max;

            for (const auto& object : objects) {
                if (object->hit(r, interval(ray_t.min, closest_so_far), temp_rec)) {
                    hit_anything = true;
                    closest_so_far = temp_rec.t;
                    rec = temp_rec;
                }
            }

            return hit_anything;
        }

        AABB bounding_box() const override {
    		return box;
    	}

        private:
        AABB box;

    };
}

#endif
