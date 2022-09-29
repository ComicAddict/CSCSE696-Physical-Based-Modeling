#ifndef PARTICLEDATA_H
#define PARTICLEDATA_H

#include <vector>
#include <memory>
#include <cmath>
#include <glm/glm.hpp>

#define MAX_PARTICLE_PER_GENERATOR 1000
#define MAX_PARTICLES 10000

class ParticleData {

public:
	ParticleData() {};
	ParticleData(int maxParticles) { genParticle(maxParticles); }
	~ParticleData() {};

	std::unique_ptr<glm::vec3[]> p;
	std::unique_ptr<glm::vec3[]> c;
	std::unique_ptr<glm::vec3[]> v;
	std::unique_ptr<glm::vec3[]> a;
	std::unique_ptr<glm::vec3[]> t;
	std::unique_ptr<bool[]> v;

	int n;
	int n_alive;

	void genParticle(int maxParticles);
	void swapData(int a, int b);

private:

	

};
#endif

