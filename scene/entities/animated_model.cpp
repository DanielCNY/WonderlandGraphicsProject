#include <vector>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/detail/type_mat.hpp>
#include <render/shader.h>
#include <iostream>
#include <map>
#include <unordered_map>
#include <tinygltf-2.9.3/tiny_gltf.h>
#include  "animated_model.h"
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

std::unordered_map<std::string, std::shared_ptr<AnimatedModel::ModelCache>> AnimatedModel::modelCache;

AnimatedModel::AnimatedModel() : cachedModel(nullptr) {
}

AnimatedModel::~AnimatedModel() {
    cleanup();
}

AnimatedModel::AnimatedModel(AnimatedModel&& other) noexcept
    : modelFilename(std::move(other.modelFilename)),
      cachedModel(std::move(other.cachedModel)),
      currentTime(other.currentTime),
      isPlaying(other.isPlaying),
      playbackSpeed(other.playbackSpeed),
      currentAnimationClip(other.currentAnimationClip) {
    other.modelFilename.clear();
    other.cachedModel.reset();
    other.currentTime = 0.0f;
}

AnimatedModel& AnimatedModel::operator=(AnimatedModel&& other) noexcept {
    if (this != &other) {
        cleanup();
        modelFilename = std::move(other.modelFilename);
        cachedModel = std::move(other.cachedModel);
        currentTime = other.currentTime;
        isPlaying = other.isPlaying;
        playbackSpeed = other.playbackSpeed;
        currentAnimationClip = other.currentAnimationClip;
        other.modelFilename.clear();
        other.cachedModel.reset();
        other.currentTime = 0.0f;
    }
    return *this;
}

void AnimatedModel::ModelCache::cleanup() {
    for (auto& primitive : primitiveObjects) {
        if (primitive.vao) {
            glDeleteVertexArrays(1, &primitive.vao);
            primitive.vao = 0;
        }

        for (auto vbo : primitive.vbos) {
            glDeleteBuffers(1, &vbo);
        }
        primitive.vbos.clear();

        if (primitive.textureID && !primitive.isTextureFromManager) {
            glDeleteTextures(1, &primitive.textureID);
            primitive.textureID = 0;
        }
    }
    primitiveObjects.clear();

    if (programID) {
        glDeleteProgram(programID);
        programID = 0;
    }
}

