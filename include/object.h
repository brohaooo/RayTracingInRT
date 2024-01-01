#ifndef OBJECT_H
#define OBJECT_H

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <shader.h>

// 渲染上下文，包含渲染所需的所有数据
struct RenderContext {
public:
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
	// 其他可能的渲染数据...
};


class Object {
public:
	// 物体的数据
	GLuint VAO;
	GLuint VBO;
	GLuint texture;
	Shader *shader;
	bool hasTexture = false;


	// 绘制方法
	virtual void draw() = 0;
	// 准备绘制，比如MVP类模型需要在绘制前计算MVP矩阵
	virtual void prepareDraw(const RenderContext& context) {
		// 默认实现或空实现
	}
	virtual void setShader(Shader* _shader) {
		shader = _shader;
	}
	virtual void Delete() = 0;

	virtual void setTexture(const unsigned char* texture_data, int image_width, int image_height, int channels) {

		hasTexture = true;
		glGenTextures(1, &texture);
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

	virtual void setTexture(const char* filename) {
		hasTexture = true;
		glGenTextures(1, &texture);
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
			glBindBuffer(GL_ARRAY_BUFFER, VBO); // 绑定VBO
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // 将顶点数据复制到缓冲中
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0); // 设置顶点属性指针
			glEnableVertexAttribArray(0); // 启用顶点属性
			glBindBuffer(GL_ARRAY_BUFFER, 0); // 解绑VBO
			glBindVertexArray(0); // 解绑VAO
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
		// 使用 context 中的视图和投影矩阵
		view = context.viewMatrix;
		projection = context.projectionMatrix;
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
		glBindBuffer(GL_ARRAY_BUFFER, VBO); // 绑定VBO
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * sphereVertices.size(), sphereVertices.data(), GL_STATIC_DRAW); // 将顶点数据复制到缓冲中
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); // 设置顶点属性指针: 位置属性
		glEnableVertexAttribArray(0); // 启用顶点属性
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); // 设置顶点属性指针: 纹理坐标属性
		glEnableVertexAttribArray(1); // 启用顶点属性
		glBindBuffer(GL_ARRAY_BUFFER, 0); // 解绑VBO

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); // 绑定EBO
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * sphereIndices.size(), sphereIndices.data(), GL_STATIC_DRAW); // 将顶点数据复制到缓冲中


		glBindVertexArray(0); // 解绑VAO
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
Triangle(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2){
	GLfloat vertices[] =
	{
		// Positions    // uv
		v0.x, v0.y, v0.z,   0.0f, 1.0f, // left top
		v1.x, v1.y, v1.z,   1.0f, 0.0f, // right bottom
		v2.x, v2.y, v2.z,   0.0f, 0.0f, // left bottom
	};

		//GLfloat vertices[] =
		//{
		//	// Positions    // uv
		//	
		//	-0.5f, 2.0f, 0.0f,   0.0f, 1.0f, // left top
		//	0.5f, 2.0f, 0.0f,   1.0f, 0.0f, // right bottom
		//	0.0f, 3.0f, 0.0f,   0.0f, 0.0f, // left bottom
		//	 
		//};

		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO); // 绑定VBO
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // 将顶点数据复制到缓冲中
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); // 设置顶点属性指针
		glEnableVertexAttribArray(0); // 启用顶点属性
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); // 设置顶点属性指针
		glEnableVertexAttribArray(1); // 启用顶点属性
		glBindBuffer(GL_ARRAY_BUFFER, 0); // 解绑VBO
		glBindVertexArray(0); // 解绑VAO
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


#endif