#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

class StaticModel {
private:
    struct PrimitiveObject {
        GLuint vao = 0;
        std::vector<GLuint> vbos;
        GLenum mode = GL_TRIANGLES;
        int indexCount = 0;
        GLenum indexType = GL_UNSIGNED_INT;
        GLuint textureID = 0;
        bool isTextureFromManager = false;
    };

    struct ModelCache {
        GLuint programID;
        GLuint modelMatrixID;
        GLuint viewProjectionMatrixID;
        GLuint mvpMatrixID;
        GLuint lightPositionID;
        GLuint lightIntensityID;
        GLuint ambientLightID;
        GLuint viewPositionID;
        GLuint textureSamplerID;
        std::vector<PrimitiveObject> primitiveObjects;
        int referenceCount = 0;

        ~ModelCache();
        void cleanup();
    };

    static std::unordered_map<std::string, std::shared_ptr<ModelCache>> modelCache;
    std::string modelFilename;
    std::shared_ptr<ModelCache> cachedModel;

    GLuint loadTextureFromMemory(const unsigned char* data, int width, int height, int channels);
    GLuint createDefaultTexture();
    std::shared_ptr<ModelCache> loadModelToCache(const char* filename);

    void saveOpenGLState(GLint& program, GLint& vao, GLint& arrayBuffer,
                        GLint& elementBuffer, GLboolean& depthTest,
                        GLboolean& cullFace, GLint attribEnabled[4]);
    void restoreOpenGLState(GLint program, GLint vao, GLint arrayBuffer,
                           GLint elementBuffer, GLboolean depthTest,
                           GLboolean cullFace, GLint attribEnabled[4]);

public:
    StaticModel();
    ~StaticModel();

    StaticModel(const StaticModel&) = delete;
    StaticModel& operator=(const StaticModel&) = delete;

    StaticModel(StaticModel&& other) noexcept;
    StaticModel& operator=(StaticModel&& other) noexcept;

    bool loadModel(const char* filename);
    void render(const glm::mat4& modelMatrix, const glm::mat4& viewProjectionMatrix, const glm::vec3& lightPosition,
                        const glm::vec3& lightIntensity, const glm::vec3& ambientLight, const glm::vec3& viewPosition);
    void cleanup();

    static void cleanupAll();
};