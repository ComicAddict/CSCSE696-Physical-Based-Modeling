#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H

#include <vector>
#include <cmath>
#include <glm/glm.hpp>

#define MAX_PARTICLE_PER_GENERATOR 1000
#define MAX_PARTICLES 10000

struct particle_gpu {
	glm::vec3 p;
	glm::vec3 c;
};

struct particle_cpu {
	glm::vec3 v;
	float m;
	float LS;
	float COF;
	float COR;
	float age;
};

struct Particle {
	particle_gpu PGPU;
	particle_gpu PCPU;
};

struct ParticleGenerator {


};

class ParticleSystem {

public:
	ParticleSystem();
	~ParticleSystem();

private:
	

};
#endif

