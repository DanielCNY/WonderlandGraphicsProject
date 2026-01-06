#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>

class GroundPlane {
private:
    GLuint programID;
    GLuint mvpMatrixID;
    GLuint textureSamplerID;
    GLuint textureID;
    GLuint vertexArrayID;
    GLuint vertexBufferID;
    GLuint colorBufferID;
    GLuint uvBufferID;
    GLuint indexBufferID;

    
public:
    GroundPlane();
    ~GroundPlane();
    
    void initialize();
    void render(glm::mat4 cameraMatrix);
    void restoreState(GLint program, GLint vao, GLint arrayBuffer,
                            GLint elementBuffer, GLint attribEnabled[4]);
    void cleanup();

    static GLuint loadGroundShaders();
};