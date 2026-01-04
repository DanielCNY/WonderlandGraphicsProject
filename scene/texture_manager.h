#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <string>
#include <unordered_map>
#include <glad/gl.h>

class TextureManager {
public:
    static TextureManager& getInstance();

    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    GLuint getTexture(const std::string& path);
    void cleanup();

private:
    TextureManager() = default;
    ~TextureManager() = default;

    GLuint loadTextureFromFile(const std::string& path);

    std::unordered_map<std::string, GLuint> textures;
};

#endif