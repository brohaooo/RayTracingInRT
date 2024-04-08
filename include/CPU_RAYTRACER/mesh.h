#ifndef CPU_RAYTRACER_MESH_H
#define CPU_RAYTRACER_MESH_H


#include "utils.h"
#include "hittable.h"

#include "bvh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace CPU_RAYTRACER {
    // define a mesh class to store multiple triangles
    // it takes a list of triangles and build a BVH tree automatically
    class mesh : public hittable {
    public:
    	mesh(std::vector<shared_ptr<hittable>> _triangles, shared_ptr<material> _material = nullptr)
    		: triangles(_triangles), mat(_material) {
    		// we directly build a BVH tree for the triangles(without any transformation)
    		bvh_node = std::make_shared<BVH_node>(triangles, 0, triangles.size());
    	}

    	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
    		//  do the hit test in local space
    		return bvh_node->hit(r, ray_t, rec);
    	}

    	AABB bounding_box() const override {
    		return bvh_node->bounding_box();
    	}

    private:
    	std::vector<shared_ptr<hittable>> triangles; // the triangles of the mesh
    	shared_ptr<material> mat = nullptr; // if it is not null, it will override the material of the object
    	shared_ptr<BVH_node> bvh_node; // its contained triangles are not transformed (still in object space)
    };
    
	
	// load a mesh from a file using assimp
	// legacy function, not recommended to use
	// it loads an obj, merge all meshes triangles into a single std::vector<shared_ptr<hittable>>
	// then you can build a mesh object from the triangles using the mesh constructor
	// we will load the UV and normals of the vertices
	std::vector<shared_ptr<hittable>> load_mesh(const std::string& filename, shared_ptr<material> mat) {
		std::vector<shared_ptr<hittable>> triangles;
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(filename, aiProcessPreset_TargetRealtime_Quality | aiProcess_PreTransformVertices);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
			return triangles;
		}
		for (unsigned int i = 0; i < scene->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[i];
			for (unsigned int j = 0; j < mesh->mNumFaces; j++)
			{
				aiFace face = mesh->mFaces[j];
				if (face.mNumIndices != 3) {
					std::cerr << "ERROR::ASSIMP::face.mNumIndices != 3" << std::endl;
					return triangles;
				}
				aiVector3D v0 = mesh->mVertices[face.mIndices[0]];
				aiVector3D v1 = mesh->mVertices[face.mIndices[1]];
				aiVector3D v2 = mesh->mVertices[face.mIndices[2]];
				aiVector3D uv0 = mesh->mTextureCoords[0][face.mIndices[0]];
				aiVector3D uv1 = mesh->mTextureCoords[0][face.mIndices[1]];
				aiVector3D uv2 = mesh->mTextureCoords[0][face.mIndices[2]];
				aiVector3D n0 = mesh->mNormals[face.mIndices[0]];
				aiVector3D n1 = mesh->mNormals[face.mIndices[1]];
				aiVector3D n2 = mesh->mNormals[face.mIndices[2]];
				triangles.push_back(std::make_shared<triangle>(
					glm::vec3(v0.x, v0.y, v0.z),
					glm::vec3(v1.x, v1.y, v1.z),
					glm::vec3(v2.x, v2.y, v2.z),
					mat,
					glm::vec2(uv0.x, uv0.y),
					glm::vec2(uv1.x, uv1.y),
					glm::vec2(uv2.x, uv2.y),
					glm::vec3(n0.x, n0.y, n0.z),
					glm::vec3(n1.x, n1.y, n1.z),
					glm::vec3(n2.x, n2.y, n2.z)
				));

			}
		}


		return triangles;
	}

	// or load these triangles from a list of vertices and indices into a std::vector<shared_ptr<hittable>>
	// this should be the correct way to load a mesh, since CPU_RAYTRACER should rely on the user to provide the vertices and indices
	void load_triangles(std::vector<shared_ptr<hittable>> & triangles, const std::vector<glm::vec3>& vertices, const std::vector<glm::vec2>& uvs, const std::vector<glm::vec3>& normals, const std::vector<unsigned int>& indices, shared_ptr<material> mat) {
		for (unsigned int i = 0; i < indices.size()-2; i += 3)
		{
			triangles.push_back(std::make_shared<triangle>(
				vertices[indices[i]],
				vertices[indices[i + 1]],
				vertices[indices[i + 2]],
				mat,
				uvs[indices[i]],
				uvs[indices[i + 1]],
				uvs[indices[i + 2]],
				normals[indices[i]],
				normals[indices[i + 1]],
				normals[indices[i + 2]]
			));
		}
	}
	



}

















#endif