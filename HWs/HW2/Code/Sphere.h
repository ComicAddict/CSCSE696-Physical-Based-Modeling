#ifndef SPHERE_H
#define SPHERE_H

#include <vector>
#include <cmath>
class Sphere {

public:
	Sphere(int stack, int sector, bool smooth, float radius);
	void setStack(int stack);
	void setSector(int sector);
	void setDims(int stack, int sector, float radius);
	void setRadius(float radius);
	float* getVertices();
	float* getNormals();
	int* getIndices();

	std::vector<float> vertices;
	std::vector<float> normals;
	std::vector<int> indices;
private:
	void genVertices();
	void genSmoothSphere();
	void genFlatSphere();

	float radius;
	int stack;
	int sector;
	bool smooth;

};
#endif // !SPHERE_H

