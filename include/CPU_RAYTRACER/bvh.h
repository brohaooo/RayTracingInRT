#ifndef CPU_RAYTRACER_BVH_H
#define CPU_RAYTRACER_BVH_H

#include "utils.h"
#include "hittable_list.h"
#include <iostream>
#include <algorithm>

// class for bounding volume hierarchy

namespace CPU_RAYTRACER {
	// class for bounding volume hierarchy node
	// it contains a pointer to elements list, and two pointers to children nodes
	class BVH_node : public hittable {
		public:
		BVH_node(const hittable_list& list)
		: BVH_node(list.objects, 0, list.objects.size()) {}


		BVH_node(const std::vector<shared_ptr<hittable>>& src_objects, int start, int end) {
			auto objects = src_objects; // Create a modifiable array of the source scene objects

			int axis = random_int(0, 2);
			auto comparator = (axis == 0) ? box_x_compare
				: (axis == 1) ? box_y_compare
				: box_z_compare;
			int object_span = end - start;
			if (object_span == 1) {
				left = right = objects[start];
			}
			else if (object_span == 2) {
				if (comparator(objects[start], objects[start + 1])) {
					left = objects[start];
					right = objects[start + 1];
				}
				else {
					left = objects[start + 1];
					right = objects[start];
				}
			}
			else {
				std::sort(objects.begin() + start, objects.begin() + end, comparator);
				auto mid = start + object_span / 2;
				left = make_shared<BVH_node>(objects, start, mid);
				right = make_shared<BVH_node>(objects, mid, end);
			}

		    box = AABB(left->bounding_box(), right->bounding_box());


		}

		bool hit(const ray& r, interval ray_t, hit_record& rec) const override{
			if (!box.hit(r, ray_t))
				return false;
			bool hit_left = left->hit(r, ray_t, rec);
			bool hit_right = right->hit(r, interval(ray_t.min, hit_left ? rec.t : ray_t.max), rec);
			return hit_left || hit_right;
		}
		AABB bounding_box() const override {
			return box;
		}


		private:
		shared_ptr<hittable> left;
		shared_ptr<hittable> right;
		AABB box;


		static bool box_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b, int axis) {
			AABB box_a = a->bounding_box();
			AABB box_b = b->bounding_box();
			return box_a.min()[axis] < box_b.min()[axis];
		}
		static bool box_x_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
			return box_compare(a, b, 0);
		}
		static bool box_y_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
			return box_compare(a, b, 1);
		}
		static bool box_z_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
			return box_compare(a, b, 2);
		}


	};
}

#endif