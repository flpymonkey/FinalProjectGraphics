#include "loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Loader::Loader() {
    // TODO:
};

Loader::~Loader() {
    // TODO:
};

void Loader::loadObj(const char* path, 
    std::vector<glm::vec4>& vertices,
    std::vector<glm::vec2>& uvs,
    std::vector<glm::vec4>& normals,
    std::vector<glm::uvec3>& faces) {
            
    Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(path, 0);
	if (!scene) {
		fprintf(stderr, importer.GetErrorString());
		getchar();
		return;
	}
    
	const aiMesh* mesh = scene->mMeshes[0];

	// Vertices.
    vertices.reserve(mesh->mNumVertices);
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		aiVector3D v = mesh->mVertices[i];
		vertices.push_back(glm::vec4(v.x, v.y, v.z, 1.0f));
	}
    
    // Uvs.
    uvs.reserve(mesh->mNumVertices);
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        // One set.
		aiVector3D uv = mesh->mTextureCoords[0][i];
		uvs.push_back(glm::vec2(uv.x, uv.y));
	}

	// Normals.
    normals.reserve(mesh->mNumVertices);
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		aiVector3D n = mesh->mNormals[i];
		normals.push_back(glm::vec4(n.x, n.y, n.z, 0.0f));
	}

	// Faces.
    faces.reserve(3 * mesh->mNumFaces);
	for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        // Triangles.
        faces.push_back(glm::uvec3(face.mIndices[0], face.mIndices[1], face.mIndices[2]));
	} 
};


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
