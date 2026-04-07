#include <iostream>
#include <string>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "Image.h"

// This allows you to skip the `std::` in front of C++ standard library
// functions. You can also say `using std::cout` to be more selective.
// You should never do this in a header file.
using namespace std;

double RANDOM_COLORS[7][3] = {
	{0.0000,    0.4470,    0.7410},
	{0.8500,    0.3250,    0.0980},
	{0.9290,    0.6940,    0.1250},
	{0.4940,    0.1840,    0.5560},
	{0.4660,    0.6740,    0.1880},
	{0.3010,    0.7450,    0.9330},
	{0.6350,    0.0780,    0.1840},
};

int main(int argc, char **argv)
{
	if(argc < 6) {
		cout << "Usage: A1 meshfile, output, width, height, task" << endl;
		return 0;
	}
	string meshName(argv[1]);
	//Output image
	string outputName(argv[2]);
	// Width of image
	int width = atoi(argv[3]);
	// Height of image
	int height = atoi(argv[4]);
	//Task number 
	int TaskNum = atoi(argv[5]);

	// Load geometry
	vector<float> posBuf; // list of vertex positions
	vector<float> norBuf; // list of vertex normals
	vector<float> texBuf; // list of vertex texture coords
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	string warnStr, errStr;
	bool rc = tinyobj::LoadObj(&attrib, &shapes, &materials, &warnStr, &errStr, meshName.c_str());
	if(!rc) {
		cerr << errStr << endl;
	} else {
		// Some OBJ files have different indices for vertex positions, normals,
		// and texture coordinates. For example, a cube corner vertex may have
		// three different normals. Here, we are going to duplicate all such
		// vertices.
		// Loop over shapes
		for(size_t s = 0; s < shapes.size(); s++) {
			// Loop over faces (polygons)
			size_t index_offset = 0;
			for(size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
				size_t fv = shapes[s].mesh.num_face_vertices[f];
				// Loop over vertices in the face.
				for(size_t v = 0; v < fv; v++) {
					// access to vertex
					tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
					posBuf.push_back(attrib.vertices[3*idx.vertex_index+0]);
					posBuf.push_back(attrib.vertices[3*idx.vertex_index+1]);
					posBuf.push_back(attrib.vertices[3*idx.vertex_index+2]);
					if(!attrib.normals.empty()) {
						norBuf.push_back(attrib.normals[3*idx.normal_index+0]);
						norBuf.push_back(attrib.normals[3*idx.normal_index+1]);
						norBuf.push_back(attrib.normals[3*idx.normal_index+2]);
					}
					if(!attrib.texcoords.empty()) {
						texBuf.push_back(attrib.texcoords[2*idx.texcoord_index+0]);
						texBuf.push_back(attrib.texcoords[2*idx.texcoord_index+1]);
					}
				}
				index_offset += fv;
				// per-face material (IGNORE)
				shapes[s].mesh.material_ids[f];
			}
		}
	}
	cout << "Number of vertices: " << posBuf.size()/3 << endl;

	//Task 8: Rotation
	if (TaskNum == 8) {
		// Rotation angle
		float cosTheta = 0.70710678f;  // cos(45°)
		float sinTheta = 0.70710678f;  // sin(45°)

		// Rotate all vertices
		for (size_t i = 0; i < posBuf.size(); i += 3) {
			float x0 = posBuf[i];
			float y0 = posBuf[i + 1];
			float z0 = posBuf[i + 2];

			float x = cosTheta * x0 + sinTheta * z0;
			float y = y0;
			float z = -sinTheta * x0 + cosTheta * z0;

			posBuf[i] = x;
			posBuf[i + 1] = y;
			posBuf[i + 2] = z;
		}

		// Rotate all normals
		for (size_t i = 0; i < norBuf.size(); i += 3) {
			float x0 = norBuf[i];
			float y0 = norBuf[i + 1];
			float z0 = norBuf[i + 2];

			float x = cosTheta * x0 + sinTheta * z0;
			float y = y0;
			float z = -sinTheta * x0 + cosTheta * z0;

			float len = sqrt(x * x + y * y + z * z);
			if (len > 0) {
				x /= len;
				y /= len;
				z /= len;
			}

			norBuf[i] = x;
			norBuf[i + 1] = y;
			norBuf[i + 2] = z;
		}
	}
	//Create Image
	auto image = make_shared<Image>(width, height); 

	//Variables for the minimum and maximum x and y values
	float xmin = posBuf[0];
	float xmax = posBuf[0];
	float ymin = posBuf[1];
	float ymax = posBuf[1];

	//Variables for current x and y
	float x;
	float y;

	//Compute max and min x and y 
	for (size_t i = 3; i < posBuf.size(); i += 3) {
		x = posBuf[i];
		y = posBuf[i + 1];

		if (x < xmin) xmin = x;
		if (x > xmax) xmax = x;
		if (y < ymin) ymin = y;
		if (y > ymax) ymax = y;
	}

	//Computing the objects size, scale factors, and centers
	float objectWidth = xmax - xmin;
	float objectHeight = ymax - ymin;

	float scaleX = width / objectWidth;
	float scaleY = height / objectHeight;
	float scale = min(scaleX, scaleY);
	
	float objCenterX = (xmin + xmax) / 2;
	float objCenterY = (ymin + ymax) / 2;

	float imageCenterX = width / 2;
	float imageCenterY = height / 2;


	vector<float> zBuffer(width* height, -std::numeric_limits<float>::infinity());
	float zmin = posBuf[2];
	float zmax = posBuf[2];
	for (size_t i = 2; i < posBuf.size(); i += 3) {
		if (posBuf[i] < zmin) zmin = posBuf[i];
		if (posBuf[i] > zmax) zmax = posBuf[i];
	}

	//Loop of the triangles
	for (size_t i = 0; i < posBuf.size(); i += 9) {
		float x1 = posBuf[i];
		float y1 = posBuf[i + 1];
		float x2 = posBuf[i + 3];
		float y2 = posBuf[i + 4];
		float x3 = posBuf[i + 6];
		float y3 = posBuf[i + 7];

		float u1 = (x1 - objCenterX) * scale + imageCenterX;
		float v1 = imageCenterY - (y1 - objCenterY) * scale;

		float u2 = (x2 - objCenterX) * scale + imageCenterX;
		float v2 = imageCenterY - (y2 - objCenterY) * scale;

		float u3 = (x3 - objCenterX) * scale + imageCenterX;
		float v3 = imageCenterY - (y3 - objCenterY) * scale;

		// Compute per-triangle bounding box 
		float triMinX = min(u1, min(u2, u3));
		float triMaxX = max(u1, max(u2, u3));
		float triMinY = min(v1, min(v2, v3));
		float triMaxY = max(v1, max(v2, v3));


		// Clamp to image bounds to avoid out-of-bounds errors
		//The following four lines of code were added in through the use of ChatGPT to avoid out-of-bounds errors
		triMinX = max(0.0f, triMinX);
		triMaxX = min((float)width - 1, triMaxX);
		triMinY = max(0.0f, triMinY);
		triMaxY = min((float)height - 1, triMaxY);

		// Pick color for triangle
		int triangleIndex = i / 9;
		auto color = RANDOM_COLORS[triangleIndex % 7];
		unsigned char r = (unsigned char)(color[0] * 255);
		unsigned char g = (unsigned char)(color[1] * 255);
		unsigned char b = (unsigned char)(color[2] * 255);

		auto edge = [](float x0, float y0, float x1, float y1, float x, float y) {
			return (x - x0) * (y1 - y0) - (y - y0) * (x1 - x0);
			};

		float area = edge(u1, v1, u2, v2, u3, v3);

		// vertex 1 normal
		float nx1 = norBuf[i + 0];
		float ny1 = norBuf[i + 1];
		float nz1 = norBuf[i + 2];

		// vertex 2 normal
		float nx2 = norBuf[i + 3];
		float ny2 = norBuf[i + 4];
		float nz2 = norBuf[i + 5];

		// vertex 3 normal
		float nx3 = norBuf[i + 6];
		float ny3 = norBuf[i + 7];
		float nz3 = norBuf[i + 8];

		//Written by ChatGPT to handle negative area
		bool flip = false;
		if (area < 0) {
			area = -area;
			flip = true;
		}

		// Fill rectangle
		for (int px = triMinX; px <= triMaxX; px++) {
			for (int py = triMinY; py <= triMaxY; py++) {
				float fx = px + 0.5f;
				float fy = py + 0.5f;

				// Barycentric coordinates
				float w1 = edge(u2, v2, u3, v3, fx, fy) / area;
				float w2 = edge(u3, v3, u1, v1, fx, fy) / area;
				float w3 = edge(u1, v1, u2, v2, fx, fy) / area;

				// Task 1: Drawing Bounding Boxes
				if (TaskNum == 1) {
					image->setPixel(px, height - 1 - py, r, g, b);
				}
				// Task 2: Drawing Triangles
				if (TaskNum == 2) {
					// Epsilon test
					//Cite: ChatGBT
					if (w1 >= -1e-6f && w2 >= -1e-6f && w3 >= -1e-6f) {
						image->setPixel(px, height - 1 - py, r, g, b);
					}

				}
				// Task 3: Interpolating Per-Vertex Colors
				if (TaskNum == 3) {
					if (w1 >= -1e-6f && w2 >= -1e-6f && w3 >= -1e-6f) {
						//Identify Vertex Indices
						// 'i' is the index in posBuf (floats). Since each vertex has 3 floats (x,y,z), divide by 3 to get the actual Vertex Index.
						int v1_idx = i / 3;
						int v2_idx = (i / 3) + 1;
						int v3_idx = (i / 3) + 2;

						// Get Colors for Vertices
						// Use modulo 7 (%) to wrap around the RANDOM_COLORS array as requested
						double* c1 = RANDOM_COLORS[v1_idx % 7];
						double* c2 = RANDOM_COLORS[v2_idx % 7];
						double* c3 = RANDOM_COLORS[v3_idx % 7];

						// Interpolate (Red, Green, Blue separately)
						// Color = w1*c1 + w2*c2 + w3*c3
						float r_f = w1 * c1[0] + w2 * c2[0] + w3 * c3[0];
						float g_f = w1 * c1[1] + w2 * c2[1] + w3 * c3[1];
						float b_f = w1 * c1[2] + w2 * c2[2] + w3 * c3[2];

						// Clamp and convert to unsigned char (0-255)
						// We clamp because floating point errors might result in -0.001 or 1.001
						//Added in throught he use of ChatGPT
						r_f = max(0.0f, min(1.0f, r_f));
						g_f = max(0.0f, min(1.0f, g_f));
						b_f = max(0.0f, min(1.0f, b_f));

						unsigned char r_val = (unsigned char)(r_f * 255.0f);
						unsigned char g_val = (unsigned char)(g_f * 255.0f);
						unsigned char b_val = (unsigned char)(b_f * 255.0f);

						image->setPixel(px, height - 1 - py, r_val, g_val, b_val);
					}

				}
				//Task 4: Vertical Color
				if (TaskNum == 4) {
					if (w1 >= -1e-6f && w2 >= -1e-6f && w3 >= -1e-6f) {
						// Map pixel y back to object space
						float y_obj = (fy - imageCenterY) / scale + objCenterY;

						// Normalize y to [0,1] over the object
						float t = (y_obj - ymin) / objectHeight;
						t = max(0.0f, min(1.0f, t));

						// Vertical gradient: blue -> red
						unsigned char r = (unsigned char)(t * 255.0f);
						unsigned char g = 0;
						unsigned char b = (unsigned char)((1.0f - t) * 255.0f);

						image->setPixel(px, height - 1 - py, r, g, b);
					
					}
				}
				//Task 5: Z-buffering
				if (TaskNum == 5) {
					float z1 = posBuf[i + 2]; // z of vertex 1
					float z2 = posBuf[i + 5]; // z of vertex 2
					float z3 = posBuf[i + 8]; // z of vertex 3
					float z = w1 * z1 + w2 * z2 + w3 * z3;

					if (w1 >= -1e-6f && w2 >= -1e-6f && w3 >= -1e-6f) {
						float z = w1 * z1 + w2 * z2 + w3 * z3;
						int idx = py * width + px;
						if (z > zBuffer[idx]) {
							zBuffer[idx] = z;
							float zNormalized = (z - zmin) / (zmax - zmin);
							zNormalized = max(0.0f, min(1.0f, zNormalized));
							unsigned char red = (unsigned char)((1.0f - zNormalized) * 255);
							image->setPixel(px, height - 1 - py, red, 0, 0);
						}
					}
				}
				//Task 6: Normal Coloring
				if (TaskNum == 6) {
					if (w1 >= -1e-6f && w2 >= -1e-6f && w3 >= -1e-6f) {

						// 1. Calculate Z (Same as Task 5)
						float z1 = posBuf[i + 2];
						float z2 = posBuf[i + 5];
						float z3 = posBuf[i + 8];
						float z = w1 * z1 + w2 * z2 + w3 * z3;

						// 2. Check Z-Buffer (Essential for 3D visibility)
						int idx = py * width + px;
						if (z > zBuffer[idx]) {

							// 3. Update Z-Buffer
							zBuffer[idx] = z;

							// 4. Calculate Normal Color and Draw
							float nx = w1 * nx1 + w2 * nx2 + w3 * nx3;
							float ny = w1 * ny1 + w2 * ny2 + w3 * ny3;
							float nz = w1 * nz1 + w2 * nz2 + w3 * nz3;

							float len = sqrt(nx * nx + ny * ny + nz * nz);
							if (len > 0) { nx /= len; ny /= len; nz /= len; }

							unsigned char r = (unsigned char)(255.0f * (0.5f * nx + 0.5f));
							unsigned char g = (unsigned char)(255.0f * (0.5f * ny + 0.5f));
							unsigned char b = (unsigned char)(255.0f * (0.5f * nz + 0.5f));

							image->setPixel(px, height - 1 - py, r, g, b);
						}
					}
				}
				// Task 7: Simple Lighting
				if (TaskNum == 7 || TaskNum == 8) {

					if (w1 >= -1e-6f && w2 >= -1e-6f && w3 >= -1e-6f) {
						// 1. Calculate Z for Z-buffering
						float z1 = posBuf[i + 2];
						float z2 = posBuf[i + 5];
						float z3 = posBuf[i + 8];
						float z = w1 * z1 + w2 * z2 + w3 * z3;

						int idx = py * width + px;
						if (z > zBuffer[idx]) {
							zBuffer[idx] = z;

							// interpolate normal
							float nx = w1 * nx1 + w2 * nx2 + w3 * nx3;
							float ny = w1 * ny1 + w2 * ny2 + w3 * ny3;
							float nz = w1 * nz1 + w2 * nz2 + w3 * nz3;

							// normalize
							float len = sqrt(nx * nx + ny * ny + nz * nz);
							if (len > 0) { nx /= len; ny /= len; nz /= len; }

							// ensure normal faces camera
							if (nz < 0) {
								nx = -nx;
								ny = -ny;
								nz = -nz;
							}

							// light direction
							float inv = 1.0f / sqrt(3.0f);
							float lx = inv, ly = inv, lz = inv;

							// lambert
							float c = max(lx * nx + ly * ny + lz * nz, 0.0f);
							c = min(c, 1.0f);

							unsigned char v = (unsigned char)(255.0f * c);
							image->setPixel(px, height - 1 - py, v, v, v);
						}
					}
				}
			}

		}
	}

	image->writeToFile(outputName);

	return 0;
}
