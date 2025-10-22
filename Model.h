#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <string>
#include <iostream>
#include <limits>
#include <unordered_map>

// Корректные GLM-инклюды
#include <glm.hpp>
#include <matrix_transform.hpp>

// Assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include "Shader.h"

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
    bool init = false;

    AABB()
        : min(glm::vec3(std::numeric_limits<float>::max())),
        max(glm::vec3(std::numeric_limits<float>::lowest())),
        init(false) {}
    void expand(const aiVector3D& v) {
        min.x = std::min(min.x, v.x);
        min.y = std::min(min.y, v.y);
        min.z = std::min(min.z, v.z);
        max.x = std::max(max.x, v.x);
        max.y = std::max(max.y, v.y);
        max.z = std::max(max.z, v.z);
        init = true;
    }
    glm::vec3 center() const {
        return (min + max) * 0.5f;
    }
};

class Model {
public:
    // Центры интересующих узлов (по итогам AABB)
    glm::vec3 plecho_center = glm::vec3(0.0f);
    glm::vec3 kyst_center = glm::vec3(0.0f);

    std::vector<Mesh> meshes;
    std::vector<glm::mat4> meshTransforms;
    std::string directory;

    // Дополнительно: карты имени меша -> индекс/границы
    std::vector<std::string> meshNames;  // по порядку добавления meshes
    std::unordered_map<std::string, AABB> nameToAABB;

    Model(std::string const& path) {
        loadModel(path);
        meshTransforms.resize(meshes.size(), glm::mat4(1.0f));

        // === Присвоение центров по известным именам мешей ===
        // Для твоей модели manipulatorlab3.obj:
        // plecho_center <- "Cube.001" (первая секция)
        // kyst_center   <- "Cube"     (эффектор)
        auto itP = nameToAABB.find("Cube.002");
        if (itP != nameToAABB.end() && itP->second.init) {
            plecho_center = itP->second.center();
        }
        auto itK = nameToAABB.find("Cube.003");
        if (itK != nameToAABB.end() && itK->second.init) {
            kyst_center = itK->second.center();
        }
        // Можно добавить fallback-логики, если имена иные — по желанию.
    }

    void Draw(Shader& shader) {
        for (size_t i = 0; i < meshes.size(); i++) {
            shader.setMat4("model", meshTransforms[i]);
            meshes[i].Draw(shader);
        }
    }

    void UpdateTransform(int meshIndex, const glm::mat4& transform) {
        if (meshIndex >= 0 && meshIndex < (int)meshTransforms.size()) {
            meshTransforms[meshIndex] = transform;
        }
    }

private:
    void loadModel(std::string const& path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            path,
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_FlipUVs
            // при желании: | aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality
        );

        if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
            std::cerr << "ASSIMP ERROR: " << importer.GetErrorString() << std::endl;
            return;
        }

        // Каталог (на случай относительных путей к текстурам)
        size_t slashPos = path.find_last_of("/\\");
        directory = (slashPos == std::string::npos) ? "" : path.substr(0, slashPos);

        processNode(scene->mRootNode, scene);
    }

    void processNode(aiNode* node, const aiScene* scene) {
        // Обрабатываем все меши текущего узла
        for (unsigned int m = 0; m < node->mNumMeshes; m++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[m]];

            // Имя меша: предпочитаем mName меша, иначе имя узла
            std::string meshName = mesh->mName.C_Str();
            if (meshName.empty()) {
                meshName = node->mName.C_Str();
            }
            meshNames.push_back(meshName);

            // 1) Собираем геометрию в наш Mesh
            meshes.push_back(processMesh(mesh, scene));

            // 2) Считаем AABB для ЭТОГО меша корректно (min/max вне цикла вершин)
            AABB aabbAccum; // локальная коробка для данного меша
            for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
                aabbAccum.expand(mesh->mVertices[i]);
            }
            // Накапливаем/сливаем AABB по имени
            auto& box = nameToAABB[meshName];
            if (!box.init) {
                box = aabbAccum;
            }
            else {
                // вдруг имя повторится на разных узлах — расширим общий AABB
                box.min = glm::min(box.min, aabbAccum.min);
                box.max = glm::max(box.max, aabbAccum.max);
                box.init = true;
            }
        }

        // Отладка (сколько мешей у узла)
        // printf("node '%s' mcount = %u\n", node->mName.C_Str(), node->mNumMeshes);

        // Рекурсивно обрабатываем детей
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* /*scene*/) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        vertices.reserve(mesh->mNumVertices);

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            vertex.Position = glm::vec3(
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z
            );

            if (mesh->HasNormals()) {
                vertex.Normal = glm::vec3(
                    mesh->mNormals[i].x,
                    mesh->mNormals[i].y,
                    mesh->mNormals[i].z
                );
            }
            else {
                vertex.Normal = glm::vec3(0.0f, 0.0f, 1.0f);
            }

            // Если нужны UV — добавить:
            // if (mesh->HasTextureCoords(0)) { ... }

            vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            const aiFace& face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }

        return Mesh(vertices, indices);
    }
};

#endif // MODEL_H
