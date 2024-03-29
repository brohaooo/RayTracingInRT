#ifndef OBJECT_H
#define OBJECT_H

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <Shader.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// render context, contains view matrix, projection matrix, camera position, etc.
// not used currently, we use ubo to pass vp matrix
struct RenderContext {
public:
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
	glm::vec3 cameraPos;
	// other things like light position, etc.
	bool flipYCoord;
};


class Object {
public:
	// render related opengl objects references
	GLuint VAO;
	GLuint VBO;
	GLuint texture;
	Shader *shader;
	bool hasTexture = false;


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
	virtual void setTexture(const unsigned char* texture_data, int image_width, int image_height, int channels) {
		// if the object doesn't have a texture, create one
		if (!hasTexture) {
			glGenTextures(1, &texture);
		}
		hasTexture = true;
		glBindTexture(GL_TEXTURE_2D, texture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if (channels == 3) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture_data);
		}
		else if (channels == 4) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);
		}
		
	}
	// load texture from a file
	virtual void setTexture(const char* filename) {
		// if the object doesn't have a texture, create one
		if (!hasTexture) {
			glGenTextures(1, &texture);
		}
		//std::cout << texture << std::endl;
		hasTexture = true;
		glBindTexture(GL_TEXTURE_2D, texture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		int image_width, image_height, components_per_pixel;
		unsigned char* img_data = stbi_load(filename, &image_width, &image_height, &components_per_pixel, 0);
		if (!img_data)
		{
			std::cerr << "ERROR: Could not load texture image file '" << filename << "'.\n";
			image_width = image_height = 0;
		}

		if (components_per_pixel == 3) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0, GL_RGB, GL_UNSIGNED_BYTE, img_data);
		}
		else if (components_per_pixel == 4) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img_data);
		}
		else {
			std::cerr << "ERROR: unknown channel num '" << filename << "channel:" << components_per_pixel <<"'.\n";
			image_width = image_height = 0;
		}
			
	}
	// set texture from a texture id (it takes the texture id from another object)
	virtual void setTexture(GLuint _texture) {
		hasTexture = true;
		texture = _texture;
	}



	virtual void updateTexture(const unsigned char* texture_data, int image_width, int image_height, int channels) {
		glBindTexture(GL_TEXTURE_2D, texture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
		if (channels == 3) {
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image_width, image_height, GL_RGB, GL_UNSIGNED_BYTE, texture_data);
		}
		else if (channels == 4) {
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image_width, image_height, GL_RGBA, GL_UNSIGNED_BYTE, texture_data);
		}
		else {
			std::cerr << "ERROR: unknown channel num '" << channels << "'.\n";
		}
		
	}
};

class ScreenSpaceObject : public Object {
};



class Rect : public ScreenSpaceObject {
	public:
		Rect() {
			GLfloat vertices[] =
			{
				// Positions    // uv
				-1.0f,  1.0f,   0.0f, 1.0f, // left top
				-1.0f, -1.0f,   0.0f, 0.0f, // left bottom
					1.0f, -1.0f,   1.0f, 0.0f, // right bottom

					-1.0f,  1.0f,  0.0f, 1.0f, // left top
					1.0f, -1.0f,  1.0f, 0.0f, // right bottom
					1.0f,  1.0f,  1.0f, 1.0f  // right top
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
			shader->use();
			shader->setBool("flipYCoord", context.flipYCoord);
		}

		void draw() override {
			shader->use();
			if (hasTexture) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, texture);
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
	glm::mat4 view;
	glm::mat4 projection;
	void prepareDraw(const RenderContext& context) override {
		// not used currently, we use ubo to pass vp matrix
	}
	void setModel(glm::mat4 _model) {
		model = _model;
	}
};


class Sphere : public MVPObject {
public:
	glm::vec4 color;
	GLuint EBO;
	Sphere() {
		std::vector<GLfloat> sphereVertices;
		float radius = 1.0f;
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

				// 位置
				sphereVertices.push_back(x);
				sphereVertices.push_back(y);
				sphereVertices.push_back(z);

				// 纹理坐标
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
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); // set vertex attribute pointers: position
		glEnableVertexAttribArray(0); // activate vertex attribute
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); // set vertex attribute pointers: uv
		glEnableVertexAttribArray(1); // activate vertex attribute
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); // bind EBO
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * sphereIndices.size(), sphereIndices.data(), GL_STATIC_DRAW); // copy the index data to EBO


		glBindVertexArray(0);
	}


	void setColor(glm::vec4 _color) {
		color = _color;
	}

	void Delete() override {
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &EBO);
	};


	void draw() override {
		shader->use();
		if (hasTexture) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture);
			shader->setInt("texture1", 0);
		}
		shader->setMat4("model", model);
		shader->setMat4("view", view);
		shader->setMat4("projection", projection);
		shader->setVec4("color", color);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 10800, GL_UNSIGNED_INT, 0);
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
			glBindTexture(GL_TEXTURE_2D, texture);
			shader->setInt("texture1", 0);
		}
		shader->setMat4("model", model);
		shader->setMat4("view", view);
		shader->setMat4("projection", projection);
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
glm::vec4 color;
glm::vec3 v0,v1,v2;
glm::vec3 n1,n2,n3;

