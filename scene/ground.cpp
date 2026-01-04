#include <ground.h>
#include <texture_manager.h>
#include <./render/shader.h>

void GroundPlane::initialize() {
    GLfloat vertex_buffer_data[12] = {
        -250.0f, 0.0f, -250.0f,
        -250.0f, 0.0f, 250.0f,
        250.0f, 0.0f, 250.0f,
        250.0f, 0.0f, -250.0f,
    };

    GLfloat color_buffer_data[12] = {
        0.8f, 1.0f, 0.7f,
        1.0f, 1.0f, 0.9f,
        0.9f, 0.95f, 0.8f,
        0.8f, 1.0f, 0.7f,
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

    glGenBuffers(1, &colorBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(color_buffer_data), color_buffer_data, GL_STATIC_DRAW);

    glGenBuffers(1, &uvBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uv_buffer_data), uv_buffer_data, GL_STATIC_DRAW);

    glGenBuffers(1, &indexBufferID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer_data), index_buffer_data, GL_STATIC_DRAW);

    programID = LoadShadersFromFile("../scene/shaders/ground.vert", "../scene/shaders/ground.frag");
    mvpMatrixID = glGetUniformLocation(programID, "MVP");

    TextureManager& tm = TextureManager::getInstance();
    textureID = tm.getTexture("../scene/textures/snowy_ground.jpg");

    textureSamplerID = glGetUniformLocation(programID,"textureSampler");
}

void GroundPlane::render(glm::mat4 cameraMatrix) {
    glUseProgram(programID);

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
}

void GroundPlane::cleanup() {
    if (vertexBufferID) glDeleteBuffers(1, &vertexBufferID);
    if (colorBufferID) glDeleteBuffers(1, &colorBufferID);
    if (indexBufferID) glDeleteBuffers(1, &indexBufferID);
    if (vertexArrayID) glDeleteVertexArrays(1, &vertexArrayID);
    if (uvBufferID) glDeleteBuffers(1, &uvBufferID);
    if (programID) glDeleteProgram(programID);
}