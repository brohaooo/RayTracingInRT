#ifndef CPU_RAYTRACER_CAMERA_H
#define CPU_RAYTRACER_CAMERA_H


#include "utils.h"

#include "color.h"
#include "hittable.h"
#include "material.h"
#include "skybox.h"

#include "svpng.inc"

#include <iostream>

#include <thread> // for non-blocking rendering


namespace CPU_RAYTRACER {
    class camera {
      public:
        float aspect_ratio      = 1.0;  // Ratio of image width over height
        int    image_width       = 100;  // Rendered image width in pixel count
        int    samples_per_pixel = 10;   // Count of random samples for each pixel
        int    max_depth         = 10;   // Maximum number of ray bounces into scene

        float vfov     = 90;              // Vertical view angle (field of view)
        glm::vec3 lookfrom = glm::vec3(0,0,-1);  // Point camera is looking from
        glm::vec3 lookat   = glm::vec3(0,0,0);   // Point camera is looking at
        glm::vec3   vup      = glm::vec3(0,1,0);     // Camera-relative "up" direction

        float defocus_angle = 0;  // Variation angle of rays through each pixel
        float focus_dist = 10;    // Distance from camera lookfrom point to plane of perfect focus

        unsigned char * rendered_image = nullptr;


        void non_blocking_render(const hittable& world, bool & finish_flag, const skybox* skybox = nullptr) {
            initialize();
            std::thread t(&camera::render_thread, this, std::ref(world), std::ref(finish_flag), skybox);
            t.detach();
        }

        // rgba
        void render_thread(const hittable& world, bool& finish_flag, const skybox * skybox = nullptr) {
            unsigned char* p = rendered_image;
            for (int j = 0; j < image_height; ++j) {
                std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
                for (int i = 0; i < image_width; ++i) {
                    glm::vec3 pixel_color(0, 0, 0);
                    for (int sample = 0; sample < samples_per_pixel; ++sample) {
                        ray r = get_ray(i, j);
                        pixel_color += ray_color(r, max_depth, world, skybox);
                    }
                    pixel_color /= samples_per_pixel;

                    //pixel_color = linear_to_gamma(pixel_color);

                    static const interval intensity(0.000, 0.999);
                    *p++ = static_cast<unsigned char>(256 * intensity.clamp(pixel_color.x));
                    *p++ = static_cast<unsigned char>(256 * intensity.clamp(pixel_color.y));
                    *p++ = static_cast<unsigned char>(256 * intensity.clamp(pixel_color.z));
                    *p++ = 255;

                }
            }
            std::clog << "\rDone. Now writing to file...                 \n";
            FILE* file_pointer;
            std::string file_name = "../../outputs/" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".png";
            file_pointer = fopen(file_name.c_str(), "wb");

            if (file_pointer == NULL)
            {
                std::cout << "Error, Unable to open the file" << std::endl;
            }
            svpng(file_pointer, image_width, image_height, rendered_image, 1);

            std::clog << "\rDone.                 \n";
            finish_flag = true;
        }
    

        //----------legacy code-----------------------------------------------------------
        void render_to_stream(const hittable& world) {
            initialize();

            std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

            for (int j = 0; j < image_height; ++j) {
                std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
                for (int i = 0; i < image_width; ++i) {
                    glm::vec3 pixel_color(0, 0, 0);
                    for (int sample = 0; sample < samples_per_pixel; ++sample) {
                        ray r = get_ray(i, j);
                        pixel_color += ray_color(r, max_depth, world);
                    }
                    write_color(std::cout, pixel_color, samples_per_pixel);
                }
            }



            std::clog << "\rDone.                 \n";
        }

        void render_to_png(const hittable& world, const char* save_path = "../../outputs/image.png") {
            initialize();

            unsigned char* p = rendered_image;

            std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

            for (int j = 0; j < image_height; ++j) {
                std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
                for (int i = 0; i < image_width; ++i) {
                    glm::vec3 pixel_color(0, 0, 0);
                    for (int sample = 0; sample < samples_per_pixel; ++sample) {
                        ray r = get_ray(i, j);
                        pixel_color += ray_color(r, max_depth, world);
                    }
                    pixel_color /= samples_per_pixel;

                    //pixel_color = linear_to_gamma(pixel_color);

                    static const interval intensity(0.000, 0.999);
                    *p++ = static_cast<unsigned char>(256 * intensity.clamp(pixel_color.x));
                    *p++ = static_cast<unsigned char>(256 * intensity.clamp(pixel_color.y));
                    *p++ = static_cast<unsigned char>(256 * intensity.clamp(pixel_color.z));
                    *p++ = 255;

                }
            }
            FILE* file_pointer;
            file_pointer = fopen(save_path, "wb");

            if (file_pointer == NULL)
            {
                std::cout << "Error, Unable to open the file" << std::endl;
            }
            svpng(file_pointer, image_width, image_height, rendered_image, 1);

            std::clog << "\rDone.                 \n";
        }
        //--------------------------------------------------------------------------------

