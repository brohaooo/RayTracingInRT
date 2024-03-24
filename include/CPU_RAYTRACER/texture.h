#ifndef CPU_RAYTRACER_TEXTURE_H
#define CPU_RAYTRACER_TEXTURE_H

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "utils.h"
#include <iostream>

namespace CPU_RAYTRACER {
	
	class texture
	{	
		public:
		virtual ~texture() = default;
		virtual glm::vec3 value(float u, float v, const glm::vec3& p = glm::vec3(0,0,0)) const = 0;
	};

	class constant_texture : public texture
	{
		public:
		constant_texture(glm::vec3 c) : color(c) {}
		virtual glm::vec3 value(float u, float v, const glm::vec3& p) const override
		{
			return color;
		}
		glm::vec3 color;
	};

	class image_texture : public texture
	{
		public:
		image_texture(const char *filename)
		{
			img_data = stbi_load(filename, &image_width, &image_height, &components_per_pixel, 0);
			if (!img_data)
			{
				std::cerr << "ERROR: Could not load texture image file '" << filename << "'.\n";
				image_width = image_height = 0;
			}		
		}

		virtual glm::vec3 value(float u, float v, const glm::vec3& p) const override
		{
			// If we have no texture data, then return solid cyan as a debugging aid.
			if (img_data == nullptr)
				return glm::vec3(0,1,1);

			// Clamp input texture coordinates to [0,1] x [1,0]
			u = interval(0, 1).clamp(u);
			// v = 1.0 - interval(0, 1).clamp(v);  // Flip V to image coordinates
			v = interval(0, 1).clamp(v);  // don't flip V to image coordinates

			int i = static_cast<int>(u * image_width);
			int j = static_cast<int>(v * image_height);

			// Clamp integer mapping, since actual coordinates should be less than 1.0
			if (i >= image_width)  i = image_width - 1;
			if (j >= image_height) j = image_height - 1;

			const float color_scale = 1.0f / 255.0f;
			unsigned char* pixel = img_data + (j * image_width + i) * components_per_pixel;
			return glm::vec3(pixel[0], pixel[1], pixel[2]) * color_scale;
		}

		unsigned char* img_data;
		int image_width;
		int image_height;
		int components_per_pixel;


	};

}




#endif