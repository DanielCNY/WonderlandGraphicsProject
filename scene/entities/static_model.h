#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include <string>

class StaticModel {
private:
    struct PrimitiveObject {
        GLuint vao;
        std::vector<GLuint> vbos;
        GLenum mode;
        int indexCount;
        GLenum indexType;
        GLuint textureID;
        bool isTextureFromManager;
    };
    
    std::vector<PrimitiveObject> primitiveObjects;
    GLuint programID;
    GLuint mvpMatrixID;
    GLuint textureSamplerID;
    
    GLuint loadTextureFromMemory(const unsigned char* data, int width, int height, int channels);
    GLuint createDefaultTexture();

    void saveOpenGLState(GLint& program, GLint& vao, GLint& arrayBuffer,
                        GLint& elementBuffer, GLboolean& depthTest,
                        GLboolean& cullFace, GLint attribEnabled[4]);
    void restoreOpenGLState(GLint program, GLint vao, GLint arrayBuffer,
                           GLint elementBuffer, GLboolean depthTest,
                           GLboolean cullFace, GLint attribEnabled[4]);
    
public:
    StaticModel();
    ~StaticModel();
    
    bool loadModel(const char* filename);
    void render(const glm::mat4& cameraMatrix);
    void cleanup();
};