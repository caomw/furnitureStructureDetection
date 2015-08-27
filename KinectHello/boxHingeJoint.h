#ifndef BOXHINGEJOINT_H
#define BOXHINGEJOINT_H
#include "boxJoint.h"
class BoxHingeJoint:public BoxJoint
{
public:
	BoxHingeJoint(Box& pParent, Box&pChild);
	~BoxHingeJoint();
	
	void rotate(double pAngle);
	void rotate();


private:

};

#endif