#include "chunk.h"

#include "camera.h"

Chunk::Chunk(size_t x, size_t y, const TerrainTexManager& terrainTexManager,
	                                     TerrainGeometry&   terrainGeometry) :
	_chunkPos(x,y),
	_visible(false),
	_currentSubdivLvl(1),
  _terrainTexManager(terrainTexManager),
  _terrainGeometry(terrainGeometry) {

	for (size_t i = 0; i < MAX_SUBDIV_LVL+1; i++) {
    _subdivisionLevels.push_back(std::unique_ptr<Buffers>(new Buffers()));
  }
}

void Chunk::fillBufferData() {
	std::list<const Triangle*> triangles = _terrainGeometry.getTrianglesInChunk(_chunkPos.x, _chunkPos.y, _currentSubdivLvl);
	std::list<Vertex*> vertices = TerrainGeometry::SubdivisionLevel::getVertices(triangles);

	Buffers* currentBuffers = _subdivisionLevels[_currentSubdivLvl].get();

	currentBuffers->vertices.resize(vertices.size() * 3);
	currentBuffers->normals.resize(vertices.size() * 3);
	currentBuffers->coords.resize(vertices.size() * 2);

	std::map<Vertex*, size_t> verticesArrayIndices;

	size_t vertIndex = 0;

	for (auto vert = vertices.begin(); vert != vertices.end(); vert++) {
		for (size_t i = 0; i < 3; i++) {
			currentBuffers->vertices[3*vertIndex + i] = (*vert)->pos[i];
			currentBuffers->normals[3*vertIndex + i] = (*vert)->normal[i];
		}

		currentBuffers->coords[2*vertIndex] =
		((*vert)->pos.x - CHUNK_SIZE*_chunkPos.x)/CHUNK_SIZE*TEX_FACTOR;
		currentBuffers->coords[2*vertIndex + 1] =
		((*vert)->pos.y - CHUNK_SIZE*_chunkPos.y)/CHUNK_SIZE*TEX_FACTOR;

		verticesArrayIndices[*vert] = vertIndex;

		vertIndex++;
	}

	for (auto tri = triangles.begin(); tri != triangles.end(); tri++) {
		std::vector<GLuint>& currentIndices = currentBuffers->indicesInfo[(*tri)->biome].indices;

		currentIndices.resize(currentIndices.size() + 3);

		for (size_t i = 0; i < 3; i++) {
			currentIndices[currentIndices.size()-3+i] = verticesArrayIndices.at((*tri)->vertices[i]);
		}
	}
}

void Chunk::generateBuffers() {
	Buffers* currentBuffers = _subdivisionLevels[_currentSubdivLvl].get();

	// geometry VBO

	currentBuffers->vbo.bind();

	size_t bufferSizeVertices = currentBuffers->vertices.size()*sizeof currentBuffers->vertices[0];
	size_t bufferSizeNormals	= currentBuffers->normals. size()*sizeof currentBuffers->normals[0];
	size_t bufferSizeCoords		= currentBuffers->coords.  size()*sizeof currentBuffers->coords[0];

	glBufferData(GL_ARRAY_BUFFER, bufferSizeVertices + bufferSizeNormals + bufferSizeCoords, NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, bufferSizeVertices , &currentBuffers->vertices[0]);
  glBufferSubData(GL_ARRAY_BUFFER,    bufferSizeVertices , bufferSizeNormals, &currentBuffers->normals[0]);
	glBufferSubData(GL_ARRAY_BUFFER,    bufferSizeVertices + bufferSizeNormals, bufferSizeCoords, &currentBuffers->coords[0]);

  VertexBufferObject::unbind();

	// VAO

	currentBuffers->vao.bind();
	currentBuffers->vbo.bind();

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(currentBuffers->vertices.size()*sizeof currentBuffers->vertices[0]));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(currentBuffers->vertices.size()*sizeof currentBuffers->vertices[0] +
		              currentBuffers->normals. size()*sizeof currentBuffers->normals[0]));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	VertexBufferObject::unbind();
	VertexArrayObject::unbind();

	// IBO for each biome

	for (auto it = currentBuffers->indicesInfo.begin(); it != currentBuffers->indicesInfo.end(); it++) {
		size_t bufferSizeIndices = it->second.indices.size()*sizeof it->second.indices[0];

		it->second.ibo.bind();

		glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferSizeIndices, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, bufferSizeIndices, &(it->second.indices[0]));

		IndexBufferObject::unbind();
	}

	currentBuffers->generated = true;
}

