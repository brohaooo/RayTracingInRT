#ifndef TEXTURE_H
#define TEXTURE_H

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#include <glm/glm.hpp>
#include <glad/glad.h>

#include <string>

class Texture {
public:
    int width, height;  // the size of the texture
    int channels;     // the number of channels in the texture
    unsigned char* data; // the pixel data

    Texture() : textureRef(0), width(0), height(0), data(nullptr), channels(0) {};

    bool loadFromFile(const std::string& filename){
        data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
        if (data == nullptr) {
            std::cerr << "Failed to load texture: " << filename << std::endl;
            return false;
        }
        return true;
    }

    void loadFromData(const int w, const int h, const int c, unsigned char* d){
        width = w;
        height = h;
        channels = c;
        data = d;
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
        glGenTextures(1, &textureRef);
        glBindTexture(GL_TEXTURE_2D, textureRef);
        // set the texture wrapping/filtering options (on the currently bound texture object)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // check if the data is loaded
        if (data == nullptr) {
            std::cerr << "No data loaded for texture" << std::endl;
            return;
        }
        // load and generate the texture
        if (channels == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
        else if (channels == 4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
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









#endif