glm::mat4 AnimatedModel::getNodeTransform(const tinygltf::Node& node) {
    glm::mat4 transform(1.0f);

    if (node.matrix.size() == 16) {
        transform = glm::make_mat4(node.matrix.data());
    } else {
        if (node.translation.size() == 3) {
            transform = glm::translate(transform, glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
        }
        if (node.rotation.size() == 4) {
            glm::quat q(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
            transform *= glm::mat4_cast(q);
        }
        if (node.scale.size() == 3) {
            transform = glm::scale(transform, glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
        }
    }
    return transform;
}

void AnimatedModel::computeNodeHierarchy(int nodeIndex, const glm::mat4& parentTransform) {
    if (!cachedModel || nodeIndex < 0 || nodeIndex >= cachedModel->localNodeTransforms.size()) {
        return;
    }

    cachedModel->globalNodeTransforms[nodeIndex] = parentTransform * cachedModel->localNodeTransforms[nodeIndex];

    const auto& node = cachedModel->model.nodes[nodeIndex];
    for (int childIndex : node.children) {
        computeNodeHierarchy(childIndex, cachedModel->globalNodeTransforms[nodeIndex]);
    }
}

int AnimatedModel::findKeyframeIndex(const std::vector<float>& times, float animationTime) {
    int left = 0;
    int right = times.size() - 1;

    while (left <= right) {
        int mid = (left + right) / 2;

        if (mid + 1 < times.size() && times[mid] <= animationTime && animationTime < times[mid + 1]) {
            return mid;
        }
        else if (times[mid] > animationTime) {
            right = mid - 1;
        }
        else {
            left = mid + 1;
        }
    }

    return times.size() - 2;
}

void AnimatedModel::updateAnimation(std::vector<glm::mat4>& localTransforms, float time) {
    if (!cachedModel || cachedModel->animationClips.empty()) return;

    const auto& clip = cachedModel->animationClips[0];
    float animationTime = fmod(time, clip.duration);

    for (size_t i = 0; i < clip.channels.size(); ++i) {
        const auto& channel = clip.channels[i];
        if (channel.samplerIndex < 0 || channel.samplerIndex >= clip.samplers.size()) {
            continue;
        }

        const auto& sampler = clip.samplers[channel.samplerIndex];

        int keyframeIndex = findKeyframeIndex(sampler.inputTimes, animationTime);
        float t = (animationTime - sampler.inputTimes[keyframeIndex]) /
                  (sampler.inputTimes[keyframeIndex + 1] - sampler.inputTimes[keyframeIndex]);

        if (channel.targetPath == "translation") {
            glm::vec3 translation0 = glm::vec3(sampler.outputValues[keyframeIndex]);
            glm::vec3 translation1 = glm::vec3(sampler.outputValues[keyframeIndex + 1]);
            glm::vec3 translation = translation0 + t * (translation1 - translation0);
            localTransforms[channel.targetNodeIndex][3] = glm::vec4(translation, 1.0f);
        }
        else if (channel.targetPath == "rotation") {
            glm::quat rotation0 = glm::quat(
                sampler.outputValues[keyframeIndex].w,
                sampler.outputValues[keyframeIndex].x,
                sampler.outputValues[keyframeIndex].y,
                sampler.outputValues[keyframeIndex].z
            );
            glm::quat rotation1 = glm::quat(
                sampler.outputValues[keyframeIndex + 1].w,
                sampler.outputValues[keyframeIndex + 1].x,
                sampler.outputValues[keyframeIndex + 1].y,
                sampler.outputValues[keyframeIndex + 1].z
            );
            glm::quat rotation = glm::slerp(rotation0, rotation1, t);

            glm::vec3 scale(
                glm::length(glm::vec3(localTransforms[channel.targetNodeIndex][0])),
                glm::length(glm::vec3(localTransforms[channel.targetNodeIndex][1])),
                glm::length(glm::vec3(localTransforms[channel.targetNodeIndex][2]))
            );

            glm::vec3 translation = glm::vec3(localTransforms[channel.targetNodeIndex][3]);

            glm::mat4 R = glm::mat4_cast(rotation);
            glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
            glm::mat4 T = glm::mat4(1.0f);
            T[3] = glm::vec4(translation, 1.0f);

            localTransforms[channel.targetNodeIndex] = T * R * S;
        }
        else if (channel.targetPath == "scale") {
            glm::vec3 scale0 = glm::vec3(sampler.outputValues[keyframeIndex]);
            glm::vec3 scale1 = glm::vec3(sampler.outputValues[keyframeIndex + 1]);
            glm::vec3 scale = scale0 + t * (scale1 - scale0);

            glm::mat4 M = localTransforms[channel.targetNodeIndex];
            glm::vec3 translation = glm::vec3(M[3]);
            glm::mat3 rotation = glm::mat3(M);

            localTransforms[channel.targetNodeIndex] =
                glm::translate(glm::mat4(1.0f), translation) *
                glm::mat4(rotation) *
                glm::scale(glm::mat4(1.0f), scale);
        }
    }
}

void AnimatedModel::computeGlobalNodeTransform(const tinygltf::Model& model,
    const std::vector<glm::mat4>& localTransforms,
    int nodeIndex, const glm::mat4& parentTransform,
    std::vector<glm::mat4>& globalTransforms)
{
    glm::mat4 globalTransform = parentTransform * localTransforms[nodeIndex];
    globalTransforms[nodeIndex] = globalTransform;

    const tinygltf::Node &node = model.nodes[nodeIndex];
    for (int childIndex : node.children) {
        computeGlobalNodeTransform(model, localTransforms, childIndex, globalTransform, globalTransforms);
    }
}

void AnimatedModel::updateSkinning(const std::vector<glm::mat4>& globalTransforms) {
    if (!cachedModel || cachedModel->skinData.empty()) return;

    glm::mat4 rootGlobal(1.0f);

    for (size_t i = 0; i < cachedModel->model.nodes.size(); i++) {
        if (cachedModel->model.nodes[i].skin == 0) {
            rootGlobal = globalTransforms[i];
            break;
        }
    }

    glm::mat4 invRoot = glm::inverse(rootGlobal);

    for (size_t i = 0; i < cachedModel->skinData.size(); ++i) {
        auto& skin = cachedModel->skinData[i];
        const tinygltf::Skin& gltfSkin = cachedModel->model.skins[i];

        for (size_t j = 0; j < gltfSkin.joints.size(); ++j) {
            int jointNodeIndex = gltfSkin.joints[j];
            skin.globalJointTransforms[j] = globalTransforms[jointNodeIndex];
            skin.jointMatrices[j] = invRoot * skin.globalJointTransforms[j] * skin.inverseBindMatrices[j];
        }
    }
}

void AnimatedModel::updateNodeTransforms() {
    if (!cachedModel) return;

    for (int rootNode : cachedModel->model.scenes[cachedModel->model.defaultScene].nodes) {
        computeNodeHierarchy(rootNode, glm::mat4(1.0f));
    }
}

void AnimatedModel::update(float time) {
    if (!isPlaying || !cachedModel || cachedModel->animationClips.empty()) return;

    currentTime += time;

    const auto& clip = cachedModel->animationClips[0];

    std::vector<glm::mat4> localTransforms(cachedModel->model.nodes.size());
    std::vector<glm::mat4> globalTransforms(cachedModel->model.nodes.size());

    for (size_t i = 0; i < cachedModel->model.nodes.size(); ++i) {
        localTransforms[i] = getNodeTransform(cachedModel->model.nodes[i]);
    }

    updateAnimation(localTransforms, currentTime);

    for (int root : cachedModel->model.scenes[cachedModel->model.defaultScene].nodes) {
        computeGlobalNodeTransform(cachedModel->model, localTransforms, root, glm::mat4(1.0f), globalTransforms);
    }

    updateSkinning(globalTransforms);

    cachedModel->localNodeTransforms = localTransforms;
    cachedModel->globalNodeTransforms = globalTransforms;
}

std::shared_ptr<AnimatedModel::ModelCache> AnimatedModel::loadModelToCache(const char* filename) {
    auto it = modelCache.find(filename);
    if (it != modelCache.end()) {
        it->second->referenceCount++;
        return it->second;
    }

    std::cout << "Loading animated model to cache: " << filename << std::endl;

    auto cache = std::make_shared<ModelCache>();

    GLint prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer;
    GLboolean prevDepthTest, prevCullFace;
    GLint attribEnabled[5];
    saveOpenGLState(prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer,
                   prevDepthTest, prevCullFace, attribEnabled);

    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err;
    std::string warn;

    bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    if (!res) {
        std::cout << "Failed to load glTF: " << filename << " - " << err << std::endl;
        restoreOpenGLState(prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer,
                          prevDepthTest, prevCullFace, attribEnabled);
        return nullptr;
    }

    cache->model = model;

    cache->programID = LoadShadersFromFile("../scene/shaders/bot.vert", "../scene/shaders/bot.frag");
    if (cache->programID == 0) {
        std::cerr << "Failed to load animated model shaders" << std::endl;
        restoreOpenGLState(prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer,
                          prevDepthTest, prevCullFace, attribEnabled);
        return nullptr;
    }

    cache->mvpMatrixID = glGetUniformLocation(cache->programID, "MVP");
    cache->jointMatricesID = glGetUniformLocation(cache->programID, "jointMatrices");
    cache->lightPositionID = glGetUniformLocation(cache->programID, "lightPosition");
    cache->lightIntensityID = glGetUniformLocation(cache->programID, "lightIntensity");
    cache->ambientLightID = glGetUniformLocation(cache->programID, "ambientLight");
    cache->viewPositionID = glGetUniformLocation(cache->programID, "viewPosition");
    cache->modelMatrixID = glGetUniformLocation(cache->programID, "modelMatrix");
    cache->textureSamplerID = glGetUniformLocation(cache->programID, "textureSampler");

    cache->localNodeTransforms.resize(model.nodes.size());
    cache->globalNodeTransforms.resize(model.nodes.size());
    cache->nodeParents.resize(model.nodes.size(), -1);

    for (size_t i = 0; i < model.nodes.size(); i++) {
        cache->localNodeTransforms[i] = getNodeTransform(model.nodes[i]);

        for (int childIndex : model.nodes[i].children) {
            if (childIndex >= 0 && childIndex < model.nodes.size()) {
                cache->nodeParents[childIndex] = i;
            }
        }
    }

    for (const auto& skin : model.skins) {
        SkinData skinData;

        if (skin.inverseBindMatrices >= 0) {
            const auto& accessor = model.accessors[skin.inverseBindMatrices];
            const auto& bufferView = model.bufferViews[accessor.bufferView];
            const auto& buffer = model.buffers[bufferView.buffer];

            const float* data = reinterpret_cast<const float*>(
                buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);

            skinData.inverseBindMatrices.resize(accessor.count);
            for (size_t i = 0; i < accessor.count; i++) {
                skinData.inverseBindMatrices[i] = glm::make_mat4(data + i * 16);
            }
        }

        skinData.jointIndices = skin.joints;
        skinData.jointMatrices.resize(skin.joints.size(), glm::mat4(1.0f));
        skinData.globalJointTransforms.resize(skin.joints.size(), glm::mat4(1.0f));

        cache->skinData.push_back(skinData);
    }

    for (const auto& anim : model.animations) {
        AnimationClip clip;
        clip.name = anim.name;
        clip.duration = 0.0f;

        for (const auto& sampler : anim.samplers) {
            AnimationSampler animSampler;

            const auto& inputAccessor = model.accessors[sampler.input];
            const auto& inputBufferView = model.bufferViews[inputAccessor.bufferView];
            const auto& inputBuffer = model.buffers[inputBufferView.buffer];

            const float* inputData = reinterpret_cast<const float*>(
                inputBuffer.data.data() + inputBufferView.byteOffset + inputAccessor.byteOffset);

            animSampler.inputTimes.resize(inputAccessor.count);
            memcpy(animSampler.inputTimes.data(), inputData, inputAccessor.count * sizeof(float));

            if (!animSampler.inputTimes.empty()) {
                clip.duration = glm::max(clip.duration, animSampler.inputTimes.back());
            }

            const auto& outputAccessor = model.accessors[sampler.output];
            const auto& outputBufferView = model.bufferViews[outputAccessor.bufferView];
            const auto& outputBuffer = model.buffers[outputBufferView.buffer];

            const float* outputData = reinterpret_cast<const float*>(
                outputBuffer.data.data() + outputBufferView.byteOffset + outputAccessor.byteOffset);

            animSampler.outputValues.resize(outputAccessor.count);

            if (outputAccessor.type == TINYGLTF_TYPE_VEC3) {
                for (size_t i = 0; i < outputAccessor.count; i++) {
                    animSampler.outputValues[i] = glm::vec4(
                        outputData[i * 3],
                        outputData[i * 3 + 1],
                        outputData[i * 3 + 2],
                        0.0f
                    );
                }
            } else if (outputAccessor.type == TINYGLTF_TYPE_VEC4) {
                memcpy(animSampler.outputValues.data(), outputData,
                      outputAccessor.count * sizeof(glm::vec4));
            }

            if (sampler.interpolation == "LINEAR") {
                animSampler.interpolation = 0;
            } else if (sampler.interpolation == "STEP") {
                animSampler.interpolation = 1;
            } else if (sampler.interpolation == "CUBICSPLINE") {
                animSampler.interpolation = 2;
            } else {
                animSampler.interpolation = 0;
            }

            clip.samplers.push_back(animSampler);
        }

        for (const auto& channel : anim.channels) {
            AnimationChannel animChannel;
            animChannel.samplerIndex = channel.sampler;
            animChannel.targetPath = channel.target_path;
            animChannel.targetNodeIndex = channel.target_node;
            clip.channels.push_back(animChannel);
        }

        cache->animationClips.push_back(clip);
    }

    for (const auto& mesh : model.meshes) {
        for (const auto& primitive : mesh.primitives) {
            PrimitiveObject primObj;
            primObj.mode = primitive.mode;

            glGenVertexArrays(1, &primObj.vao);
            glBindVertexArray(primObj.vao);

            for (const auto& attrib : primitive.attributes) {
                const auto& accessor = model.accessors[attrib.second];
                const auto& bufferView = model.bufferViews[accessor.bufferView];
                const auto& buffer = model.buffers[bufferView.buffer];

                GLuint vbo;
                glGenBuffers(1, &vbo);
                glBindBuffer(bufferView.target, vbo);
                glBufferData(bufferView.target, bufferView.byteLength,
                            buffer.data.data() + bufferView.byteOffset, GL_STATIC_DRAW);

                primObj.vbos.push_back(vbo);

                int location = -1;
                if (attrib.first == "POSITION") location = 0;
                else if (attrib.first == "NORMAL") location = 1;
                else if (attrib.first == "TEXCOORD_0") location = 2;
                else if (attrib.first == "JOINTS_0") location = 3;
                else if (attrib.first == "WEIGHTS_0") location = 4;

                if (location >= 0) {
                    glEnableVertexAttribArray(location);
                    glBindBuffer(GL_ARRAY_BUFFER, vbo);

                    GLint size = 1;
                    if (accessor.type == TINYGLTF_TYPE_VEC2) size = 2;
                    else if (accessor.type == TINYGLTF_TYPE_VEC3) size = 3;
                    else if (accessor.type == TINYGLTF_TYPE_VEC4) size = 4;

                    glVertexAttribPointer(location, size, accessor.componentType,
                                        accessor.normalized ? GL_TRUE : GL_FALSE,
                                        accessor.ByteStride(bufferView),
                                        BUFFER_OFFSET(accessor.byteOffset));
                }
            }

            if (primitive.indices >= 0) {
                const auto& indexAccessor = model.accessors[primitive.indices];
                const auto& indexBufferView = model.bufferViews[indexAccessor.bufferView];
                const auto& indexBuffer = model.buffers[indexBufferView.buffer];

                GLuint ebo;
                glGenBuffers(1, &ebo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferView.byteLength,
                            indexBuffer.data.data() + indexBufferView.byteOffset, GL_STATIC_DRAW);

                primObj.vbos.push_back(ebo);
                primObj.indexCount = indexAccessor.count;
                primObj.indexType = indexAccessor.componentType;
            }

            if (primitive.material >= 0 && primitive.material < model.materials.size()) {
                const auto& material = model.materials[primitive.material];
                if (material.pbrMetallicRoughness.baseColorTexture.index >= 0) {
                    int texIndex = material.pbrMetallicRoughness.baseColorTexture.index;
                    if (texIndex < model.textures.size()) {
                        const auto& texture = model.textures[texIndex];
                        if (texture.source >= 0 && texture.source < model.images.size()) {
                            const auto& image = model.images[texture.source];
                            if (!image.image.empty()) {
                                primObj.textureID = loadTextureFromMemory(
                                    image.image.data(),
                                    image.width,
                                    image.height,
                                    image.component
                                );
                            }
                        }
                    }
                }
            }

            if (primObj.textureID == 0) {
                primObj.textureID = createDefaultTexture();
            }

            cache->primitiveObjects.push_back(primObj);
            glBindVertexArray(0);
        }
    }

    restoreOpenGLState(prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer,
                      prevDepthTest, prevCullFace, attribEnabled);

    cache->referenceCount = 1;
    modelCache[filename] = cache;

    std::cout << "Animated model cached successfully: " << filename << std::endl;
    return cache;
}

bool AnimatedModel::loadModel(const char* filename) {
    cleanup();

    cachedModel = loadModelToCache(filename);
    if (!cachedModel) {
        return false;
    }

    modelFilename = filename;

    if (!cachedModel->animationClips.empty()) {
        currentAnimationClip = 0;
        currentTime = 0.0f;

        for (size_t i = 0; i < cachedModel->model.nodes.size(); ++i) {
            cachedModel->localNodeTransforms[i] = getNodeTransform(cachedModel->model.nodes[i]);
        }

        for (int root : cachedModel->model.scenes[cachedModel->model.defaultScene].nodes) {
            computeGlobalNodeTransform(cachedModel->model, cachedModel->localNodeTransforms,
                                     root, glm::mat4(1.0f), cachedModel->globalNodeTransforms);
        }
        updateSkinning(cachedModel->globalNodeTransforms);
    }

    return true;
}

void AnimatedModel::render(const glm::mat4& modelMatrix, const glm::mat4& viewProjectionMatrix,
                          const glm::vec3& lightPosition, const glm::vec3& lightIntensity,
                          const glm::vec3& ambientLight, const glm::vec3& viewPosition) {
    if (!cachedModel || cachedModel->programID == 0 || cachedModel->primitiveObjects.empty()) {
        return;
    }

    GLint prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer;
    GLboolean prevDepthTest, prevCullFace;
    GLint attribEnabled[5];
    saveOpenGLState(prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer,
                   prevDepthTest, prevCullFace, attribEnabled);

    glUseProgram(cachedModel->programID);

    glm::mat4 mvp = viewProjectionMatrix * modelMatrix;
    glUniformMatrix4fv(cachedModel->mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);
    glUniformMatrix4fv(cachedModel->modelMatrixID, 1, GL_FALSE, &modelMatrix[0][0]);

    if (!cachedModel->skinData.empty() && cachedModel->jointMatricesID != 0) {
        const auto& skin = cachedModel->skinData[0];
        glUniformMatrix4fv(cachedModel->jointMatricesID, skin.jointMatrices.size(),
                          GL_FALSE, glm::value_ptr(skin.jointMatrices[0]));
    }

    glUniform3fv(cachedModel->lightPositionID, 1, &lightPosition[0]);
    glUniform3fv(cachedModel->lightIntensityID, 1, &lightIntensity[0]);
    glUniform3fv(cachedModel->ambientLightID, 1, &ambientLight[0]);
    glUniform3fv(cachedModel->viewPositionID, 1, &viewPosition[0]);

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(cachedModel->textureSamplerID, 0);

    for (const auto& primitive : cachedModel->primitiveObjects) {
        glBindTexture(GL_TEXTURE_2D, primitive.textureID);
        glBindVertexArray(primitive.vao);

        if (primitive.indexCount > 0) {
            glDrawElements(primitive.mode, primitive.indexCount,
                         primitive.indexType, nullptr);
        }

        glBindVertexArray(0);
    }

    restoreOpenGLState(prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer,
                      prevDepthTest, prevCullFace, attribEnabled);
}

void AnimatedModel::cleanup() {
    if (cachedModel) {
        auto it = modelCache.find(modelFilename);
        if (it != modelCache.end()) {
            it->second->referenceCount--;
            if (it->second->referenceCount <= 0) {
                modelCache.erase(it);
                std::cout << "Animated model removed from cache: " << modelFilename << std::endl;
            }
        }
        cachedModel.reset();
    }
    modelFilename.clear();
}

void AnimatedModel::saveOpenGLState(GLint& program, GLint& vao, GLint& arrayBuffer,
                                   GLint& elementBuffer, GLboolean& depthTest,
                                   GLboolean& cullFace, GLint attribEnabled[5]) {
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &elementBuffer);

    depthTest = glIsEnabled(GL_DEPTH_TEST);
    cullFace = glIsEnabled(GL_CULL_FACE);

    for (int i = 0; i < 5; i++) {
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attribEnabled[i]);
    }
}

void AnimatedModel::restoreOpenGLState(GLint program, GLint vao, GLint arrayBuffer,
                                      GLint elementBuffer, GLboolean depthTest,
                                      GLboolean cullFace, GLint attribEnabled[5]) {
    glUseProgram(program);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);

    if (depthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (cullFace) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);

    for (int i = 0; i < 5; i++) {
        if (attribEnabled[i]) {
            glEnableVertexAttribArray(i);
        } else {
            glDisableVertexAttribArray(i);
        }
    }
}

GLuint AnimatedModel::createDefaultTexture() {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    unsigned char defaultTexture[] = { 128, 128, 128, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return textureID;
}

GLuint AnimatedModel::loadTextureFromMemory(const unsigned char* data, int width, int height, int channels) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = GL_RGB;
    if (channels == 1) format = GL_RED;
    else if (channels == 2) format = GL_RG;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    return textureID;
}