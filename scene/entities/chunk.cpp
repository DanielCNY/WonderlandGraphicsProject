#include "chunk.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Chunk::Chunk(int x, int z) : chunkX(x), chunkZ(z) {
}

void Chunk::initialize() {
    ground.initialize();

    if (!tree.loadModel("../scene/entities/models/winter_fir.gltf")) {
        std::cerr << "Failed to load tree model" << std::endl;
    }
}

void Chunk::render(const glm::mat4& viewProjectionMatrix) {
    glm::mat4 groundModelMatrix = glm::mat4(1.0f);
    groundModelMatrix = glm::translate(groundModelMatrix,
                                      glm::vec3(chunkX * SIZE, 0.0f, chunkZ * SIZE));
    glm::mat4 groundMvp = viewProjectionMatrix * groundModelMatrix;
    ground.render(groundMvp);

    float centerX = chunkX * SIZE - SIZE / 2.0f + 60;
    float centerZ = chunkZ * SIZE + SIZE / 2.0f - 20;

    glm::mat4 treeModelMatrix = glm::mat4(1.0f);
    treeModelMatrix = glm::translate(treeModelMatrix,
                                    glm::vec3(centerX, 5.0f, centerZ));
    treeModelMatrix = glm::rotate(treeModelMatrix, glm::radians(90.0f),
                                 glm::vec3(-1.0f, 0.0f, 0.0f));

    glm::mat4 treeMvp = viewProjectionMatrix * treeModelMatrix;
    tree.render(treeMvp);
}