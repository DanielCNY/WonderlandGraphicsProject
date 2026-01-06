#include "ground.h"
#include "../utils/texture_manager.h"
#include "../render/shader.h"
#include <iostream>

GroundPlane::GroundPlane()
    : programID(0), mvpMatrixID(0), textureSamplerID(0), textureID(0),
      vertexArrayID(0), vertexBufferID(0), colorBufferID(0),
      uvBufferID(0), indexBufferID(0) {
}

GroundPlane::~GroundPlane() {
    cleanup();
}

GLuint GroundPlane::loadGroundShaders() {
    return LoadShadersFromFile("../scene/shaders/ground.vert",
                              "../scene/shaders/ground.frag");
}

void GroundPlane::initialize() {
    GLint prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevArrayBuffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &prevElementBuffer);

    GLint attribEnabled[4];
    for (int i = 0; i < 4; i++) {
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attribEnabled[i]);
    }

    programID = loadGroundShaders();
    if (programID == 0) {
        std::cerr << "FAILED TO LOAD GROUND SHADERS!" << std::endl;
        restoreState(prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer, attribEnabled);
        return;
    }

    mvpMatrixID = glGetUniformLocation(programID, "MVP");
    textureSamplerID = glGetUniformLocation(programID, "textureSampler");

    GLfloat vertex_buffer_data[12] = {
        -250.0f, 0.0f, -250.0f,
        -250.0f, 0.0f, 250.0f,
        250.0f, 0.0f, 250.0f,
        250.0f, 0.0f, -250.0f,
    };

    GLfloat color_buffer_data[12] = {
        1,1,1,
        1,1,1,
        1,1,1,
        1,1,1
    };

    GLuint index_buffer_data[6] = {
        0, 1, 2,
        0, 2, 3,
    };

    GLfloat uv_buffer_data[8] = {
        0.0f, 0.0f,
        0.0f, 2.0f,
        2.0f, 2.0f,
        2.0f, 0.0f,
    };

    glGenVertexArrays(1, &vertexArrayID);
    glBindVertexArray(vertexArrayID);

    glGenBuffers(1, &vertexBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data),
                 vertex_buffer_data, GL_STATIC_DRAW);

    glGenBuffers(1, &colorBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(color_buffer_data),
                 color_buffer_data, GL_STATIC_DRAW);

    glGenBuffers(1, &uvBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uv_buffer_data),
                 uv_buffer_data, GL_STATIC_DRAW);

    glGenBuffers(1, &indexBufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer_data),
                 index_buffer_data, GL_STATIC_DRAW);

    TextureManager& tm = TextureManager::getInstance();
    int facade = 1 + std::rand() % 2;
    textureID = tm.getTexture("../scene/textures/snowy_ground0" + std::to_string(facade) + ".jpg");

    glBindVertexArray(0);
    restoreState(prevProgram, prevVAO, prevArrayBuffer, prevElementBuffer, attribEnabled);
}

void GroundPlane::restoreState(GLint program, GLint vao, GLint arrayBuffer,
                              GLint elementBuffer, GLint attribEnabled[4]) {
    glUseProgram(program);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);

    for (int i = 0; i < 4; i++) {
        if (attribEnabled[i]) {
            glEnableVertexAttribArray(i);
        } else {
            glDisableVertexAttribArray(i);
        }
    }
}

void GroundPlane::render(glm::mat4 cameraMatrix) {
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
    glBindVertexArray(vertexArrayID);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glUniform1i(textureSamplerID, 0);

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glm::mat4 mvp = cameraMatrix * modelMatrix;
    glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, prevArrayBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prevElementBuffer);
    glBindVertexArray(prevVAO);
    glUseProgram(prevProgram);

    for (int i = 0; i < 4; i++) {
        if (attribEnabled[i]) {
            glEnableVertexAttribArray(i);
        } else {
            glDisableVertexAttribArray(i);
        }
    }
}

void GroundPlane::cleanup() {
    if (vertexBufferID) glDeleteBuffers(1, &vertexBufferID);
    if (colorBufferID) glDeleteBuffers(1, &colorBufferID);
    if (indexBufferID) glDeleteBuffers(1, &indexBufferID);
    if (vertexArrayID) glDeleteVertexArrays(1, &vertexArrayID);
    if (uvBufferID) glDeleteBuffers(1, &uvBufferID);
}