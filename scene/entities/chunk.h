#ifndef CHUNK_H
#define CHUNK_H

#include <glad/gl.h>
#include <glm/glm.hpp>
#include "ground.h"
#include "entities/static_model.h"

struct Transformation {
    glm::vec3 translation;
    float rotation;
    float scale;
};

class Chunk {
public:
    static constexpr int SIZE = 1000;

    Chunk(int x, int z);
    void initialize();
    void generateTrees();
    void render(const glm::mat4& viewProjectionMatrix);
    bool isVisible(const glm::vec3& cameraPos) const;

private:
    int chunkX, chunkZ;
    int seed;
    int numTrees;
    GroundPlane ground;
    StaticModel tree;
    std::vector<Transformation> treeTransforms;
};

#endif