      private:
        int    image_height;    // Rendered image height
        glm::vec3 center;          // Camera center
        glm::vec3 pixel00_loc;     // Location of pixel 0, 0
        glm::vec3   pixel_delta_u;   // Offset to pixel to the right
        glm::vec3   pixel_delta_v;   // Offset to pixel below
        glm::vec3   u, v, w;         // Camera frame basis vectors
        glm::vec3   defocus_disk_u;  // Defocus disk horizontal radius
        glm::vec3   defocus_disk_v;  // Defocus disk vertical radius

        void initialize() {
            image_height = static_cast<int>(image_width / aspect_ratio);
            image_height = (image_height < 1) ? 1 : image_height;

            center = lookfrom;

            // Determine viewport dimensions.
            auto theta = glm::radians(vfov);
            auto h = tan(theta/2);
            auto viewport_height = 2 * h * focus_dist;
            auto viewport_width = viewport_height * (static_cast<float>(image_width)/image_height);

            // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
            w = glm::normalize(lookfrom - lookat);
            u = glm::normalize(cross(vup, w));
            v = cross(w, u);

            // Calculate the vectors across the horizontal and down the vertical viewport edges.
            glm::vec3 viewport_u = viewport_width * u;    // Vector across viewport horizontal edge
            glm::vec3 viewport_v = viewport_height * -v;  // Vector down viewport vertical edge

            // Calculate the horizontal and vertical delta vectors to the next pixel.
            pixel_delta_u = viewport_u / (float)image_width;
            pixel_delta_v = viewport_v / (float)image_height;

            // Calculate the location of the upper left pixel.
            auto viewport_upper_left = center - (focus_dist * w) - viewport_u/2.0f - viewport_v/2.0f;
            pixel00_loc = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);

            // Calculate the camera defocus disk basis vectors.
            auto defocus_radius = focus_dist * tan(glm::radians(defocus_angle / 2));
            defocus_disk_u = u * defocus_radius;
            defocus_disk_v = v * defocus_radius;

            // initialize the image buffer
            if (rendered_image != nullptr)
    			delete[] rendered_image;
            rendered_image = new unsigned char[4 * image_width * image_height];
            // initialize the image buffer, rgba, let the alpha channel be 0
            for (int i = 0; i < 4 * image_width * image_height; i++)
    		{
    			rendered_image[i] = 0;
    		}

        }

        ray get_ray(int i, int j) const {
            // Get a randomly-sampled camera ray for the pixel at location i,j, originating from
            // the camera defocus disk.

            auto pixel_center = pixel00_loc + ((float)i * pixel_delta_u) + ((float)j * pixel_delta_v);
            auto pixel_sample = pixel_center + pixel_sample_square();

            auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
            auto ray_direction = pixel_sample - ray_origin;

            return ray(ray_origin, ray_direction);
        }

        glm::vec3 pixel_sample_square() const {
            // Returns a random point in the square surrounding a pixel at the origin.
            float px = -0.5 + random_float();
            float py = -0.5 + random_float();
            return (px * pixel_delta_u) + (py * pixel_delta_v);
        }

        glm::vec3 pixel_sample_disk(float radius) const {
            // Generate a sample from the disk of given radius around a pixel at the origin.
            auto p = radius * random_in_unit_disk();
            return (p[0] * pixel_delta_u) + (p[1] * pixel_delta_v);
        }

        glm::vec3 defocus_disk_sample() const {
            // Returns a random point in the camera defocus disk.
            auto p = random_in_unit_disk();
            return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
        }

        glm::vec3 ray_color(const ray& r, int depth, const hittable& world, const skybox* skybox = nullptr ) const {
            // If we've exceeded the ray bounce limit, no more light is gathered.
            if (depth <= 0)
                return glm::vec3(0,0,0);

            hit_record rec;

            if (world.hit(r, interval(0.001, infinity), rec)) {
                ray scattered;
                glm::vec3 attenuation;
                // if it is a scatterable material
                if (rec.mat->scatter(r, rec, attenuation, scattered)) {
                    return attenuation * ray_color(scattered, depth-1, world, skybox) + rec.mat->emitted(rec);
                }
                // else it is a light
                else {
                    return rec.mat->emitted(rec);
                }
                // return glm::vec3(0,0,0);
            }

            glm::vec3 unit_direction = glm::normalize(r.direction());
            float a = 0.5*(unit_direction.y + 1.0);

            // if there is a skybox, return the color of the skybox
            if (skybox != nullptr) {
                return skybox->cube_sample_color(r);
    		}

            return float(1.0-a)* glm::vec3(1.0, 1.0, 1.0) + a* glm::vec3(0.5, 0.7, 1.0);
        }
    };
}

#endif
