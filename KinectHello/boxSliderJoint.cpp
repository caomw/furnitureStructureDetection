#include "boxSliderJoint.h"

BoxSliderJoint::BoxSliderJoint(Box& pParent, Box&pChild) :BoxJoint(pParent, pChild)
{
	jointType = SLIDER;
	currentValue = 0.0f;
}

BoxSliderJoint::~BoxSliderJoint()
{

}

void BoxSliderJoint::slide(double pValue){
	currentValue = pValue;
	Vec3fShape offset = pivotPoint[1] - pivotPoint[0];
	offset.normalize();
	QMatrix4x4 transform;
	transform.setToIdentity();

	transform.translate(offset[0] * currentValue, offset[1] * currentValue, offset[2] * currentValue);

	child.m_transform = transform;
}

void BoxSliderJoint::slide(){
	slide(currentValue);
}