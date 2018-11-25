#include "loader.h"

Loader::Loader() {
    // TODO:
};

Loader::~Loader() {
    // TODO:
};

void Loader::loadObj(const char* path, 
    std::vector<glm::vec4>& vertices, 
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
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		aiVector3D v = mesh->mVertices[i];
		vertices.push_back(glm::vec4(v.x, v.y, v.z, 1.0f));
	}

	// Normals.
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		aiVector3D n = mesh->mNormals[i];
		normals.push_back(glm::vec4(n.x, n.y, n.z, 0.0f));
	}

	// Faces.
	for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        // Triangles.
        faces.push_back(glm::uvec3(face.mIndices[0], face.mIndices[1], face.mIndices[2]));
	} 
};