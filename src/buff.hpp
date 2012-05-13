#ifndef FROBNICATOR_BUFF_H
#define FROBNICATOR_BUFF_H

#include "common.hpp"

class Buff {
public:
	Buff()
		: amount(0.0f)
		, duration(0.0f){}

	Buff(float amount, float duration)
		: amount(amount)
		, duration(duration){}

	void tick(float dt){
		if ( duration == 0.0f ) return;
		duration = max(duration-dt, 0.0f);
	}

	/* public because I'm lazy */
	float amount;
	float duration;
};

class SlowBuff: public Buff {
public:
	SlowBuff(): Buff(){}
	SlowBuff(float amount, float duration): Buff(amount, duration){}
};

class PoisonBuff: public Buff {
public:
	PoisonBuff(): Buff(){}
	PoisonBuff(float amount, float duration): Buff(amount, duration){}
};

#endif /* FROBNICATOR_BUFF_H */