void Chunk::computeChunkBoundingBox() {
	Buffers* currentBuffers = _subdivisionLevels[_currentSubdivLvl].get();

	float minCoord[3];
	float maxCoord[3];

	for (size_t i = 0; i < 3; i++) {
		minCoord[i] = std::numeric_limits<float>::max();
		maxCoord[i] = - std::numeric_limits<float>::max();
	}

	for (size_t i = 0; i < currentBuffers->vertices.size(); i+=3) {
		for (size_t j = 0; j < 3; j++) {
			if (currentBuffers->vertices[i+j] < minCoord[j])
				minCoord[j] = currentBuffers->vertices[i+j];
			if (currentBuffers->vertices[i+j] > maxCoord[j])
				maxCoord[j] = currentBuffers->vertices[i+j];
		}
	}

	currentBuffers->corners[0] = glm::vec3(minCoord[0], minCoord[1], minCoord[2]);
	currentBuffers->corners[1] = glm::vec3(minCoord[0], minCoord[1], maxCoord[2]);
	currentBuffers->corners[2] = glm::vec3(minCoord[0], maxCoord[1], minCoord[2]);
	currentBuffers->corners[3] = glm::vec3(minCoord[0], maxCoord[1], maxCoord[2]);
	currentBuffers->corners[4] = glm::vec3(maxCoord[0], minCoord[1], minCoord[2]);
	currentBuffers->corners[5] = glm::vec3(maxCoord[0], minCoord[1], maxCoord[2]);
	currentBuffers->corners[6] = glm::vec3(maxCoord[0], maxCoord[1], minCoord[2]);
	currentBuffers->corners[7] = glm::vec3(maxCoord[0], maxCoord[1], maxCoord[2]);
	_centerOfChunk = glm::vec3((_chunkPos.x+0.5)*CHUNK_SIZE, (_chunkPos.y+0.5)*CHUNK_SIZE,
	    	 getHeight(glm::vec2((_chunkPos.x+0.5)*CHUNK_SIZE, (_chunkPos.y+0.5)*CHUNK_SIZE)));
}

void Chunk::generate() {
	if (!_subdivisionLevels[_currentSubdivLvl]->generated) {
		fillBufferData();
		generateBuffers();
		computeChunkBoundingBox();
	}
}

size_t Chunk::draw() const {
	Buffers* currentBuffers = _subdivisionLevels[_currentSubdivLvl].get();

	currentBuffers->vao.bind();

	size_t nbTriangles = 0;

	for (auto it = currentBuffers->indicesInfo.begin(); it != currentBuffers->indicesInfo.end(); it++) {
		_terrainTexManager.bindTexture(it->first);

		it->second.ibo.bind();

		glDrawElements(GL_TRIANGLES, it->second.indices.size(), GL_UNSIGNED_INT, BUFFER_OFFSET(0));

		IndexBufferObject::unbind();

		nbTriangles += it->second.indices.size() / 3;
	}

	Texture::unbind();

	VertexArrayObject::unbind();

	return nbTriangles;
}

bool Chunk::theCornersAreOutside(const glm::vec3& cam, const glm::vec3& vec) const {
  float dots[8];

  for (size_t i = 0 ; i < 8 ; i++) {
    dots[i] = glm::dot(_subdivisionLevels[_currentSubdivLvl]->corners[i]-cam, vec);

		if (dots[i] < 0.f)
			return false;
  }

  return true;
}

void Chunk::computeCulling(const std::vector<glm::vec3>& planeNormals) {
	Camera& cam = Camera::getInstance();
  glm::vec3 pos = cam.getPos();

	for (size_t i = 0; i < planeNormals.size(); i++) {
		if (theCornersAreOutside(pos, planeNormals[i])) {
			_visible = false;
			return;
		}
	}

  _visible = true;
}
void Chunk::setTreesHeights() {
	for (size_t i = 0; i < _trees.size(); i++) {
		_trees[i]->setHeight(getHeight(_trees[i]->getPos()));
	}
}

void Chunk::setTrees(std::vector<igElement*> trees) {
	_trees = trees;
	setTreesHeights();
	loadTrees();
}

void Chunk::loadTrees() {
	_treeDrawer.loadElements(_trees, true);
}

void Chunk::computeDistanceOptimizations() {
	Camera& camera = Camera::getInstance();

	float distanceToChunk = glm::length(camera.getPos()-_centerOfChunk);

	if (distanceToChunk > 2000)
		_treesNeedTwoPasses = false;
	else
		_treesNeedTwoPasses = true;

#ifdef __ANDROID__
	if (distanceToChunk > 2000)
#else
	if (distanceToChunk > 4000)
#endif
		_displayMovingElements = false;
	else
		_displayMovingElements = true;

	if (distanceToChunk > 10000)
		setSubdivisionLevel(1);
	else if (distanceToChunk > 4000)
		setSubdivisionLevel(2);
	else if (distanceToChunk > 1000)
		setSubdivisionLevel(3);
	else
		setSubdivisionLevel(4);
}

void Chunk::setSubdivisionLevel(size_t newSubdLvl) {
	if (newSubdLvl != _currentSubdivLvl) {
		_currentSubdivLvl = newSubdLvl;
		generate();
		setTrees(_trees);
	}
}

float Chunk::getHeight(glm::vec2 pos) const {
  return _terrainGeometry.getHeight(pos, _currentSubdivLvl);
}

glm::vec3 Chunk::getNorm(glm::vec2 pos) const {
	return _terrainGeometry.getNorm(pos, _currentSubdivLvl);
}
