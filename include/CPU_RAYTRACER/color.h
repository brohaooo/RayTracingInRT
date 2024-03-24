#ifndef CPU_RAYTRACER_COLOR_H
#define CPU_RAYTRACER_COLOR_H


#include "glm/glm.hpp"

#include <iostream>

namespace CPU_RAYTRACER {
    inline float linear_to_gamma(float linear_component)
    {
        return sqrt(linear_component);
    }

    inline glm::vec3 linear_to_gamma(glm::vec3 linear_color)
    {
    	return glm::vec3(linear_to_gamma(linear_color.x),
    				 linear_to_gamma(linear_color.y),
    				 linear_to_gamma(linear_color.z));
    }


    void write_color(std::ostream &out, glm::vec3 pixel_color, int samples_per_pixel) {
        auto r = pixel_color.x;
        auto g = pixel_color.y;
        auto b = pixel_color.z;

        // Divide the color by the number of samples.
        auto scale = 1.0 / samples_per_pixel;
        r *= scale;
        g *= scale;
        b *= scale;

        // Apply a linear to gamma transform for gamma 2
        r = linear_to_gamma(r);
        g = linear_to_gamma(g);
        b = linear_to_gamma(b);

        // Write the translated [0,255] value of each color component.
        static const interval intensity(0.000, 0.999);
        out << static_cast<int>(256 * intensity.clamp(r)) << ' '
            << static_cast<int>(256 * intensity.clamp(g)) << ' '
            << static_cast<int>(256 * intensity.clamp(b)) << '\n';
    }
}

#endif
