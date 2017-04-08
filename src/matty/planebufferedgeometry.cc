#include <iostream>
#include <vector>
#include "planebufferedgeometry.h"

#include "opengl/gl3w.h"

PlaneBufferedGeometry::PlaneBufferedGeometry(int w, int h, int ws, int hs) {

	// vertex array object
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// build plane using no intermediate storage
	auto segment_width = ((float)w / (float)ws);
	auto segment_height = ((float)h / (float)hs);

	// vertex buffer object
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	std::vector<float> vertices;

	// build vertices, normals and uvs
	for (auto i = 0; i <= hs; i++) {
		auto y = i * segment_height;
		for (auto j = 0; j <= ws; j++) {
			auto x = j * segment_width;
			
			vertices.push_back(x);
			vertices.push_back(y);
			vertices.push_back(0.0f);

			// vertex
			printf("vertex (%.2f, %.2f, %.2f)\n", x, y, 0.0f);

			// normal
			printf("normal (%.2f, %.2f, %.2f)\n", 0.0f, 0.0f, 1.0f);

			// uv
			printf("uv (%.2f)\n", ((float)j / (float)ws));
			printf("uv (%.2f)\n", (1 - ((float)i / (float)hs)));
		}
	}

	// buffer data
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// element buffer object
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

	std::vector<int> indices;

	// build indices
	for (auto i = 0; i < hs; i++) {
		for (auto j = 0; j < ws; j++) {
			auto a = j + (hs + 1) * i;
			auto b = j + (hs + 1) * (i + 1);
			auto c = (j + 1) + (hs + 1) * (i + 1);
			auto d = (j + 1) + (hs + 1) * i;

			printf("faces (%d, %d, %d)\n", a, b, d);
			printf("faces (%d, %d, %d)\n", b, c, d);

			// triangle one
			indices.push_back(a);
			indices.push_back(b);
			indices.push_back(d);

			// triangle two
			indices.push_back(b);
			indices.push_back(c);
			indices.push_back(d);
		}
	}

	// element buffer object
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), &indices[0], GL_STATIC_DRAW);

	// unbind vao
	glBindVertexArray(0);
}

PlaneBufferedGeometry::~PlaneBufferedGeometry() {

}

void PlaneBufferedGeometry::render() {
	glBindVertexArray(vao);
	glDrawElementsBaseVertex(GL_TRIANGLES, 9 * sizeof(int), GL_UNSIGNED_INT, 0, 0);
	glBindVertexArray(0);
}