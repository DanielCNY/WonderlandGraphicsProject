#ifndef GROUND_H
#define GROUND_H

#include <glm/glm.hpp>
#include <glad/gl.h>

class GroundPlane {
public:
    GroundPlane();
    ~GroundPlane();

    void initialize();
    void render(const glm::mat4& viewProjectionMatrix);
    void cleanup();

private:
    GLuint vertexArrayID;
    GLuint vertexBufferID;
    GLuint indexBufferID;
    GLuint colorBufferID;
    GLuint uvBufferID;
    GLuint textureID;

    GLuint mvpMatrixID;
    GLuint textureSamplerID;
    GLuint programID;

    static const int NUM_VERTICES = 4;
    static const int NUM_INDICES = 6;
};

#endif