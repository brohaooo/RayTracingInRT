#ifndef OBJECT_H
#define OBJECT_H

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <Shader.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <Texture.h>

// render context, contains view matrix, projection matrix, camera position, etc.
// not used currently, we use ubo to pass vp matrix
struct RenderContext {
public:
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
	glm::vec3 cameraPos;
};


class Object {
public:
	// render related opengl objects references
	GLuint VAO;
	GLuint VBO;
	Shader *shader;
	bool hasTexture = false;
	Texture * texture;


	// draw function, to be implemented by derived classes
	virtual void draw() = 0;
	// do before draw, to be implemented by derived classes
	// since we use ubo to pass vp matrix, this function is not useful currently
	virtual void prepareDraw(const RenderContext& context) {
		// default implementation: do nothing
	}
	virtual void setShader(Shader* _shader) {
		shader = _shader;
	}
	virtual void Delete() = 0;

	// load texture from a const unsigned char* data, used for loading texture from memory
	virtual void setTexture(Texture * _texture) {
		hasTexture = true;
		this->texture = _texture;
	}
};

class ScreenSpaceObject : public Object {
};



class Rect : public ScreenSpaceObject {
	public:
		Rect() {
			GLfloat vertices[] =
			{
				// Positions    	// uv
				-1.0f,  1.0f,   	0.0f, 1.0f, // left top
				-1.0f, 	-1.0f,   	0.0f, 0.0f, // left bottom
				1.0f, 	-1.0f,    	1.0f, 0.0f, // right bottom

				-1.0f, 	1.0f,  		0.0f, 1.0f, // left top
				1.0f, 	-1.0f,   	1.0f, 0.0f, // right bottom
				1.0f,  	1.0f,   	1.0f, 1.0f  // right top
			};

			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO); // bind VBO
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // copy the vertex data to VBO
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0); // set the vertex attribute pointers
			glEnableVertexAttribArray(0); // activate the vertex attribute
			glBindBuffer(GL_ARRAY_BUFFER, 0); // unbund VBO
			glBindVertexArray(0); // unbund VAO
		}


		void Delete() override {
			glDeleteVertexArrays(1, &VAO);
			glDeleteBuffers(1, &VBO);
		};

		void prepareDraw(const RenderContext& context) override {
			
		}

		void draw() override {
			shader->use();
			if (hasTexture) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, texture->getTextureRef());
				shader->setInt("texture1", 0);
			}	
			glBindVertexArray(VAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glBindVertexArray(0);
		}
};

class MVPObject : public Object {
public:
	glm::mat4 model;
	glm::vec4 color;
	void prepareDraw(const RenderContext& context) override {
		// not used currently, we use ubo to pass vp matrix
	}
	virtual void setModel(glm::mat4 _model) {
		model = _model;
	}
	virtual void setColor(glm::vec4 _color) {
		color = _color;
	}
};


// the sphere object's VBO data is its object space vertex data
// by default, the sphere is at the origin, with radius 1
class Sphere : public MVPObject {
public:
	GLuint EBO;
	float radius;
	glm::vec3 center;
	Sphere() {
		radius = 1.0f;
		center = glm::vec3(0.0f, 0.0f, 0.0f);
		generateBufferResource();
	}
	// another initialization function, set the center and radius of the sphere
	Sphere(glm::vec3 _center, float _radius) {
		center = _center;
		radius = _radius;
		generateBufferResource();
	};

