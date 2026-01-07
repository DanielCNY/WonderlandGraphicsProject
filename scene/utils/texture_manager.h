#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H
#include <string>
#include <unordered_map>
#include <glad/gl.h>

class TextureManager {
public:
    static TextureManager& getInstance();

    GLuint getTexture(const std::string& path);

private:
    TextureManager() = default;

    GLuint loadTextureFromFile(const std::string& path);

    std::unordered_map<std::string, GLuint> textures;
};

#endif