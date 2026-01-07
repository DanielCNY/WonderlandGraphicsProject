#ifndef ANIMATED_MODEL_H
#define ANIMATED_MODEL_H
#include <vector>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include "tinygltf-2.9.3/tiny_gltf.h"

namespace tinygltf {
    class Model;
    struct Node;
}

class AnimatedModel {
public:
    AnimatedModel();
    ~AnimatedModel();
    AnimatedModel(AnimatedModel&& other) noexcept;
    AnimatedModel& operator=(AnimatedModel&& other) noexcept;

    bool loadModel(const char* filename);
    void update(float deltaTime);
    void render(const glm::mat4& modelMatrix, const glm::mat4& viewProjectionMatrix, const glm::vec3& lightPosition,
                        const glm::vec3& lightIntensity, const glm::vec3& ambientLight, const glm::vec3& viewPosition);
    void cleanup();
    void play() { isPlaying = true; }
    void pause() { isPlaying = false; }
    void setPlaybackSpeed(float speed) { playbackSpeed = speed; }
    void resetAnimation() { currentTime = 0.0f; updateNodeTransforms(); }

    float currentTime = 0.0f;
    bool isPlaying = true;
    float playbackSpeed = 1.0f;

private:
    struct PrimitiveObject {
        GLuint vao = 0;
        std::vector<GLuint> vbos;
        GLenum mode = GL_TRIANGLES;
        GLsizei indexCount = 0;
        GLenum indexType = GL_UNSIGNED_INT;
        GLuint textureID = 0;
        bool isTextureFromManager = false;
    };

    struct SkinData {
        std::vector<glm::mat4> inverseBindMatrices;
        std::vector<int> jointIndices;
        std::vector<glm::mat4> jointMatrices;
        std::vector<glm::mat4> globalJointTransforms;
    };

    struct AnimationSampler {
        std::vector<float> inputTimes;
        std::vector<glm::vec4> outputValues;
        int interpolation;
    };

    struct AnimationChannel {
        int samplerIndex;
        std::string targetPath;
        int targetNodeIndex;
    };

    struct AnimationClip {
        std::string name;
        float duration = 0.0f;
        std::vector<AnimationSampler> samplers;
        std::vector<AnimationChannel> channels;
    };

    struct ModelCache {
        tinygltf::Model model;
        std::vector<PrimitiveObject> primitiveObjects;
        GLuint programID = 0;
        GLuint mvpMatrixID = 0;
        GLuint jointMatricesID = 0;
        GLuint lightPositionID = 0;
        GLuint lightIntensityID = 0;
        GLuint ambientLightID = 0;
        GLuint viewPositionID = 0;
        GLuint modelMatrixID = 0;
        GLuint textureSamplerID = 0;

        std::vector<SkinData> skinData;
        std::vector<AnimationClip> animationClips;
        std::vector<int> skinToMeshMap;

        std::vector<glm::mat4> localNodeTransforms;
        std::vector<glm::mat4> globalNodeTransforms;
        std::vector<int> nodeParents;

        int referenceCount = 0;

        ~ModelCache() { cleanup(); }
        void cleanup();
    };

    static std::unordered_map<std::string, std::shared_ptr<ModelCache>> modelCache;

    std::string modelFilename;
    std::shared_ptr<ModelCache> cachedModel;

    int currentAnimationClip = 0;

    std::shared_ptr<ModelCache> loadModelToCache(const char* filename);
    void updateNodeTransforms();

    void updateAnimation(std::vector<glm::mat4>& localTransforms, float time);
    static void computeGlobalNodeTransform(const tinygltf::Model& model, const std::vector<glm::mat4>& localTransforms,
        int nodeIndex, const glm::mat4& parentTransform, std::vector<glm::mat4>& globalTransforms);
    void updateSkinning(const std::vector<glm::mat4>& globalTransforms);

    glm::mat4 getNodeTransform(const tinygltf::Node& node);
    void computeNodeHierarchy(int nodeIndex, const glm::mat4& parentTransform);
    int findKeyframeIndex(const std::vector<float>& times, float animationTime);
    glm::mat4 interpolateTransform(const AnimationSampler& sampler, float time, const std::string& path);

    void saveOpenGLState(GLint& program, GLint& vao, GLint& arrayBuffer,
                        GLint& elementBuffer, GLboolean& depthTest,
                        GLboolean& cullFace, GLint attribEnabled[5]);
    void restoreOpenGLState(GLint program, GLint vao, GLint arrayBuffer,
                           GLint elementBuffer, GLboolean depthTest,
                           GLboolean cullFace, GLint attribEnabled[5]);

    GLuint createDefaultTexture();
    GLuint loadTextureFromMemory(const unsigned char* data, int width, int height, int channels);
};

#endif