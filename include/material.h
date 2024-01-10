#ifndef MATERIAL_H
#define MATERIAL_H


#include "utils.h"

#include "hittable_list.h"
#include "texture.h"


class material {
  public:
    virtual ~material() = default;

    virtual glm::vec3 emitted(const hit_record& rec) const {
		return glm::vec3(0,0,0);
	}

    virtual bool scatter(
        const ray& r_in, const hit_record& rec, glm::vec3& attenuation, ray& scattered
    ) const = 0;
};


class lambertian : public material {
  public:
    lambertian(const glm::vec3& a) : albedo(make_shared<constant_texture>(a)) {}
    lambertian(shared_ptr<texture> a) : albedo(a) {}

    bool scatter(const ray& r_in, const hit_record& rec, glm::vec3& attenuation, ray& scattered)
    const override {
        auto scatter_direction = rec.normal + random_unit_vector();

        // Catch degenerate scatter direction
        if (near_zero(scatter_direction))
            scatter_direction = rec.normal;

        scattered = ray(rec.p, scatter_direction);
        attenuation = albedo->value(rec.u, rec.v, rec.p);
        return true;
    }

  private:
    shared_ptr<texture> albedo;
};


class metal : public material {
  public:
    metal(const glm::vec3& a, float f) : albedo(a), fuzz(f < 1 ? f : 1) {}

    bool scatter(const ray& r_in, const hit_record& rec, glm::vec3& attenuation, ray& scattered)
    const override {
        glm::vec3 reflected = reflect(glm::normalize(r_in.direction()), rec.normal);
        scattered = ray(rec.p, reflected + fuzz*random_in_unit_sphere());
        attenuation = albedo;
        return (dot(scattered.direction(), rec.normal) > 0);
    }

  private:
    glm::vec3 albedo;
    float fuzz;
};


class dielectric : public material {
  public:
    dielectric(float index_of_refraction) : ir(index_of_refraction) {}

    bool scatter(const ray& r_in, const hit_record& rec, glm::vec3& attenuation, ray& scattered)
    const override {
        attenuation = glm::vec3(1.0, 1.0, 1.0);
        float refraction_ratio = rec.front_face ? (1.0/ir) : ir;

        glm::vec3 unit_direction = glm::normalize(r_in.direction());
        float cos_theta = fmin(dot(-unit_direction, rec.normal), 1.0);
        float sin_theta = sqrt(1.0 - cos_theta*cos_theta);

        bool cannot_refract = refraction_ratio * sin_theta > 1.0;
        glm::vec3 direction;

        if (cannot_refract || reflectance(cos_theta, refraction_ratio) > random_float())
            direction = reflect(unit_direction, rec.normal);
        else
            direction = refract(unit_direction, rec.normal, refraction_ratio);

        scattered = ray(rec.p, direction);
        return true;
    }

  private:
    float ir; // Index of Refraction

    static float reflectance(float cosine, float ref_idx) {
        // Use Schlick's approximation for reflectance.
        auto r0 = (1-ref_idx) / (1+ref_idx);
        r0 = r0*r0;
        return r0 + (1-r0)*pow((1 - cosine),5);
    }
};


class diffuse_light : public material {
  public:
	diffuse_light(shared_ptr<texture> a) : emit(a) {}
	diffuse_light(glm::vec3 c) : emit(make_shared<constant_texture>(c)) {}

	bool scatter(const ray& r_in, const hit_record& rec, glm::vec3& attenuation, ray& scattered)
	const override {
		return false;
	}

	virtual glm::vec3 emitted(const hit_record& rec) const {
        float u = rec.u;
        float v = rec.v;
        glm::vec3 p = rec.p;
		return emit->value(u, v, p);
	}

  private:
	shared_ptr<texture> emit;
};





#endif
