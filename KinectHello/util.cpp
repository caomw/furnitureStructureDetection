#include "util.h"
void buildPlaneCoords(const VEC4D &plane, VEC3D &origin, VEC3D &U, VEC3D &V)
{
	origin = VEC3D::ZERO;
	if (fabs(plane[0])>1e-6f) origin[0] = -plane[3] / plane[0];
	else if (fabs(plane[1])>1e-6f) origin[1] = -plane[3] / plane[1];
	else origin[2] = -plane[3] / plane[2];

	VEC3D plane_normal(plane[0], plane[1], plane[2]);
	U = VEC3D::ZERO;
	if (fabs(plane_normal[0])>1e-6f || fabs(plane_normal[2])>1e-6f)
	{
		U[0] = -plane_normal[2];
		U[2] = plane_normal[0];
	}
	else if (fabs(plane_normal[1])>1e-6f)
	{
		U[1] = -plane_normal[2];
		U[2] = plane_normal[1];
	}
	U.Normalize();
	V = plane_normal.Cross(U);
	V.Normalize();
}


VEC3D proj_onto_plane_3d(const VEC4D &plane, const VEC3D &pt)
{
	VEC3D plane_origin, U, V;
	buildPlaneCoords(plane, plane_origin, U, V);
	VEC3D plane_normal(plane[0], plane[1], plane[2]);
	plane_normal.Normalize();
	VEC3D vertical_offset = plane_normal * (plane_normal.Dot(pt - plane_origin));
	return pt - vertical_offset;
}


void align_seg_2_plane(const VEC3D seg_ends[2], const VEC4D &plane, MAT3D &out_R, VEC3D &out_t)
{
	//get the rotation
	VEC3D ori_dir = seg_ends[1] - seg_ends[0]; ori_dir.Normalize();
	VEC3D pn = VEC3D(plane[0], plane[1], plane[2]); pn.Normalize();

	VEC3D cr = ori_dir.Cross(pn); cr.Normalize();
	VEC3D proj_dir = pn.Cross(cr); proj_dir.Normalize();

	QUATD rot_q;
	rot_q.Align(ori_dir, proj_dir);
	rot_q.ToRotationMatrix(out_R);

	//the rotation center is the mid point
	VEC3D rc = (seg_ends[0] + seg_ends[1])*0.5f;

	//get the rotated end, then translate the end to the plane
	VEC3D rot_end = out_R*(seg_ends[0] - rc) + rc;

	double signed_dist = signed_dist_pt2Plane(plane, rot_end);

	VEC3D offset = -VEC3D(plane[0], plane[1], plane[2])*signed_dist;

	out_t = rc - out_R*rc + offset;

	//test
	VEC3D final_end1, final_end2;
	final_end1 = out_R*seg_ends[0] + out_t;
	final_end2 = out_R*seg_ends[1] + out_t;

}
//void align_seg_2_plane(const VEC3D seg_ends[2], const VEC4D &plane, MAT3D &out_R, VEC3D &out_t)
//{
//	//first project the 2 points to the plane
//	VEC3D proj_ends[2];
//	for (int i = 0; i<2; i++)
//	{
//		proj_ends[i] = proj_onto_plane_3d(plane, seg_ends[i]);
//	}
//
//	//get the rotation
//	VEC3D ori_dir = seg_ends[1] - seg_ends[0]; ori_dir.Normalize();
//	VEC3D proj_dir = proj_ends[1] - proj_ends[0]; proj_dir.Normalize();
//	QUATD rot_q;
//	rot_q.Align(ori_dir, proj_dir);
//	rot_q.ToRotationMatrix(out_R);
//
//	//the rotation center is the mid point
//	VEC3D rc = (seg_ends[0] + seg_ends[1])*0.5f;
//
//	//get the rotated end, then translate the end to the plane
//	VEC3D rot_end = out_R*(seg_ends[0] - rc) + rc;
//
//	double signed_dist = signed_dist_pt2Plane(plane, rot_end);
//
//	VEC3D offset = -VEC3D(plane[0], plane[1], plane[2])*signed_dist;
//
//	out_t = rc - out_R*rc + offset;
//
//	//test
//	VEC3D final_end1, final_end2;
//	final_end1 = out_R*seg_ends[0] + out_t;
//	final_end2 = out_R*seg_ends[1] + out_t;
//
//	double r1 = plane.Dot(VEC4D(final_end1[0], final_end1[1], final_end1[2], 1));
//	double r2 = plane.Dot(VEC4D(final_end2[0], final_end2[1], final_end2[2], 1));
//	printf("%.4f %.4f\n",r1,r2);
//
//}

void gatherFaces(VEC4D &abcd, VEC3D* m_corners)
{
	VEC3D normal;
	//face 0 1 2 3
	normal = (m_corners[0] - m_corners[1]).Cross(m_corners[2] - m_corners[1]);
	normal.Normalize();
	////make the face normal towards outside
	//if (normal.Dot(m_corners[4] - m_corners[0])>0)
	//{
	//	normal = -normal;
	//}
	abcd[0] = normal[0];	abcd[1] = normal[1];	abcd[2] = normal[2];
	abcd[3] = -normal.Dot(m_corners[0]);
}

double signed_dist_pt2Plane(const VEC4D &plane, const VEC3D &pt)
{
	VEC4D pt_homo(pt[0], pt[1], pt[2], 1);
	return plane.Dot(pt_homo);
}

void make_seg_parallel(const VEC3D seg1_ends[2], const VEC3D seg2_ends[2], MAT3D &out_R, VEC3D &out_t)
{
	VEC3D dir_1 = seg1_ends[1] - seg1_ends[0];
	dir_1.Normalize();
	VEC3D dir_2 = seg2_ends[1] - seg2_ends[0];
	dir_2.Normalize();

	//we dont care about dir, we just want parallel
	if (dir_1.Dot(dir_2)<0)
	{
		dir_2 = -dir_2;
	}

	QUATD rot_q;
	rot_q.Align(dir_1, dir_2);
	rot_q.ToRotationMatrix(out_R);

	//the rotation center is the mid point
	VEC3D rc = (seg1_ends[0] + seg1_ends[1])*0.5f;
	out_t = rc - out_R*rc;
}

double calc_jnt_angle(const MAT3D &input_R, const VEC3D &jnt_axis)
{
	QUATD q;
	q.FromRotationMatrix(input_R);
	VEC3D tmp_axis;
	double angle;
	q.ToAxisAngle(tmp_axis, angle);
	if (tmp_axis.Dot(jnt_axis)<0)
	{
		angle = -angle;
	}
	return angle*180.f/3.1415926f;
}
