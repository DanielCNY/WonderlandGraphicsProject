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
    std::uniform_int_distribution<int> spawnDist(0, 5);
    giantAppear = (spawnDist(rng) == 3);
    caneAppear = (spawnDist(rng) < 3);

    if (numTrees == 0 && giantAppear) {
        if (!bot.loadModel("../scene/entities/models/bot/bot.gltf")) {
            std::cerr << "Failed to load bot model" << std::endl;
        }
    }
    else if (numTrees == 0 && caneAppear) {
        if (!cane.loadModel("../scene/entities/models/candy_cane/cane.gltf")) {
            std::cerr << "Failed to load candy_cane model" << std::endl;
        }
    }
    else {
        if (!tree.loadModel("../scene/entities/models/fir_tree/winter_fir.gltf")) {
            std::cerr << "Failed to load fir_tree model" << std::endl;
        }
        generateTrees();
    }

}

void Chunk::update(float deltaTime, float globalTime) {
    if (numTrees == 0 && giantAppear) {
        bot.update(deltaTime, globalTime);
    }
}

void Chunk::generateTrees()
{
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

    std::uniform_real_distribution<float> scaleDist(0.7f, 1.3f);

    std::uniform_real_distribution<float> rotationDist(0.0f, 360.0f);

    for (int i = 0; i < numTrees; i++) {
        Transformation t {glm::vec3(corners[i].x, 0.0f, corners[i].y), rotationDist(rng), scaleDist(rng) };
        treeTransforms.push_back(t);
    }
}

void Chunk::render(const glm::mat4& viewProjectionMatrix, const glm::vec3& lightPosition,
                            const glm::vec3& lightIntensity, const glm::vec3& viewPosition) {
    glm::mat4 groundModelMatrix = glm::mat4(1.0f);
    groundModelMatrix = glm::translate(groundModelMatrix,
                                      glm::vec3(chunkX * SIZE, 0.0f, chunkZ * SIZE));
    glm::mat4 groundMvp = viewProjectionMatrix * groundModelMatrix;
    ground.render(groundModelMatrix, viewProjectionMatrix, lightPosition, lightIntensity, viewPosition);

    float centerX = chunkX * SIZE - (SIZE / 2.0f) + 300.0f;
    float centerZ = chunkZ * SIZE + (SIZE / 2.0f) - 300.0f;

    if (numTrees == 0 && giantAppear) {
        glm::mat4 botModelMatrix = glm::mat4(1.0f);
        botModelMatrix = glm::translate(botModelMatrix, glm::vec3(centerX, -135.0f, centerZ));
        botModelMatrix = glm::scale(botModelMatrix, glm::vec3(4.0f, 4.0f, 4.0f));
        bot.render(botModelMatrix, viewProjectionMatrix, lightPosition, lightIntensity, viewPosition);
    }
    else if (numTrees == 0 && caneAppear) {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<float> scaleDist(50.0f, 150.0f);
        std::uniform_real_distribution<float> rotationDist(0.0f, 360.0f);
        float newScale = scaleDist(rng);
        glm::mat4 caneModelMatrix = glm::mat4(1.0f);
        caneModelMatrix = glm::translate(caneModelMatrix, glm::vec3(centerX, -50.0f, centerZ));
        caneModelMatrix = glm::rotate(caneModelMatrix, glm::radians(90.0f),
                                    glm::vec3(-1.0f, 0.0f, 0.0f));
        caneModelMatrix = glm::rotate(caneModelMatrix, glm::radians(rotationDist(rng)),
                                    glm::vec3(0.0f, 0.0f, 1.0f));
        caneModelMatrix = glm::scale(caneModelMatrix, glm::vec3(newScale, newScale, newScale));
        cane.render(caneModelMatrix, viewProjectionMatrix, lightPosition, lightIntensity, viewPosition);
    }
    else if (numTrees > 0)
    {
        for (auto& t : treeTransforms)
        {
            glm::mat4 treeModelMatrix = glm::mat4(1.0f);
            treeModelMatrix = glm::translate(treeModelMatrix, glm::vec3(centerX + t.translation.x,
                                   t.translation.y, centerZ + t.translation.z));
            treeModelMatrix = glm::rotate(treeModelMatrix, glm::radians(90.0f),
                                    glm::vec3(-1.0f, 0.0f, 0.0f));
            treeModelMatrix = glm::rotate(treeModelMatrix, glm::radians(t.rotation),
                                    glm::vec3(0.0f, 0.0f, 1.0f));
            treeModelMatrix = glm::scale(treeModelMatrix, glm::vec3(t.scale, t.scale, t.scale));

            glm::mat4 treeMvp = viewProjectionMatrix * treeModelMatrix;
            tree.render(treeModelMatrix, viewProjectionMatrix, lightPosition, lightIntensity, viewPosition);
        }
    }
}
