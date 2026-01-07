#ifndef WORLD_MANAGER_H
#define WORLD_MANAGER_H
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <unordered_map>
#include <utility>

class Chunk;

struct PairHash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const {
        auto hash1 = std::hash<T1>{}(p.first);
        auto hash2 = std::hash<T2>{}(p.second);
        return hash1 ^ (hash2 << 1);
    }
};

class WorldManager {
public:
    WorldManager();
    ~WorldManager();

    void update(const glm::vec3& cameraPos, const float& deltaTime, float globalTime);
    void render(const glm::mat4& viewProjectionMatrix, const glm::vec3& lightPosition,
                        const glm::vec3& lightIntensity, const glm::vec3& viewPosition);
    void setMarkedForRemoval(bool marked) { markedForRemoval = marked; }
    bool isMarkedForRemoval() const { return markedForRemoval; }

private:
    std::unordered_map<std::pair<int, int>, std::unique_ptr<Chunk>, PairHash> chunkMap;
    int centerChunkX;
    int centerChunkZ;
    bool initialized;
    bool markedForRemoval = false;

    int getChunkCoord(float worldCoord);
    void updateChunkGrid();
};

#endif