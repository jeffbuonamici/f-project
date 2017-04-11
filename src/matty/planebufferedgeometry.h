#pragma once

#include "bufferedgeometry.h"

class PlaneBufferedGeometry : public BufferedGeometry {
public:
	PlaneBufferedGeometry() : vao(0), vbo(0), ebo(0) {};
	PlaneBufferedGeometry(int, int, int, int);
	~PlaneBufferedGeometry();

	void render();

private: // private variables
	GLuint vao, vbo, ebo;

	// buffers
	std::vector<int> indices;
	std::vector<float> vertices;
};	// planebufferedgeometry
