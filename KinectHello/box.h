#ifndef BOX_H
#define BOX_H

#include <qmatrix4x4.h>
#include "RansacShapeDetector.h"
#include "PlanePrimitiveShapeConstructor.h"
#include "PlanePrimitiveShape.h"

extern class BoxJoint;

static int mat[8][3] = {
	{ 0, 1, 1 },
	{ 1, 1, 1 },
	{ 1, 0, 1 },
	{ 0, 0, 1 },
	{ 0, 1, 0 },
	{ 1, 1, 0 },
	{ 1, 0, 0 },
	{ 0, 0, 0 },
};

static int plane[6][4] = {
	{ 0, 1, 2, 3 },
	{ 5, 4, 7, 6 },
	{ 4, 0, 3, 7 },
	{ 1, 5, 6, 2 },
	{ 3, 2, 6, 7 },
	{ 1, 0, 4, 5 }
};

class Box{
public:
	Box(Vec3fShape pcenter, Vec3fShape* pnormal, float* pmax, float* pmin) :boxBegin(0), boxEnd(0), joint(NULL){
		Vec3fShape it;
		m_transform.setToIdentity();
		center = pcenter;
		for (size_t i = 0; i < 3; i++)
		{
			normal[i] = pnormal[i];
			max[i] = pmax[i];
			min[i] = pmin[i];
		}
		for (size_t i = 0; i < 8; i++)
		{
			vertex[i] = center
				+ mat[i][0] * normal[0] * max[0] + (1 - mat[i][0])* normal[0] * min[0]
				+ mat[i][1] * normal[1] * max[1] + (1 - mat[i][1])* normal[1] * min[1]
				+ mat[i][2] * normal[2] * max[2] + (1 - mat[i][2])* normal[2] * min[2];
		}

		selectedPlaneIndex = -1;
		selectedPointIndex[0] = -1;
		selectedPointIndex[1] = -1;
		shapeRange = new std::vector<std::pair<int, int>>;
		pivotPoint[0] = -1;
		pivotPoint[1] = -1;
		joint = NULL;

	};
	Box(int t){
		selectedPlaneIndex = -1;
		selectedPointIndex[0] = -1;
		selectedPointIndex[1] = -1;
		shapeRange = new std::vector<std::pair<int, int>>;
		pivotPoint[0] = -1;
		pivotPoint[1] = -1;
		joint = NULL;
	};
	Vec3fShape& getPlane(int index){
		if (index < 0 || index > 3 )
		{
			return Vec3fShape(0,0,0);
		}
		return vertex[plane[selectedPlaneIndex][index]];
	}

	Vec3fShape& getTargetPlane(int tPlane,int index){
		if (index < 0 || index > 3)
		{
			return Vec3fShape(0, 0, 0);
		}
		return vertex[plane[tPlane][index]];
	}

	int getPlaneIndex(int index){
		if (index < 0 || index > 3)
		{
			return -1;
		}
		return plane[selectedPlaneIndex][index];
	}

	Vec3fShape& getVertexByInx(int i){
		return vertex[i];
	}

	Vec3fShape center;
	Vec3fShape normal[3];
	Vec3fShape vertex[8];
	QMatrix4x4 m_transform;
	std::vector<std::pair<int, int>> *shapeRange;
	float max[3];
	float min[3];
	int boxBegin;
	int boxEnd;
	int selectedPointIndex[2];
	int selectedPlaneIndex;
	BoxJoint * joint;
	int pivotPoint[2];
	
};

#endif