Triangle(glm::vec3 _v0, glm::vec3 _v1, glm::vec3 _v2):v0(_v0),v1(_v1),v2(_v2)
{
	// a triangle has only one normal(if it is just a single triangle)
	n1 = n2 = n3 = glm::normalize(glm::cross(v1 - v0, v2 - v0));

	GLfloat vertices[] =
	{
		// Positions    // uv
		_v0.x, _v0.y, _v0.z,   0.0f, 1.0f, // left bottom
		_v1.x, _v1.y, _v1.z,   1.0f, 1.0f, // right bottom
		_v2.x, _v2.y, _v2.z,   0.5f, 0.0f, // mid top
	};

		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO); 
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); 
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); 
		glEnableVertexAttribArray(0); 
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); 
		glEnableVertexAttribArray(1); 
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0); 
	}

	void setColor(glm::vec4 _color) {
		color = _color;
	}

	void Delete() override {
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
	};

	void draw() override {
		shader->use();
		if (hasTexture) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture);
			shader->setInt("texture1", 0);
		}
		shader->setMat4("model", model);
		shader->setMat4("view", view);
		shader->setMat4("projection", projection);
		shader->setVec4("color", color);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindVertexArray(0);
	}


};

// a skybox cube, doesn't need M matrix, has its own shader, has cube texture
class Skybox : public MVPObject {
public:
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

	// this will now load 6 textures into a cubemap, the texture file need to have names of: back, front, top, bottom, left, right
	void setTexture(const char* filepath) override {
		hasTexture = true;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

		std::vector<std::string> faces
		{
			filepath + std::string("/right.jpg"),
			filepath + std::string("/left.jpg"),
			filepath + std::string("/top.jpg"),
			filepath + std::string("/bottom.jpg"),
			filepath + std::string("/front.jpg"),
			filepath + std::string("/back.jpg")
		};


		int width, height, nrChannels;
		for (unsigned int i = 0; i < faces.size(); i++)
		{
			unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
			if (data)
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
				stbi_image_free(data);
			}
			else
			{
				std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
				stbi_image_free(data);
			}
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	};



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
		glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
		//shader->setMat4("model", model);
		shader->setMat4("view", view);
		shader->setMat4("projection", projection);
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
			glBindTexture(GL_TEXTURE_2D, texture);
			shader->setInt("texture1", 0);
		}
		shader->setMat4("model", model);
		shader->setMat4("rotation", rotation); // tmp implementation
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

	void setTexture(const char* filename) override{
		for (unsigned int i = 0; i < meshes.size(); i++)
			meshes[i]->setTexture(filename);
	}

	void setShader(Shader* _shader) override{
		shader = _shader;
		for (unsigned int i = 0; i < meshes.size(); i++)
			meshes[i]->setShader(_shader);
	}

	void setModel(glm::mat4 _model) {
		this->model = _model;
		for (unsigned int i = 0; i < meshes.size(); i++)
			meshes[i]->setModel(_model);
	}

	void prepareDraw(const RenderContext& context) override {
		
		for (unsigned int i = 0; i < meshes.size(); i++)
			meshes[i]->prepareDraw(context);
	}

	void updateRotation(glm::mat4 _rotation) {
		for (unsigned int i = 0; i < meshes.size(); i++)
			meshes[i]->rotation = _rotation;
	}


};



#endif