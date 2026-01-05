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

StaticModel::StaticModel() : programID(0), mvpMatrixID(0), textureSamplerID(0) {
}

StaticModel::~StaticModel() {
    cleanup();
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

bool StaticModel::loadModel(const char* filename) {
    cleanup();

    GLint prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevArrayBuffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &prevElementBuffer);

    GLboolean prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prevCullFace = glIsEnabled(GL_CULL_FACE);

    GLint attribEnabled[4];
    for (int i = 0; i < 4; i++) {
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attribEnabled[i]);
    }

    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err;
    std::string warn;

    bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);

    if (!res) {
        std::cout << "Failed to load glTF: " << filename << std::endl;
        restoreOpenGLState(prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer,
                          prevDepthTest, prevCullFace, attribEnabled);
        return false;
    }

    if (model.meshes.empty()) {
        std::cout << "Model has no meshes: " << filename << std::endl;
        restoreOpenGLState(prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer,
                          prevDepthTest, prevCullFace, attribEnabled);
        return false;
    }

    std::filesystem::path gltfPath(filename);
    std::string directory = gltfPath.parent_path().string();
    if (directory.empty()) directory = ".";

    std::map<int, GLuint> vbos;
    for (size_t i = 0; i < model.bufferViews.size(); ++i) {
        const tinygltf::BufferView &bufferView = model.bufferViews[i];

        if (bufferView.target == 0) continue;

        const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(bufferView.target, vbo);
        glBufferData(bufferView.target, bufferView.byteLength,
                    &buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);
        vbos[i] = vbo;
    }

    const tinygltf::Mesh &mesh = model.meshes[0];

    for (size_t i = 0; i < mesh.primitives.size(); ++i) {
        const tinygltf::Primitive &primitive = mesh.primitives[i];

        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        bool hasTexCoords = false;
        for (auto &attrib : primitive.attributes) {
            const tinygltf::Accessor &accessor = model.accessors[attrib.second];

            if (vbos.find(accessor.bufferView) == vbos.end()) continue;

            glBindBuffer(GL_ARRAY_BUFFER, vbos[accessor.bufferView]);
            int byteStride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);

            int location = -1;
            if (attrib.first == "POSITION") location = 0;
            else if (attrib.first == "NORMAL") location = 1;
            else if (attrib.first == "TEXCOORD_0") {
                location = 2;
                hasTexCoords = true;
            }

            if (location >= 0) {
                glEnableVertexAttribArray(location);

                GLint size = 1;
                if (accessor.type == TINYGLTF_TYPE_VEC2) size = 2;
                else if (accessor.type == TINYGLTF_TYPE_VEC3) size = 3;
                else if (accessor.type == TINYGLTF_TYPE_VEC4) size = 4;

                glVertexAttribPointer(location, size, accessor.componentType,
                    accessor.normalized ? GL_TRUE : GL_FALSE,
                    byteStride, BUFFER_OFFSET(accessor.byteOffset));
            }
        }

        int indexCount = 0;
        GLenum indexType = GL_UNSIGNED_INT;

        if (primitive.indices >= 0) {
            const tinygltf::Accessor &indexAccessor = model.accessors[primitive.indices];
            if (vbos.find(indexAccessor.bufferView) != vbos.end()) {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[indexAccessor.bufferView]);
                indexCount = indexAccessor.count;
                indexType = indexAccessor.componentType;
            }
        }

        GLuint textureID = 0;
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
                            std::cout << "Attempting to load texture from: " << texturePath << std::endl;

                            textureID = TextureManager::getInstance().getTexture(texturePath);

                            if (textureID == 0) {
                                std::cout << "WARNING: TextureManager failed to load: " << texturePath << std::endl;
                                std::cout << "Trying to load directly from memory..." << std::endl;

                                if (!image.image.empty()) {
                                    textureID = loadTextureFromMemory(image.image.data(),
                                                                    image.width,
                                                                    image.height,
                                                                    image.component);
                                    std::cout << "Direct load result: " << (textureID != 0 ? "SUCCESS" : "FAILED") << std::endl;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (textureID == 0) {
            textureID = createDefaultTexture();
        }

        PrimitiveObject primObj;
        primObj.vao = vao;
        primObj.vbos = vbos;
        primObj.mode = primitive.mode;
        primObj.indexCount = indexCount;
        primObj.indexType = indexType;
        primObj.textureID = textureID;

        primitiveObjects.push_back(primObj);
        glBindVertexArray(0);
    }

    programID = LoadShadersFromFile("../scene/shaders/static_model.vert",
                                   "../scene/shaders/static_model.frag");
    if (programID == 0) {
        std::cerr << "Failed to load static model shaders" << std::endl;
        restoreOpenGLState(prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer,
                          prevDepthTest, prevCullFace, attribEnabled);
        return false;
    }

    mvpMatrixID = glGetUniformLocation(programID, "MVP");
    textureSamplerID = glGetUniformLocation(programID, "textureSampler");

    restoreOpenGLState(prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer,
                      prevDepthTest, prevCullFace, attribEnabled);

    return true;
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
    if (programID == 0 || primitiveObjects.empty()) return;

    GLint prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevArrayBuffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &prevElementBuffer);

    GLint attribEnabled[4];
    for (int i = 0; i < 4; i++) {
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attribEnabled[i]);
    }

    glUseProgram(programID);
    glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &cameraMatrix[0][0]);

    for (const auto& primitive : primitiveObjects) {
        if (primitive.textureID != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, primitive.textureID);

            if (textureSamplerID != -1) {
                glUniform1i(textureSamplerID, 0);
            }
        }

        glBindVertexArray(primitive.vao);

        if (primitive.indexCount > 0) {
            glDrawElements(primitive.mode, primitive.indexCount,
                         primitive.indexType, nullptr);
        }

        glBindVertexArray(0);
    }

    for (int i = 0; i < 4; i++) {
        if (attribEnabled[i]) {
            glEnableVertexAttribArray(i);
        } else {
            glDisableVertexAttribArray(i);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, prevArrayBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prevElementBuffer);
    glBindVertexArray(prevVAO);
    glUseProgram(prevProgram);
}

void StaticModel::cleanup() {
    for (auto& primitive : primitiveObjects) {
        if (primitive.vao) {
            glDeleteVertexArrays(1, &primitive.vao);
        }

        for (auto& vboPair : primitive.vbos) {
            glDeleteBuffers(1, &vboPair.second);
        }
        primitive.vbos.clear();

        if (primitive.textureID) {
            glDeleteTextures(1, &primitive.textureID);
        }
    }
    primitiveObjects.clear();

    if (programID) {
        glDeleteProgram(programID);
        programID = 0;
    }
}