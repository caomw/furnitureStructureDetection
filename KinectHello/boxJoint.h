#ifndef BOXJOINT_H
#define BOXJOINT_H
#include "box.h"
class BoxJoint
{
public:
	enum {HINGE = 0,SLIDER = 1};
	BoxJoint(Box& pParent, Box& pChild);
	~BoxJoint();
	int getType();
	void setBoxIndex(int i, int j);
	void setBox(Box& pParent, Box& pChild);
	double getRangeMin();
	double getRangeMax();
	double getCurrentValue();
	Vec3fShape getPivotPoint(int index);
	void setRange(double min, double max);
	void setPivotPoint(Vec3fShape & pivot1, Vec3fShape & pivot2);
	void setPivotPointIndex(int index1, int index2);
	Box& getParent();
	Box& getChild();
	virtual void rotate(double pAngle){};
	virtual void rotate(){};
	virtual void slide(double ){};
	virtual void slide(){};

	Box& parent;
	Box& child;
	int iParent;
	int iChild;
	int jointType;
	Vec3fShape pivotPoint[2];
	double initValue;
	double valueRange[2];
	double currentValue;

};


#endif