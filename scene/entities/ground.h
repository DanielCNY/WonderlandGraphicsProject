#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>

class GroundPlane {
private:
    GLuint modelMatrixID;
    GLuint viewProjectionMatrixID;
    GLuint lightPositionID;
    GLuint lightIntensityID;
    GLuint viewPositionID;

    GLuint programID;
    GLuint mvpMatrixID;
    GLuint textureSamplerID;
    GLuint textureID;
    GLuint vertexArrayID;
    GLuint vertexBufferID;
    GLuint uvBufferID;
    GLuint indexBufferID;
    GLuint normalBufferID;

    
public:
    GroundPlane();
    ~GroundPlane();
    
    void initialize();
    void render(const glm::mat4& modelMatrix , const glm::mat4& viewProjectionMatrix, const glm::vec3& lightPosition,
                         const glm::vec3& lightIntensity, const glm::vec3& viewPosition);
    void restoreState(GLint program, GLint vao, GLint arrayBuffer,
                            GLint elementBuffer, GLint attribEnabled[4]);
    void cleanup();

    static GLuint loadGroundShaders();
};