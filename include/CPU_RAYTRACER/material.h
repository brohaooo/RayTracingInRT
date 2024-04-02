#ifndef CPU_RAYTRACER_MATERIAL_H
#define CPU_RAYTRACER_MATERIAL_H


#include "utils.h"

#include "hittable_list.h"
#include "texture.h"

namespace CPU_RAYTRACER {
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
      lambertian(const glm::vec3& color) : material_texture(nullptr), base_color(color) {}
      lambertian(shared_ptr<texture> tex, const glm::vec3& color = glm::vec3(1,1,1)) : material_texture(tex), base_color(color) {}

      bool scatter(const ray& r_in, const hit_record& rec, glm::vec3& attenuation, ray& scattered)
      const override {
          auto scatter_direction = rec.normal + random_unit_vector();

          // Catch degenerate scatter direction
          if (near_zero(scatter_direction))
              scatter_direction = rec.normal;

          scattered = ray(rec.p, scatter_direction);
          if (material_texture == nullptr)
              attenuation = base_color;
          else{
              attenuation = material_texture->value(rec.u, rec.v, rec.p)*base_color;
          }
          
          return true;
      }

    private:
      glm::vec3 base_color;
      shared_ptr<texture> material_texture;
  };


  class metal : public material {
    public:
      metal(const glm::vec3& color, float f) : base_color(color), fuzz(f < 1 ? f : 1), material_texture(nullptr) {}
      metal(shared_ptr<texture> tex, const glm::vec3& color = glm::vec3(1,1,1), float f = 0) : base_color(color), fuzz(f < 1 ? f : 1), material_texture(tex) {}

      bool scatter(const ray& r_in, const hit_record& rec, glm::vec3& attenuation, ray& scattered)
      const override {
          glm::vec3 reflected = reflect(glm::normalize(r_in.direction()), rec.normal);
          scattered = ray(rec.p, reflected + fuzz*random_in_unit_sphere());
          attenuation = base_color;
          if (material_texture != nullptr){
              attenuation *= material_texture->value(rec.u, rec.v, rec.p);
          }
          return (dot(scattered.direction(), rec.normal) > 0);
      }

    private:
      glm::vec3 base_color;
      shared_ptr<texture> material_texture;
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
  	diffuse_light(const glm::vec3& color) : material_texture(nullptr), base_color(color) {}
  	diffuse_light(shared_ptr<texture> tex, const glm::vec3& color = glm::vec3(1,1,1)) : material_texture(tex), base_color(color) {}

  	bool scatter(const ray& r_in, const hit_record& rec, glm::vec3& attenuation, ray& scattered)
  	const override {
  		return false;
  	}

  	virtual glm::vec3 emitted(const hit_record& rec) const {
          float u = rec.u;
          float v = rec.v;
          glm::vec3 p = rec.p;
          if (material_texture == nullptr){
              return base_color;
          }
          else{
              return material_texture->value(u, v, p)*base_color;
          }
  	}

    private:
  	glm::vec3 base_color;
    shared_ptr<texture> material_texture;
  };
}




#endif
