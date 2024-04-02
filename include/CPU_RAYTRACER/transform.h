#ifndef CPU_RAYTRACER_TRANSFORM_H
#define CPU_RAYTRACER_TRANSFORM_H

#include "utils.h"
#include "hittable.h"

// this class represents a transformation matrix node, which can be used to transform objects
// its children are the objects to be transformed, and itself is just a hittable object

namespace CPU_RAYTRACER {
    class transform : public hittable {
    public:
        transform(const shared_ptr<hittable>& _object, const glm::mat4& _model, shared_ptr<material> _material = nullptr) : object(_object), model_matrix(_model), mat(_material) {
            // compute the bounding box of the transformed object
            update_bounding_box();
        }

        bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
            // first check if the ray hit the bounding box of the transformed object
            if (!box.hit(r, ray_t)) {
                return false;
            }
            // transform the ray to local space
            glm::mat4 inv_m = glm::inverse(model_matrix);
            glm::vec3 origin_local = glm::vec3(inv_m * glm::vec4(r.origin(), 1.0f));// 1.0f for position
            glm::vec3 direction_local = glm::vec3(inv_m * glm::vec4(r.direction(), 0.0f));// 0.0f for direction
            // construct a new ray in local space
            ray r_local(origin_local, direction_local);
            //  do the hit test in local space
            if (object->hit(r_local, ray_t, rec)) {
                // if hit, transform the hit record back to world space
                // position and normal need to be transformed by the inverse transpose of the model matrix
                rec.p = glm::vec3(model_matrix * glm::vec4(rec.p, 1.0f));
                // normal need to be transformed by the inverse transpose of the model matrix
                // because the Model matrix may contain non-uniform scaling
                rec.normal = glm::normalize(glm::vec3(glm::transpose(inv_m) * glm::vec4(rec.normal, 0.0f)));
                // if the material is not null, it will override the material of the object
                if (mat != nullptr) {
                    rec.mat = mat;
                }
                return true;
            }
            return false;
        }

        AABB bounding_box() const override {
            return box;
        }

        void update_model_matrix(const glm::mat4& _model) {
            model_matrix = _model;
            update_bounding_box();
        }
        

    private:
        shared_ptr<hittable> object;
        glm::mat4 model_matrix;
        AABB box;
        shared_ptr<material> mat = nullptr; // if it is not null, it will override the material of the object

        // call this function after changing the model matrix
        void update_bounding_box() {
		    // we need to transform the bounding box to world space
		    // first get the bounding box in local space
		    AABB box_local = object->bounding_box();
		    // then transform the bounding box to world space
		    box =  transform_bounding_box(box_local, model_matrix);
	    }
    };
}




















#endif