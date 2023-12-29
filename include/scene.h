#ifndef SCENE_H
#define SCENE_H


#include "utils.h"

#include "raytrace_camera.h"
#include "color.h"
#include "hittable_list.h"
#include "bvh.h"
#include "material.h"
#include "sphere.h"
#include <glm/glm.hpp>



#include <iostream>
#include <chrono>

class scene {
  public:
	hittable_list world;
    // std::vector<Object*> objects; // 存储场景中的物体
    // std::vector<Light*> lights;   // 存储场景中的光源


	scene() {
        shared_ptr<texture> earth_texture = make_shared<image_texture>("../../resource/earthmap.jpg");
        shared_ptr<material> earth_surface = make_shared<lambertian>(earth_texture);


        auto ground_material = make_shared<lambertian>(glm::vec3(0.5, 0.5, 0.5));
        world.add(make_shared<sphere>(glm::vec3(0, -10, 0), 10, ground_material));

        // for (int a = -3; a < 3; a++) {
        //     for (int b = -3; b < 3; b++) {
        //         auto choose_mat = random_float();
        //         glm::vec3 center(a + 0.9 * random_float(), 0.2 + 0.9 * random_float(), b + 0.9 * random_float());
        // 
        //         if ((center - glm::vec3(4, 0.2, 0)).length() > 0.9) {
        //             shared_ptr<material> sphere_material;
        // 
        //             if (choose_mat < 0.8) {
        //                 // diffuse
        //                 auto albedo = vec3_random() * vec3_random();
        //                 sphere_material = make_shared<lambertian>(albedo);
        //                 world.add(make_shared<sphere>(center, 0.2, sphere_material));
        //             }
        //             else if (choose_mat < 0.95) {
        //                 // metal
        //                 auto albedo = vec3_random(0.5, 1);
        //                 auto fuzz = random_float(0, 0.5);
        //                 sphere_material = make_shared<metal>(albedo, fuzz);
        //                 world.add(make_shared<sphere>(center, 0.2, sphere_material));
        //             }
        //             else {
        //                 // glass
        //                 sphere_material = make_shared<dielectric>(1.5);
        //                 world.add(make_shared<sphere>(center, 0.2, sphere_material));
        //             }
        //         }
        //     }
        // }

        auto material1 = make_shared<dielectric>(1.5);
        world.add(make_shared<sphere>(glm::vec3(0, 1, 0), 1.0, material1));

        // auto material2 = make_shared<lambertian>(glm::vec3(0.4, 0.2, 0.1));
        // shared_ptr<sphere> earth = make_shared<sphere>(glm::vec3(4, 1, 0), 1.0, earth_surface);
        // world.add(earth);
        // glm::quat q = glm::angleAxis(glm::radians(90.0f), glm::vec3(1, 0, 0));
        // earth->rotate(q);
        // 
        // auto material3 = make_shared<metal>(glm::vec3(0.7, 0.6, 0.5), 0.0);
        // world.add(make_shared<sphere>(glm::vec3(-4, 1, 0), 1.0, material3));


        hittable_list BVH_world;
        BVH_world = hittable_list(make_shared<BVH_node>(world));
        world = BVH_world;
	}
};



#endif