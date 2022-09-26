#ifndef PARTICLE_H
#define PARTICLE_H

#include <glm/glm.hpp>

class Particle {
public:
	Particle();
	~Particle();

	glm::vec3 pos;
	glm::vec3 velocity;
private:


};
 
#endif // !PARTICLE_H
