#ifndef SKYBOX_H
#define SKYBOX_H


#include "utils.h"

#include "hittable.h"

#include <glm/gtc/quaternion.hpp> 
#include <glm/gtx/quaternion.hpp>



// this hittable is a skybox, it does not have a material, but it has a texture which need
// to be loaded from a folder containing 6 images, one for each face of the cube
class RT_Skybox{
public:
	RT_Skybox(const char* filepath) {
		// load skybox
		std::vector<std::string> faces
		{
			std::string(filepath) + "/right.jpg",
			std::string(filepath) + "/left.jpg",
			std::string(filepath) + "/top.jpg",
			std::string(filepath) + "/bottom.jpg",
			std::string(filepath) + "/front.jpg",
			std::string(filepath) + "/back.jpg"
		};

		for (unsigned int i = 0; i < faces.size(); i++)
		{
			data[i] = nullptr;
			data[i] = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
			if (data[i])
			{
				continue;
			}
			else
			{
				std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			}
		}

	};

	bool hit(const ray& r, interval ray_t, hit_record& rec) const {
		// we only care about the direction of the ray, not the origin
		glm::vec3 dir = r.direction();
		dir = glm::normalize(dir);
		// sample the texture with the direction of the ray
		// the texture is a cube map, so we need to map the direction to a face of the cube
		// the face is determined by the largest component of the direction vector
		int face = 0;
		glm::vec2 coords;
		glm::vec3 absDir = glm::abs(dir);
		if (absDir.x > absDir.y && absDir.x > absDir.z) {
			face = dir.x > 0 ? 0 : 1;
		}
		else if (absDir.y > absDir.z) {
			face = dir.y > 0 ? 2 : 3;
		}
		else {
			face = dir.z > 0 ? 4 : 5;
		}

		switch (face) {
		case 0:
			coords = glm::vec2(-dir.z, -dir.y) / abs(dir.x);
			break;
		case 1:
			coords = glm::vec2(dir.z, -dir.y) / abs(dir.x);
			break;
		case 2:
			coords = glm::vec2(dir.x, dir.z) / abs(dir.y);
			break;
		case 3:
			coords = glm::vec2(dir.x, -dir.z) / abs(dir.y);
			break;
		case 4:
			coords = glm::vec2(dir.x, -dir.y) / abs(dir.z);
			break;
		case 5:
			coords = glm::vec2(-dir.x, -dir.y) / abs(dir.z);
			break;
		}
		coords *= 0.5f;
		coords += 0.5f;
	
		// get the color from the texture
		glm::vec3 color = glm::vec3(data[face][int(coords.x * width) * nrChannels + int(coords.y * height) * width * nrChannels + 0],
			data[face][int(coords.x * width) * nrChannels + int(coords.y * height) * width * nrChannels + 1],
			data[face][int(coords.x * width) * nrChannels + int(coords.y * height) * width * nrChannels + 2]);
		// set the hit record
		rec.color = color/256.0f;
	
		return true;
	};

private:
	unsigned char* data[6];
	int width, height, nrChannels;




};




#endif // SKYBOX_H