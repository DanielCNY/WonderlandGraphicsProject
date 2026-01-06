#include "static_model.h"
#include "../render/shader.h"
#include "../utils/texture_manager.h"
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <filesystem>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tinygltf-2.9.3/tiny_gltf.h>

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

std::unordered_map<std::string, std::shared_ptr<StaticModel::ModelCache>> StaticModel::modelCache;

StaticModel::StaticModel() : cachedModel(nullptr) {
}

StaticModel::~StaticModel() {
    cleanup();
}

StaticModel::StaticModel(StaticModel&& other) noexcept
    : modelFilename(std::move(other.modelFilename)),
      cachedModel(std::move(other.cachedModel)) {
    other.modelFilename.clear();
    other.cachedModel.reset();
}

StaticModel& StaticModel::operator=(StaticModel&& other) noexcept {
    if (this != &other) {
        cleanup();
        modelFilename = std::move(other.modelFilename);
        cachedModel = std::move(other.cachedModel);
        other.modelFilename.clear();
        other.cachedModel.reset();
    }
    return *this;
}

StaticModel::ModelCache::~ModelCache() {
    cleanup();
}

void StaticModel::ModelCache::cleanup() {
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
        mvpMatrixID = 0;
        textureSamplerID = 0;
    }
}

GLuint StaticModel::loadTextureFromMemory(const unsigned char* data, int width, int height, int channels) {
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

GLuint StaticModel::createDefaultTexture() {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    unsigned char defaultTexture[] = { 50, 205, 50, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, defaultTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return textureID;
}

std::shared_ptr<StaticModel::ModelCache> StaticModel::loadModelToCache(const char* filename) {
    auto it = modelCache.find(filename);
    if (it != modelCache.end()) {
        it->second->referenceCount++;
        return it->second;
    }

    std::cout << "Loading model to cache: " << filename << std::endl;

    auto cache = std::make_shared<ModelCache>();

    GLint prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer;
    GLboolean prevDepthTest, prevCullFace;
    GLint attribEnabled[4];

    saveOpenGLState(prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer,prevDepthTest, prevCullFace, attribEnabled);

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

    if (model.meshes.empty()) {
        std::cout << "Model has no meshes: " << filename << std::endl;
        restoreOpenGLState(prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer,
                          prevDepthTest, prevCullFace, attribEnabled);
        return nullptr;
    }

    cache->programID = LoadShadersFromFile("../scene/shaders/static_model.vert",
                                          "../scene/shaders/static_model.frag");
    if (cache->programID == 0) {
        std::cerr << "Failed to load static model shaders" << std::endl;
        restoreOpenGLState(prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer,
                          prevDepthTest, prevCullFace, attribEnabled);
        return nullptr;
    }

    cache->mvpMatrixID = glGetUniformLocation(cache->programID, "MVP");
    cache->textureSamplerID = glGetUniformLocation(cache->programID, "textureSampler");

    const tinygltf::Mesh &mesh = model.meshes[0];

    for (size_t i = 0; i < mesh.primitives.size(); ++i) {
        const tinygltf::Primitive &primitive = mesh.primitives[i];

        PrimitiveObject primObj;
        primObj.mode = primitive.mode;
        primObj.indexCount = 0;
        primObj.indexType = GL_UNSIGNED_INT;

        glGenVertexArrays(1, &primObj.vao);
        glBindVertexArray(primObj.vao);

        for (auto &attrib : primitive.attributes) {
            const tinygltf::Accessor &accessor = model.accessors[attrib.second];

            if (accessor.bufferView < 0 || accessor.bufferView >= model.bufferViews.size()) {
                continue;
            }

            const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
            const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

            GLuint vbo;
            glGenBuffers(1, &vbo);
            glBindBuffer(bufferView.target, vbo);
            glBufferData(bufferView.target, bufferView.byteLength,&buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);

            primObj.vbos.push_back(vbo);

            int location = -1;
            if (attrib.first == "POSITION") location = 0;
            else if (attrib.first == "NORMAL") location = 1;
            else if (attrib.first == "TEXCOORD_0") location = 2;

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
            const tinygltf::Accessor &indexAccessor = model.accessors[primitive.indices];

            if (indexAccessor.bufferView >= 0 && indexAccessor.bufferView < model.bufferViews.size()) {
                const tinygltf::BufferView &indexBufferView = model.bufferViews[indexAccessor.bufferView];
                const tinygltf::Buffer &indexBuffer = model.buffers[indexBufferView.buffer];

                GLuint ebo;
                glGenBuffers(1, &ebo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferView.byteLength,
                            &indexBuffer.data.at(0) + indexBufferView.byteOffset, GL_STATIC_DRAW);

                primObj.vbos.push_back(ebo);
                primObj.indexCount = indexAccessor.count;
                primObj.indexType = indexAccessor.componentType;
            }
        }

        GLuint textureID = 0;
        bool isTextureFromManager = false;

        if (primitive.material >= 0 && primitive.material < model.materials.size()) {
            const tinygltf::Material &material = model.materials[primitive.material];

            if (material.pbrMetallicRoughness.baseColorTexture.index >= 0) {
                int texIndex = material.pbrMetallicRoughness.baseColorTexture.index;
                if (texIndex < model.textures.size()) {
                    const tinygltf::Texture &texture = model.textures[texIndex];
                    if (texture.source >= 0 && texture.source < model.images.size()) {
                        const tinygltf::Image &image = model.images[texture.source];

                        if (!image.uri.empty()) {
                            std::string texturePath = image.uri;
                            textureID = TextureManager::getInstance().getTexture(texturePath);
                            isTextureFromManager = (textureID != 0);

                            if (textureID == 0 && !image.image.empty()) {
                                textureID = loadTextureFromMemory(image.image.data(),
                                                                image.width,
                                                                image.height,
                                                                image.component);
                            }
                        } else if (!image.image.empty()) {
                            textureID = loadTextureFromMemory(image.image.data(),
                                                            image.width,
                                                            image.height,
                                                            image.component);
                        }
                    }
                }
            }
        }

        if (textureID == 0) {
            textureID = createDefaultTexture();
        }

        primObj.textureID = textureID;
        primObj.isTextureFromManager = isTextureFromManager;

        cache->primitiveObjects.push_back(primObj);
        glBindVertexArray(0);
    }

    restoreOpenGLState(prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer,
                      prevDepthTest, prevCullFace, attribEnabled);

    if (cache->primitiveObjects.empty()) {
        return nullptr;
    }

    cache->referenceCount = 1;
    modelCache[filename] = cache;

    std::cout << "Model cached successfully: " << filename
              << " (primitives: " << cache->primitiveObjects.size()
              << ", textures loaded)" << std::endl;

    return cache;
}

bool StaticModel::loadModel(const char* filename) {
    cleanup();

    cachedModel = loadModelToCache(filename);
    if (!cachedModel) {
        return false;
    }

    modelFilename = filename;
    return true;
}

void StaticModel::saveOpenGLState(GLint& program, GLint& vao, GLint& arrayBuffer,
                                 GLint& elementBuffer, GLboolean& depthTest,
                                 GLboolean& cullFace, GLint attribEnabled[4]) {
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &elementBuffer);

    depthTest = glIsEnabled(GL_DEPTH_TEST);
    cullFace = glIsEnabled(GL_CULL_FACE);

    for (int i = 0; i < 4; i++) {
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attribEnabled[i]);
    }
}