	void Delete() override {
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &EBO);
	};


	void draw() override {
		shader->use();
		if (hasTexture) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture->getTextureRef());
			shader->setInt("texture1", 0);
		}
		shader->setMat4("model", model);
		shader->setVec4("color", color);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 10800, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}
	

	void generateBufferResource(){
		std::vector<GLfloat> sphereVertices;
		int sectors = 60;
		int stacks = 30;
		int num = (stacks * 2) * sectors * 3;
		for (int i = 0; i <= stacks; ++i) {
			float stackAngle = glm::pi<float>() / 2.0f - i * glm::pi<float>() / stacks;
			float y = radius * sin(stackAngle);
			for (int j = 0; j <= sectors; ++j) {
				float sectorAngle = 2.0f * glm::pi<float>() * j / sectors;
				float x = radius * cos(stackAngle) * cos(sectorAngle);
				float z = radius * cos(stackAngle) * sin(sectorAngle);

				// position
				sphereVertices.push_back(x+center.x);
				sphereVertices.push_back(y+center.y);
				sphereVertices.push_back(z+center.z);

				// normal
				glm::vec3 normal = glm::normalize(glm::vec3(x, y, z));
				sphereVertices.push_back(normal.x);
				sphereVertices.push_back(normal.y);
				sphereVertices.push_back(normal.z);
				

				// uv
				sphereVertices.push_back((float)j / sectors);
				sphereVertices.push_back((float)i / stacks);
			}
		}
		std::vector<GLuint> sphereIndices;
		for (int i = 0; i < stacks; ++i) {
			for (int j = 0; j < sectors; ++j) {
				int top = i * (sectors + 1) + j;
				int bottom = top + sectors + 1;

				sphereIndices.push_back(top);
				sphereIndices.push_back(top + 1);
				sphereIndices.push_back(bottom);

				sphereIndices.push_back(bottom);
				sphereIndices.push_back(top + 1);
				sphereIndices.push_back(bottom + 1);
			}
		}
		glGenBuffers(1, &EBO);
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO); 
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * sphereVertices.size(), sphereVertices.data(), GL_STATIC_DRAW); 
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); // set vertex attribute pointers: position
		glEnableVertexAttribArray(0); // activate vertex attribute
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); // set vertex attribute pointers: normal
		glEnableVertexAttribArray(1); // activate vertex attribute
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); // set vertex attribute pointers: uv
		glEnableVertexAttribArray(2); // activate vertex attribute
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); // bind EBO
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * sphereIndices.size(), sphereIndices.data(), GL_STATIC_DRAW); // copy the index data to EBO
		glBindVertexArray(0);
	}
};


class TransparantSphere : public Sphere {

public:
	float transparancy = 0.5;
	void setTransparancy(float _transparancy) {
		if (_transparancy > 1.0) {
			_transparancy = 1.0;
		}
		else if (_transparancy < 0.0) {
			_transparancy = 0.0;
		}
		transparancy = _transparancy;
	}

	void draw() override {
		shader->use();
		if (hasTexture) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture->getTextureRef());
			shader->setInt("texture1", 0);
		}
		shader->setMat4("model", model);
		shader->setVec4("color", color);
		shader->setFloat("transparancy", transparancy);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 10800, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}
};




// a single triangle
class Triangle : public MVPObject {
public:
glm::vec3 v0,v1,v2;
glm::vec3 n0,n1,n2;
glm::vec2 t0,t1,t2;

	Triangle(glm::vec3 _v0, glm::vec3 _v1, glm::vec3 _v2):v0(_v0),v1(_v1),v2(_v2)
	{
		// a triangle has only one normal(if it is just a single triangle)
		n0 = n1 = n2 = glm::normalize(glm::cross(v1 - v0, v2 - v0));
		// default uv
		t0 = glm::vec2(0, 1);
		t1 = glm::vec2(1, 1);
		t2 = glm::vec2(0.5, 0);
		generateBufferResource();
	};
	Triangle(glm::vec3 _v0, glm::vec3 _v1, glm::vec3 _v2, glm::vec3 _t0, glm::vec3 _t1, glm::vec3 _t2):v0(_v0),v1(_v1),v2(_v2),t0(_t0),t1(_t1),t2(_t2)
	{
		
		n0 = n1 = n2 = glm::normalize(glm::cross(v1 - v0, v2 - v0));
		generateBufferResource();
	};

	void Delete() override {
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
	};

	void draw() override {
		shader->use();
		if (hasTexture) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture->getTextureRef());
			shader->setInt("texture1", 0);
		}
		shader->setMat4("model", model);
		shader->setVec4("color", color);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}

	void generateBufferResource(){
		GLfloat vertices[] =
		{
			// Positions    	// normals			// uv
			v0.x, v0.y, v0.z, 	n0.x, n0.y, n0.z,	t0.x, t0.y,   // left bottom
			v1.x, v1.y, v1.z, 	n1.x, n1.y, n1.z,	t1.x, t1.y,   // right bottom
			v2.x, v2.y, v2.z,	n2.x, n2.y, n2.z,	t2.x, t2.y,    // right top 
			// the back side
			v0.x, v0.y, v0.z,	n0.x, n0.y, n0.z,	t0.x, t0.y,   // left bottom 
			v2.x, v2.y, v2.z,	n2.x, n2.y, n2.z,	t2.x, t2.y,   // right top 
			v1.x, v1.y, v1.z,	n1.x, n1.y, n1.z,	t1.x, t1.y    // right bottom 
		};
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO); 
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); 
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); // position
		glEnableVertexAttribArray(0); 
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); // normal
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); // uv
		glEnableVertexAttribArray(2); 
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0); 
	}

};

