#ifndef CPU_RAYTRACER_TRIANGLE_H
#define CPU_RAYTRACER_TRIANGLE_H


#include "utils.h"

#include "hittable.h"

#include <glm/gtc/quaternion.hpp> 
#include <glm/gtx/quaternion.hpp>

namespace CPU_RAYTRACER {
	class triangle : public hittable {
	public:
	triangle() {}
	
	triangle(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, shared_ptr<material> _material, glm::vec2 _uv0 = glm::vec2(0, 1), glm::vec2 _uv1 = glm::vec2(1, 1), glm::vec2 _uv2 = glm::vec2(0.5, 0), glm::vec3 _n0 = glm::vec3(0.0f), glm::vec3 _n1 = glm::vec3(0.0f), glm::vec3 _n2 = glm::vec3(0.0f) ):
		v0(v0), v1(v1), v2(v2), uv0(_uv0), uv1(_uv1), uv2(_uv2), mat(_material), n0(_n0), n1(_n1), n2(_n2)
	{
		glm::vec3 e1 = v1 - v0;
		glm::vec3 e2 = v2 - v0;
		face_normal = glm::normalize(glm::cross(e1, e2));
		face_normal = glm::normalize(face_normal);
		glm::vec3 min = glm::min(v0, glm::min(v1, v2));
		glm::vec3 max = glm::max(v0, glm::max(v1, v2));
		// 微调最小值和最大值，确保包围盒在所有轴上都有厚度
		const float epsilon = 1e-4f; // 微调量
		for (int i = 0; i < 3; ++i) {
			if (min[i] == max[i]) {
				min[i] -= epsilon;
				max[i] += epsilon;
			}
		}
		box = AABB(min, max);
		// normals need to be normalized
		if (n0 != glm::vec3(0.0f)) n0 = glm::normalize(n0);
		if (n1 != glm::vec3(0.0f)) n1 = glm::normalize(n1);
		if (n2 != glm::vec3(0.0f)) n2 = glm::normalize(n2);
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

        // compute t to find intersection point on the triangle
        t = inv_S1E1 * glm::dot(E2, S2);
		
		if (!ray_t.surrounds(t)) { //ensure t is in the interval of ray_t
			return false;
		}
		// determine whether use point normal or face normal
		glm::vec3 normal;
		if (n0 == glm::vec3(0.0f)) {// if n0 is 0, then use face normal
			normal = face_normal;
		}
		else {
			float b0 = 1 - b1 - b2;
			normal = glm::normalize(b0 * n0 + b1 * n1 + b2 * n2);
		}
		
		// interpolate the normal

		// enable this to make the triangle single sided (like rasterization)
		//if (!(dot(r.direction(), normal) < 0)) {
		//	return false;
		//}

		rec.t = t;
		rec.p = r.at(t);
		rec.mat = mat;
		rec.set_face_normal(r, normal);
		
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
	// point normals (if 0, then use face normal)
	glm::vec3 n0, n1, n2;


	shared_ptr<material> mat;
	AABB box;
	glm::vec3 face_normal;// use to determine if ray is hitting the front or back of the triangle, it is a rule defined by convention
	
	void get_triangle_uv(float b1, float b2, float& u, float& v) const {
		float b0 = 1 - b1 - b2;
		u = b0 * uv0.x + b1 * uv1.x + b2 * uv2.x;
		v = b0 * uv0.y + b1 * uv1.y + b2 * uv2.y;	
	}

};
}



#include "bvh.h"


namespace CPU_RAYTRACER{
// define a mesh class to store multiple triangles
// it takes a list of triangles and build a BVH tree automatically
// it also has a m matrix for transformation
class mesh : public hittable {
public:
	mesh(std::vector<shared_ptr<hittable>> _triangles, shared_ptr<material> _material, glm::mat4 _m = glm::mat4(1.0f))
		: triangles(_triangles), mat(_material), m(_m) {
		// we directly build a BVH tree for the triangles(without any transformation)
		bvh_node = std::make_shared<BVH_node>(triangles, 0, triangles.size());
		update_bounding_box();
		//box = AABB(glm::vec3(1e10f), glm::vec3(1e10f));
		//box.print();
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
		// first check if the ray hit the bounding box of the mesh
		if (!box.hit(r, ray_t)) {
			return false;
		}
		// transform the ray to local space
		glm::mat4 inv_m = glm::inverse(m);
		glm::vec3 origin_local = glm::vec3(inv_m * glm::vec4(r.origin(), 1.0f));
		glm::vec3 direction_local = glm::vec3(inv_m * glm::vec4(r.direction(), 0.0f));// 0.0f for direction
		// construct a new ray in local space
		ray r_local(origin_local, direction_local);
		//  do the hit test in local space
		if (bvh_node->hit(r_local, ray_t, rec)) {
			// if hit, transform the hit record back to world space
			// position and normal need to be transformed by the inverse transpose of the model matrix
			rec.p = glm::vec3(m * glm::vec4(rec.p, 1.0f));
			// normal need to be transformed by the inverse transpose of the model matrix
			// because the Model matrix may contain non-uniform scaling
			rec.normal = glm::normalize(glm::vec3(glm::transpose(inv_m) * glm::vec4(rec.normal, 0.0f)));
			rec.mat = mat; // material is the same in the mesh (we override the material of the triangles among the same mesh)
			return true;
		}

		return false;
	}

	AABB bounding_box() const override {
		return box;
	}
	
	void update_bounding_box() {
		// we need to transform the bounding box to world space
		// first get the bounding box in local space
		AABB box_local = bvh_node->bounding_box();
		// then transform the bounding box to world space
		box =  transform_bounding_box(box_local, m);
	}

private:
	std::vector<shared_ptr<hittable>> triangles; // the triangles of the mesh
	shared_ptr<material> mat;
	shared_ptr<BVH_node> bvh_node; // its contained triangles are not transformed (still in object space)
	glm::mat4 m; // model matrix, to transform the mesh to world space
	AABB box; // bounding box in world space, need to be updated when m is changed
	// helper function to transform the bounding box of BVH node to world space via matrix m
	AABB transform_bounding_box(const AABB& box, const glm::mat4& m) {
		std::vector<glm::vec3> vertices = {
			glm::vec3(box.min().x, box.min().y, box.min().z),
			glm::vec3(box.max().x, box.min().y, box.min().z),
			glm::vec3(box.min().x, box.max().y, box.min().z),
			glm::vec3(box.max().x, box.max().y, box.min().z),
			glm::vec3(box.min().x, box.min().y, box.max().z),
			glm::vec3(box.max().x, box.min().y, box.max().z),
			glm::vec3(box.min().x, box.max().y, box.max().z),
			glm::vec3(box.max().x, box.max().y, box.max().z)
		};

		glm::vec3 new_min = glm::vec3(m * glm::vec4(vertices[0], 1.0f));
		glm::vec3 new_max = new_min;

		for (const auto& vertex : vertices) {
			glm::vec3 transformed_vertex = glm::vec3(m * glm::vec4(vertex, 1.0f));
			new_min = glm::min(new_min, transformed_vertex);
			new_max = glm::max(new_max, transformed_vertex);
		}

		return AABB(new_min, new_max);
	}

};
}


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace CPU_RAYTRACER {
	// load a mesh from a file using assimp
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
}




#endif