void StaticModel::restoreOpenGLState(GLint program, GLint vao, GLint arrayBuffer,
                                    GLint elementBuffer, GLboolean depthTest,
                                    GLboolean cullFace, GLint attribEnabled[4]) {
    glUseProgram(program);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);

    if (depthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (cullFace) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);

    for (int i = 0; i < 4; i++) {
        if (attribEnabled[i]) {
            glEnableVertexAttribArray(i);
        } else {
            glDisableVertexAttribArray(i);
        }
    }
}

void StaticModel::render(const glm::mat4& cameraMatrix) {
    if (!cachedModel || cachedModel->programID == 0 || cachedModel->primitiveObjects.empty()) {
        return;
    }

    GLint prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer;
    GLboolean prevDepthTest, prevCullFace;
    GLint attribEnabled[4];
    saveOpenGLState(prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer,
                   prevDepthTest, prevCullFace, attribEnabled);

    glUseProgram(cachedModel->programID);
    glUniformMatrix4fv(cachedModel->mvpMatrixID, 1, GL_FALSE, &cameraMatrix[0][0]);

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

void StaticModel::cleanup() {
    if (cachedModel) {
        auto it = modelCache.find(modelFilename);
        if (it != modelCache.end()) {
            it->second->referenceCount--;
            if (it->second->referenceCount <= 0) {
                modelCache.erase(it);
                std::cout << "Model removed from cache: " << modelFilename << std::endl;
            }
        }
        cachedModel.reset();
    }
    modelFilename.clear();
}

void StaticModel::cleanupAll() {
    std::cout << "Cleaning up all cached models..." << std::endl;
    modelCache.clear();
    std::cout << "All models cleared from cache." << std::endl;
}