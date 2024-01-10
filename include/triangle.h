#ifndef TRIANGLE_H
#define TRIANGLE_H


#include "utils.h"

#include "hittable.h"

#include <glm/gtc/quaternion.hpp> 
#include <glm/gtx/quaternion.hpp>

class triangle : public hittable {
public:
	triangle() {}
	triangle(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, shared_ptr<material> _material) : v0(v0), v1(v1), v2(v2), mat(_material) {
		glm::vec3 e1 = v1 - v0;
		glm::vec3 e2 = v2 - v0;
		face_normal = glm::normalize(glm::cross(e1, e2));
		face_normal = glm::normalize(face_normal);
		glm::vec3 min = glm::min(v0, glm::min(v1, v2));
		glm::vec3 max = glm::max(v0, glm::max(v1, v2));
		box = AABB(min, max);
		// default uv
		uv0 = glm::vec2(0, 1);
		uv1 = glm::vec2(1, 1);
		uv2 = glm::vec2(0.5, 0);

	}
	triangle(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec2 _uv0, glm::vec2 _uv1, glm::vec2 _uv2, shared_ptr<material> _material) : v0(v0), v1(v1), v2(v2), uv0(uv0), uv1(uv1), uv2(uv2), mat(_material) {
		glm::vec3 e1 = v1 - v0;
		glm::vec3 e2 = v2 - v0;
		face_normal = glm::normalize(glm::cross(e1, e2));
		face_normal = glm::normalize(face_normal);
		glm::vec3 min = glm::min(v0, glm::min(v1, v2));
		glm::vec3 max = glm::max(v0, glm::max(v1, v2));
		box = AABB(min, max);
		uv0 = _uv0;
		uv1 = _uv1;
		uv2 = _uv2;
	}
	// Möller-Trumbore: https://blog.csdn.net/zhanxi1992/article/details/109903792
	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        const float EPSILON = 0.0000001f;
        glm::vec3 E1, E2, S1, S, S2;
        glm::vec3 origin = r.origin();
        glm::vec3 D = r.direction();
        float S1E1, inv_S1E1, b1, b2, t;

        E1 = v1 - v0;
        E2 = v2 - v0;
        S1 = glm::cross(D, E2);
        S1E1 = glm::dot(E1, S1);
		//std::cout << "hit triangle" << std::endl;
        if (S1E1 > -EPSILON && S1E1 < EPSILON)
            return false;    // 射线与三角形平行

        inv_S1E1 = 1.0f / S1E1;
        S = origin - v0;
        b1 = inv_S1E1 * glm::dot(S, S1);

        if (b1 < 0.0 || b1 > 1.0)
            return false;

        S2 = glm::cross(S, E1);
        b2 = inv_S1E1 * glm::dot(D, S2);

        if (b2 < 0.0 || b1 + b2 > 1.0)
            return false;

        // 计算t以找出射线与三角形的交点
        t = inv_S1E1 * glm::dot(E2, S2);
		
		if (!ray_t.surrounds(t)) { // 射线的长度在允许范围内	
			return false;
		}

		// enable this to make the triangle single sided (like rasterization)
		if (!(dot(r.direction(), face_normal) < 0)) {
			return false;
		}

		rec.t = t;
		rec.p = r.at(t);
		rec.mat = mat;
		rec.set_face_normal(r, face_normal);
		
		// 三角形的uv坐标
		get_triangle_uv(b1, b2, rec.u, rec.v);

		return true;
		


	};
	AABB bounding_box() const override {
		return box;
	}
private:
	glm::vec3 v0, v1, v2;
	// UV
	glm::vec2 uv0, uv1, uv2;
	shared_ptr<material> mat;
	AABB box;
	glm::vec3 face_normal;// use to determine if ray is hitting the front or back of the triangle, it is a rule defined by convention
	
	void get_triangle_uv(float b1, float b2, float& u, float& v) const {
		float b0 = 1 - b1 - b2;
		u = b0 * uv0.x + b1 * uv1.x + b2 * uv2.x;
		v = b0 * uv0.y + b1 * uv1.y + b2 * uv2.y;	
	}

};



#endif