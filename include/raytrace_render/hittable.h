#ifndef HITTABLE_H
#define HITTABLE_H


#include "utils.h"

class material;
// AABB = axis-aligned bounding box
class AABB {
public:
	AABB() {}
	AABB(const glm::vec3& a, const glm::vec3& b) { _min = a; _max = b; }
	AABB(const AABB & box0, const AABB & box1) {
		_min = glm::vec3(fmin(box0._min.x, box1._min.x),
			fmin(box0._min.y, box1._min.y),
			fmin(box0._min.z, box1._min.z));
		_max = glm::vec3(fmax(box0._max.x, box1._max.x),
			fmax(box0._max.y, box1._max.y),
			fmax(box0._max.z, box1._max.z));
	}

	glm::vec3 min() const { return _min; }
	glm::vec3 max() const { return _max; }

	bool hit(const ray& r, interval ray_t) const {
		float tmin = ray_t.min;
		float tmax = ray_t.max;
		for (int a = 0; a < 3; a++) {
			// t0 is the intersection of the ray with the min plane
			auto t0 = fmin((_min[a] - r.origin()[a]) / (r.direction()[a]),
				(_max[a] - r.origin()[a]) / (r.direction()[a]));
			// t1 is the intersection of the ray with the max plane
			auto t1 = fmax((_min[a] - r.origin()[a]) / (r.direction()[a]),
				(_max[a] - r.origin()[a]) / (r.direction()[a] ));
			tmin = fmax(t0, tmin);
			tmax = fmin(t1, tmax);
			// if tmax <= tmin, the ray missed the box
			if (tmax <= tmin)
				return false;
		}
		return true;
	}


	glm::vec3 _min;
	glm::vec3 _max;

	void print() {
		std::cout << "AABB:" << std::endl;
		std::cout << "min: " << _min.x << " " << _min.y << " " << _min.z << std::endl;
		std::cout << "max: " << _max.x << " " << _max.y << " " << _max.z << std::endl;
	}


};



class hit_record {
  public:
    glm::vec3 p;
    glm::vec3 normal;
    shared_ptr<material> mat;
    float t;
    bool front_face;
	float u, v;

	glm::vec3 color;// for skybox only

    void set_face_normal(const ray& r, const glm::vec3& outward_normal) {
        // Sets the hit record normal vector.
        // NOTE: the parameter `outward_normal` is assumed to have unit length.

        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};


class hittable {
  public:
    virtual ~hittable() = default;

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;
    virtual AABB bounding_box() const = 0;

};


#endif
