#include "loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Loader::Loader() {
    // TODO:
};

Loader::~Loader() {
    // TODO:
};

void Loader::loadObj(const char* path, std::vector<Mesh>& meshes, std::vector<Material>& materials) {
            
    Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(path, 0);
	if (!scene) {
		fprintf(stderr, importer.GetErrorString());
		getchar();
		return;
	}
    
    if (scene->HasAnimations()) {
        printf("Loader::loadObj(): warning scene contains unsupported animations!\n");
    }
    
    if (scene->HasCameras()) {
        printf("Loader::loadObj(): warning scene contains unsupported cameras!\n");
    }
    
    if (scene->HasLights()) {
        printf("Loader::loadObj(): warning scene contains unsupported lights!\n");
    }
    
    if (scene->HasMeshes()) {
        getMeshes(scene, meshes);
    } else {
        printf("Loader::loadObj(): warning scene does not contain meshes!\n");
    }
    
    if (scene->HasMaterials()) {
        getMaterials(path, scene, meshes, materials);
    } else {
        printf("Loader::loadObj(): warning scene does not contain materials!\n");
    }
    
    if (scene->HasTextures()) {
        printf("Loader::loadObj(): warning scene contains unsupported textures!\n");
    } 
};

void Loader::getMeshes(const aiScene* scene, std::vector<Mesh>& meshes) {
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        const aiMesh* mesh = scene->mMeshes[i];
        getMesh(mesh, meshes);
    }
};

void Loader::getMesh(const aiMesh* mesh, std::vector<Mesh>& meshes) {
    Mesh m;
    
    if (mesh->HasBones()) {
        printf("Loader::getMesh(): warning mesh contains unsupported bones!\n");
    }
    
    if (mesh->HasPositions()) {
        m.vertices.reserve(mesh->mNumVertices);
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            aiVector3D v = mesh->mVertices[i];
            m.vertices.push_back(glm::vec4(v.x, v.y, v.z, 1.0f));
        }
    } else {
        printf("Loader::getMesh(): warning mesh does not contain positions!\n");
    }
    
    if (mesh->HasNormals()) {
        m.normals.reserve(mesh->mNumVertices);
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            aiVector3D n = mesh->mNormals[i];
            m.normals.push_back(glm::vec4(n.x, n.y, n.z, 0.0f));
        }
    } else {
        m.normals.reserve(mesh->mNumVertices);
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            m.normals.push_back(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
        }
        printf("Loader::getMesh(): warning mesh does not contain normals! Using default.\n");
    }
    
    if (mesh->HasFaces()) {
        m.faces.reserve(3 * mesh->mNumFaces);
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            // Triangles.
            m.faces.push_back(glm::uvec3(face.mIndices[0], face.mIndices[1], face.mIndices[2]));
        }
    } else {
        printf("Loader::getMesh(): warning mesh does not contain faces!\n");
    }
    
    if (mesh->HasTangentsAndBitangents()) {
        printf("Loader::getMesh(): warning mesh contains unsupported tangents and bitangents!\n");
    }
    
    unsigned int uv_channels = mesh->GetNumUVChannels();
    if (uv_channels > 1) {
        printf("Loader::getMesh(): warning mesh contains %d unsupported sets of texture coords!\n", uv_channels - 1);
    }
    
    if (mesh->HasTextureCoords(0)) {
        m.uvs.reserve(mesh->mNumVertices);
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            // One set.
            aiVector3D uv = mesh->mTextureCoords[0][i];
            m.uvs.push_back(glm::vec2(uv.x, uv.y));
        }
    } else {
        printf("Loader::getMesh(): warning mesh does not contain a set of uvs!\n");
    }
    
    unsigned int color_channels = mesh->GetNumColorChannels();
    if (color_channels > 0) {
        printf("Loader::getMesh(): warning mesh contains %d unsupported sets of vertex colors!\n", color_channels);
    }
    
    if (mesh->HasVertexColors(0)) {
        printf("Loader::getMesh(): warning mesh contains unsupported vertex colors!\n");
    }
    
    m.material_id = mesh->mMaterialIndex;
    meshes.push_back(m);
}

void Loader::getMaterials(const char* path, const aiScene* scene, std::vector<Mesh>& meshes, std::vector<Material>& materials) {
    for (unsigned int i = 0; i < meshes.size(); i++) {
        const aiMaterial* material = scene->mMaterials[meshes[i].material_id];
        getMaterial(path, material, materials);
    }
}

void Loader::getMaterial(const char* path, const aiMaterial* material, std::vector<Material>& materials) {
    Material m;
    
    if (material->GetTextureCount(aiTextureType_AMBIENT) > 0) {
        printf("Loader::getMaterial(): warning material contains unsupported ambient textures!\n"); 
    }
    
    if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
        for (unsigned int i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); i++) {
            aiString string;
            material->GetTexture(aiTextureType_DIFFUSE, i, &string);
            std::string p = mtl_path(path, std::string(string.C_Str()));
            m.diffuse_ids.push_back(loadTexture(p.c_str()));
        }
    } else {
        printf("Loader::getMaterial(): warning material does not contain diffuse textures!\n");
    }
    
    if (material->GetTextureCount(aiTextureType_SPECULAR) > 0) {
        for (unsigned int i = 0; i < material->GetTextureCount(aiTextureType_SPECULAR); i++) {
            aiString string;
            material->GetTexture(aiTextureType_SPECULAR, i, &string);
            std::string p = mtl_path(path, std::string(string.C_Str()));
            m.specular_ids.push_back(loadTexture(p.c_str()));
        }
    } else {
        printf("Loader::getMaterial(): warning material does not contain specular textures!\n");
    }
    
    if (material->GetTextureCount(aiTextureType_HEIGHT) > 0) {
        printf("Loader::getMaterial(): warning material contains unsupported height textures!\n"); 
    }
    
    materials.push_back(m);
}

unsigned int Loader::loadTexture(char const* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
};