// a skybox cube, doesn't need M matrix, has its own shader, has cube texture
class Skybox : public MVPObject {
public:

	SkyboxTexture * skyboxTexture;
	Skybox() {
		GLfloat skyboxVertices[] = {
			// positions          
			-1.0f,  1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			-1.0f,  1.0f, -1.0f,
			 1.0f,  1.0f, -1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			 1.0f, -1.0f,  1.0f
		};

		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO); 
		glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW); 
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);

	};

	void setTexture(SkyboxTexture * _skyboxTexture) {
		hasTexture = true;
		this->skyboxTexture = _skyboxTexture;
	}


	void Delete() override {
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
	};

	void draw() override {
		glDepthFunc(GL_LEQUAL);
		shader->use();
		if (!hasTexture) {
			std::cerr<<"ERROR: skybox has no texture"<<std::endl;
			return;
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture->getTextureRef());
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
		glDepthFunc(GL_LESS);
	}

private:
	

};

// mesh object, loaded from file via assimp
class Mesh : public MVPObject {
	public:
	// mesh data
	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> uvs;
	std::vector<GLuint> indices;

	// tmp implementation:
	glm::mat4 rotation = glm::mat4(1.0f);

	// render data
	// same as any other object, we have a VAO, VBO
	// the only difference is that we have a EBO now
	GLuint EBO;
	
	

	// constructor
	Mesh(aiMesh* mesh)
	{
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			glm::vec3 position;
			position.x = mesh->mVertices[i].x;
			position.y = mesh->mVertices[i].y;
			position.z = mesh->mVertices[i].z;
			positions.push_back(position);	
		}
		if (mesh->mNormals) 
		{
			for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			{
				glm::vec3 normal;
				normal.x = mesh->mNormals[i].x;
				normal.y = mesh->mNormals[i].y;
				normal.z = mesh->mNormals[i].z;
				normals.push_back(normal);
			}
		}
		if (mesh->mTextureCoords) 
		{
			for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			{
				glm::vec2 uv;
				uv.x = mesh->mTextureCoords[0][i].x;
				uv.y = mesh->mTextureCoords[0][i].y;
				uvs.push_back(uv);
			}
		}
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}

		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);
		// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
		glBindVertexArray(VAO);

		unsigned int vertex_size = 0;
		// position attribute
		vertex_size += sizeof(glm::vec3);
		// normal attribute
		if (normals.size() == 0) {
			generateSmoothNormals();
			//generateNormal();	
		}
		vertex_size += sizeof(glm::vec3);
		if (uvs.size() > 0) {
			vertex_size += sizeof(glm::vec2);
		}

		// combine all the data into a single array as interleaved array, then upload it into the VBO
		std::vector<float> interleaved_data;
		for (unsigned int i = 0; i < positions.size(); i++) {
			interleaved_data.push_back(positions[i].x);
			interleaved_data.push_back(positions[i].y);
			interleaved_data.push_back(positions[i].z);
			interleaved_data.push_back(normals[i].x);
			interleaved_data.push_back(normals[i].y);
			interleaved_data.push_back(normals[i].z);
			if (uvs.size() > 0) {
				interleaved_data.push_back(uvs[i].x);
				interleaved_data.push_back(uvs[i].y);
			}
		}
		
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, interleaved_data.size() * sizeof(float), &interleaved_data[0], GL_STATIC_DRAW);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

		// we specify the layout of the vertex data: position, normal(force exist by generating), uv(might not exist)
		// position attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertex_size, (void*)0);
		glEnableVertexAttribArray(0);
		// normal attribute
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertex_size, (void*)sizeof(glm::vec3));
		glEnableVertexAttribArray(1);
		// uv attribute
		if (uvs.size() > 0) {		
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertex_size, (void*)(2*sizeof(glm::vec3)));
			glEnableVertexAttribArray(2);		
		}
	
		glBindVertexArray(0);
	}

	// only load the first mesh in the file, not recommended, use Model class instead
	Mesh(const char* filename) {
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_FlipUVs);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
			return;
		}
		aiMesh* mesh = scene->mMeshes[0];

		*this = Mesh(mesh);
	}





	// if we don't have normal, we can generate it
	void generateNormal() {
		if (normals.size() == 0) {
			normals.resize(positions.size(), glm::vec3(0.0f, 0.0f, 0.0f));
			for (unsigned int i = 0; i < indices.size(); i += 3) {
				glm::vec3 v0 = positions[indices[i]];
				glm::vec3 v1 = positions[indices[i + 1]];
				glm::vec3 v2 = positions[indices[i + 2]];
				glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
				normals[indices[i]] = normal;
				normals[indices[i + 1]] = normal;
				normals[indices[i + 2]] = normal;
				
			}
		}
	}
	// better way to generate normal, smooth normal
	void generateSmoothNormals() {
		normals.resize(positions.size(), glm::vec3(0.0f, 0.0f, 0.0f));

		// add all the normals of the faces to the vertices
		for (unsigned int i = 0; i < indices.size(); i += 3) {
			glm::vec3 v0 = positions[indices[i]];
			glm::vec3 v1 = positions[indices[i + 1]];
			glm::vec3 v2 = positions[indices[i + 2]];

			glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

			normals[indices[i]] += normal;
			normals[indices[i + 1]] += normal;
			normals[indices[i + 2]] += normal;
		}

		// then normalize the normals
		for (glm::vec3& normal : normals) {
			normal = glm::normalize(normal);
		}
	}


	void prepareDraw(const RenderContext& context) override {
		
	}

	// render the mesh
	void draw() override
	{
		shader->use();
		if (hasTexture) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture->getTextureRef());
			shader->setInt("texture1", 0);
		}
		shader->setMat4("model", model);
		shader->setMat4("rotation", rotation); // tmp implementation
		shader->setVec4("color", color); // set the color
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		
	}

	void Delete() override
	{
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &EBO);
	}



};



