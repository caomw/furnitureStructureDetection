#include "boxJoint.h"
BoxJoint::BoxJoint(Box& pParent, Box& pChild) :parent(pParent), child(pChild)
{
	jointType = HINGE;
	pChild.joint = this;
	iParent = -1;
	iChild = -1;
}

BoxJoint::~BoxJoint()
{

}

void BoxJoint::setBoxIndex(int i, int j){
	iParent = i;
	iChild = j;
}

void BoxJoint::setBox(Box& pParent, Box& pChild){
	parent = pParent;
	child = pChild;
}
int BoxJoint::getType(){
	return jointType;
}

double BoxJoint::getRangeMin(){
	return valueRange[0];
}

double BoxJoint::getRangeMax(){
	return valueRange[1];
}

double BoxJoint::getCurrentValue(){
	return currentValue;
}

void BoxJoint::setRange(double min, double max){
	valueRange[0] = min;
	valueRange[1] = max;
}

void BoxJoint::setPivotPoint(Vec3fShape & pivot1, Vec3fShape & pivot2){
	pivotPoint[0] = pivot1;
	pivotPoint[1] = pivot2;
}

void BoxJoint::setPivotPointIndex(int index1, int index2){
	setPivotPoint(child.vertex[index1], child.vertex[index2]);
	child.pivotPoint[0] = index1;
	child.pivotPoint[1] = index2;
}

Vec3fShape BoxJoint::getPivotPoint(int index){
	return pivotPoint[index];
}

Box& BoxJoint::getParent(){
	return parent;
}

Box& BoxJoint::getChild(){
	return child;
}