#ifndef TEXTURE_H
#define TEXTURE_H

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#include <glm/glm.hpp>
#include <glad/glad.h>

#include <string>
#include <array>
#include <iostream>

class Texture {
public:
    int width, height;  // the size of the texture
    int channels;     // the number of channels in the texture
    unsigned char* data; // the pixel data
    GLenum dataFormat; // the format of the pixel data (in CPU)
    GLenum internalFormat; // the internal format of the texture (in GPU)
    

    Texture(GLenum _dataFormat = GL_UNSIGNED_BYTE, GLenum _internalFormat = GL_RGB) : textureRef(0), width(0), height(0), data(nullptr), channels(0) {
        internalFormat = _internalFormat;
        dataFormat = _dataFormat;
    };

    bool loadFromFile(const std::string& filename){
        data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
        if (data == nullptr) {
            std::cerr << "Failed to load texture: " << filename << std::endl;
            return false;
        }
        return true;
    }

    void setSize(const int w, const int h, const int c){
        width = w;
        height = h;
        channels = c;
    }

    void loadFromData(const int w, const int h, const int c, unsigned char* d){
        setSize(w, h, c);
        data = d;
    }

    void resizeTexture(const int newWidth, const int newHeight, const int newChannels){
        resizeData(newWidth, newHeight, newChannels);
        createGPUTexture();
    }

    void resizeData(const int newWidth, const int newHeight, const int newChannels){
        
        unsigned char* originalData = data;
        unsigned char* resizedData = (unsigned char*)malloc(newWidth * newHeight * newChannels);
        // check if the memory is allocated successfully
        if (resizedData == nullptr) {
            std::cerr << "Failed to allocate memory for resized texture" << std::endl;
            return;
        }
        stbir_resize_uint8_linear(  originalData, width, height, 0,
                                    resizedData, newWidth, newHeight, 0,
                                    static_cast<stbir_pixel_layout>(newChannels));
        // destroy the original data
        stbi_image_free(originalData);
        // update the data
        data = resizedData;
        width = newWidth;
        height = newHeight;
        channels = newChannels;
    }


    void createGPUTexture(){
        if (textureRef != 0) {
            glDeleteTextures(1, &textureRef);
            textureRef = 0;
        }
        // if the texture size is 0, then we do not create the texture
        if (width == 0 || height == 0) {
            std::cout << "Texture size is 0, not creating the texture" << std::endl;
            return;
        }
        glGenTextures(1, &textureRef);
        glBindTexture(GL_TEXTURE_2D, textureRef);
        // set the texture wrapping/filtering options (on the currently bound texture object)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // load and generate the texture
        if (channels == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_RGB, dataFormat, data);
        }
        else if (channels == 4) {
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_RGBA, dataFormat, data);
        }
        else {
            std::cerr << "Unsupported number of channels: " << channels << std::endl;
        }
    };

    void updateGPUTexture(){
        glBindTexture(GL_TEXTURE_2D, textureRef);
        if (channels == 3) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
        else if (channels == 4) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        else {
            std::cerr << "Unsupported number of channels: " << channels << std::endl;
        }
        
    };

    void destroy(){
        if (textureRef != 0) {
            glDeleteTextures(1, &textureRef);
            textureRef = 0;
        }
        if (data != nullptr) {
            stbi_image_free(data);
            data = nullptr;
        }
    };

    unsigned int getTextureRef() const { return textureRef; }

    ~Texture() { destroy(); }

private:
    unsigned int textureRef;    // OpenGL texture ID reference
    // not copyable
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
};

// skybox texture, it has 6 faces, each face is a texture
class SkyboxTexture {
    public:
    std::array<Texture, 6> faces;

    SkyboxTexture(){
        textureRef = 0;
    };

    bool loadFromFolder(const std::string & folderPath){
        std::vector<std::string> faces_path
		{
			folderPath + std::string("/right.jpg"),
			folderPath + std::string("/left.jpg"),
			folderPath + std::string("/top.jpg"),
			folderPath + std::string("/bottom.jpg"),
			folderPath + std::string("/front.jpg"),
			folderPath + std::string("/back.jpg")
		};
        // we need to load each face (only load the texture in CPU, not create the GPU texture resource)
        for (int i = 0; i < 6; i++) {
            if (!faces[i].loadFromFile(faces_path[i])) {
                std::cout << "Failed to load face: " << faces_path[i] << std::endl;
                return false;
            }
        }
        return true;
    };
    void createSkyboxTexture(){
        glGenTextures(1, &textureRef);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureRef);
        for (unsigned int i = 0; i < faces.size(); i++) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, faces[i].width, faces[i].height, 0, GL_RGB, GL_UNSIGNED_BYTE, faces[i].data);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    };
    void destroy(){
        for (auto& face : faces) {
            face.~Texture();
        }

    };

    unsigned int getTextureRef() const { return textureRef; }

    ~SkyboxTexture() { destroy(); }

    private:
    unsigned int textureRef;
    // not copyable
    SkyboxTexture(const SkyboxTexture&) = delete;
    SkyboxTexture& operator=(const SkyboxTexture&) = delete;
};








#endif