#include "boxHingeJoint.h"

BoxHingeJoint::BoxHingeJoint(Box& pParent, Box&pChild) :BoxJoint(pParent, pChild)
{
	jointType = HINGE;
	currentValue = 0.0f;
}

void BoxHingeJoint::rotate(){
	rotate(currentValue);
}

void BoxHingeJoint::rotate(double pAngle){
	currentValue = pAngle;
	Vec3fShape offset = pivotPoint[1] - pivotPoint[0];
	QMatrix4x4 rotate;
	QMatrix4x4 transform;

	transform.setToIdentity();
	rotate.setToIdentity();
	rotate.rotate(pAngle, offset[0], offset[1], offset[2]);
	transform.translate(-pivotPoint[0][0], -pivotPoint[0][1], -pivotPoint[0][2]);
	child.m_transform = rotate * transform;
	transform.setToIdentity();
	transform.translate(pivotPoint[0][0], pivotPoint[0][1], pivotPoint[0][2]);
	child.m_transform = transform * child.m_transform;
}


BoxHingeJoint::~BoxHingeJoint()
{

}

