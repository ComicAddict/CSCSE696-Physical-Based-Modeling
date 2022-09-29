#ifndef PARTICLEGENERATOR_H
#define PARTICLEGENERATOR_H

#include "ParticleData.h"

class ParticleGenerator {

public:
	ParticleGenerator();
	virtual ~ParticleGenerator();

	virtual void genParticles(float h, ParticleData *pData);
private:
};

#endif