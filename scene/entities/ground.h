#ifndef GROUND_H
#define GROUND_H

#include <glm/glm.hpp>
#include <glad/gl.h>

class GroundPlane {
public:
    void initialize();
    void render(glm::mat4 cameraMatrix);
    void cleanup();

private:
    static GLuint sharedProgramID;
    static GLuint sharedMvpMatrixID;
    static GLuint sharedTextureSamplerID;
    static bool shadersLoaded;

    GLuint vertexArrayID = 0;
    GLuint vertexBufferID = 0;
    GLuint indexBufferID = 0;
    GLuint colorBufferID = 0;
    GLuint uvBufferID = 0;
    GLuint textureID = 0;

    GLuint mvpMatrixID = 0;
    GLuint textureSamplerID = 0;
    GLuint programID = 0;
};

#endif