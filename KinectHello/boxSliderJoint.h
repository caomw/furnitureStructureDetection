#ifndef BOXSLIDERJOINT_H
#define BOXSLIDERJOINT_H
#include "boxJoint.h"

class BoxSliderJoint:public BoxJoint
{
public:
	BoxSliderJoint(Box& pParent, Box&pChild);
	~BoxSliderJoint();
	virtual void slide(double);
	virtual void slide();

private:

};

#endif