// model object, contains multiple meshes and textures
class Model : public MVPObject {
	public:
	std::vector<Mesh*> meshes;
	Model(std::string const& path)
	{
		Assimp::Importer importer;
		// noticing that we use aiProcessPreset_TargetRealtime_Quality, which is a combination of multiple flags
		// it will generate smooth normals, flip uv, and triangulate the mesh
		const aiScene* scene = importer.ReadFile(path, aiProcessPreset_TargetRealtime_Quality | aiProcess_PreTransformVertices);
		//const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
			return;
		}
		for (unsigned int i = 0; i < scene->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[i];
			Mesh *m = new Mesh(mesh);
			meshes.push_back(m);
		}

		std::cout<<"model loaded, mesh count: "<< meshes.size() <<std::endl;
		
	}

	void setColor(glm::vec4 _color) override{
		color = _color;
		for (unsigned int i = 0; i < meshes.size(); i++)
			meshes[i]->color = _color;
	}

	void draw() override
	{
		for (unsigned int i = 0; i < meshes.size(); i++)
			meshes[i]->draw();
	}

	void Delete() override
	{
		for (unsigned int i = 0; i < meshes.size(); i++)
			meshes[i]->Delete();
		// then delete the meshes
		for (unsigned int i = 0; i < meshes.size(); i++)
			delete meshes[i];
	}

	void setTexture(Texture * _texture) override{
		for (unsigned int i = 0; i < meshes.size(); i++)
			meshes[i]->setTexture(_texture);
	}

	void setShader(Shader* _shader) override{
		shader = _shader;
		for (unsigned int i = 0; i < meshes.size(); i++)
			meshes[i]->setShader(_shader);
	}

	void setModel(glm::mat4 _model) override {
		this->model = _model;
		for (unsigned int i = 0; i < meshes.size(); i++)
			meshes[i]->setModel(_model);
	}

	void prepareDraw(const RenderContext& context) override {
		
		for (unsigned int i = 0; i < meshes.size(); i++)
			meshes[i]->prepareDraw(context);
	}


};



#endif