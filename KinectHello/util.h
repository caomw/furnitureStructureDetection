#include "STJTypes.h"
void buildPlaneCoords(const VEC4D &plane, VEC3D &origin, VEC3D &U, VEC3D &V);
VEC3D proj_onto_plane_3d(const VEC4D &plane, const VEC3D &pt);
void align_seg_2_plane(const VEC3D seg_ends[2], const VEC4D &plane, MAT3D &out_R, VEC3D &out_t);
void gatherFaces(VEC4D &abcd, VEC3D* m_corners);
double signed_dist_pt2Plane(const VEC4D &plane, const VEC3D &pt);
void make_seg_parallel(const VEC3D seg1_ends[2], const VEC3D seg2_ends[2], MAT3D &out_R, VEC3D &out_t);
double calc_jnt_angle(const MAT3D &input_R, const VEC3D &jnt_axis);