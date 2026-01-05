#ifndef STATIC_MODEL_H
#define STATIC_MODEL_H

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include <string>

class StaticModel {
public:
    StaticModel();
    ~StaticModel();

    bool loadModel(const char* filename);
    void render(const glm::mat4& cameraMatrix);
    void cleanup();

private:
    struct PrimitiveObject {
        GLuint vao;
        std::map<int, GLuint> vbos;
        GLenum mode;
        int indexCount;
        GLenum indexType;
        GLuint textureID;
    };

    std::vector<PrimitiveObject> primitiveObjects;

    GLuint programID;
    GLuint mvpMatrixID;
    GLuint textureSamplerID;

    void restoreOpenGLState(GLint program, GLint vao, GLint arrayBuffer,
                           GLint elementBuffer, GLboolean depthTest,
                           GLboolean cullFace, GLint attribEnabled[4]);

    GLuint loadTextureFromMemory(const unsigned char* data, int width, int height, int channels);
    GLuint createDefaultTexture();
};

#endif