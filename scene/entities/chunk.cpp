#include "chunk.h"
#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <algorithm>
#include <iostream>

Chunk::Chunk(int x, int z) : chunkX(x), chunkZ(z) {
    seed = x * 1000 + z;
}

void Chunk::initialize() {
    ground.initialize();

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> countDist(0, 9);
    numTrees = countDist(rng);

    if (!tree.loadModel("../scene/entities/models/winter_fir.gltf")) {
        std::cerr << "Failed to load tree model" << std::endl;
    }
    generateTrees();
}

void Chunk::generateTrees()
{
    if (numTrees == 0) return;

    std::vector<glm::vec2> corners = {
        glm::vec2(0.0f, 0.0f),
        glm::vec2(250.0f, 0.0f),
        glm::vec2(0.0f, 250.0f),
        glm::vec2(250.0f, 250.0f),
        glm::vec2(250.0f, -250.0f),
        glm::vec2(-250.0f, 250.0f),
        glm::vec2(-250.0f, 0.0f),
        glm::vec2(0.0f, -250.0f),
        glm::vec2(-250.0f, -250.0f)
    };
    std::mt19937 rng(seed);
    std::shuffle(corners.begin(), corners.end(), rng);

    for (int i = 0; i < numTrees; i++) {
        Transformation t {glm::vec3(corners[i].x, 0.0f, corners[i].y), 90.0f, 1.0f };
        treeTransforms.push_back(t);
    }
}

void Chunk::render(const glm::mat4& viewProjectionMatrix) {
    glm::mat4 groundModelMatrix = glm::mat4(1.0f);
    groundModelMatrix = glm::translate(groundModelMatrix,
                                      glm::vec3(chunkX * SIZE, 0.0f, chunkZ * SIZE));
    glm::mat4 groundMvp = viewProjectionMatrix * groundModelMatrix;
    ground.render(groundMvp);

    float centerX = chunkX * SIZE - SIZE / 2.0f + 300;
    float centerZ = chunkZ * SIZE + SIZE / 2.0f - 300;

    for (auto& t : treeTransforms)
    {
        glm::mat4 treeModelMatrix = glm::mat4(1.0f);
        treeModelMatrix = glm::translate(treeModelMatrix, glm::vec3(centerX + t.translation.x,
                               5.0f + t.translation.y, centerZ + t.translation.z));
        treeModelMatrix = glm::rotate(treeModelMatrix, glm::radians(t.rotation),
                                glm::vec3(-1.0f, 0.0f, 0.0f));
        treeModelMatrix = glm::scale(treeModelMatrix, glm::vec3(t.scale, t.scale, t.scale));

        glm::mat4 treeMvp = viewProjectionMatrix * treeModelMatrix;
        tree.render(treeMvp);
    }
}
