#ifndef _MY_FRAME_H
#define _MY_FRAME_H


#include "../STJUtils/STJTypes.h"
#include "../STJUtils/STJPara.h"

class CBodyFrame
{
protected:
	VEC3D m_mc; 
	QUATD  m_rot; //orientation
	VEC3D m_axis[3]; //frame axes
	double m_hl[3];
public:
	CBodyFrame();

	CBodyFrame(const CBodyFrame &rhs);
	CBodyFrame &operator=(const CBodyFrame &rhs);

	const VEC3D& center() const{ return m_mc; }
	const QUATD& rot_quat() const { return m_rot; }
	VEC3D& center() { return m_mc; }
	QUATD& rot_quat() { return m_rot; }

	void setRotation(const QUATD &q);

	void setRotation(const MAT3D &f);

	void axisToQuat();

	void toGLMatrix(MAT4D &m);

	void toTransformMatrix(MAT4D &m) const;
	void fromTransformMatrix(const MAT4D &m);
	void toInvTransMat(MAT4D &m) const;

	void toRotTranslation(MAT3D &r, VEC3D &t) const;
	void inverse_toRotTranslation(MAT3D &r, VEC3D &t) const;

	bool save(std::ofstream &ofs);
	//bool load(xml_istream<std::ifstream> &ixml);

	bool checkFrameAxis();

	void setCenter(const double *center);
	void setCenter(const VEC3D &c);
	void setAxisFromRotationMat(const double *mat);//row major
	void setAxis(const VEC3D &a1,  const VEC3D &a2, const VEC3D &a3);
	void setHL(double hl1,double hl2,double hl3);
	double getHL(int idx) const;
	const double *getHLs() const {return m_hl;}
	void setHL(int idx,double val);

	VEC3D transformPt(const VEC3D &pt) const;

	VEC3D transformVec(const VEC3D &vec) const;

	CBodyFrame getInverse() const;

	CBodyFrame operator*(const CBodyFrame &rhs);

	bool operator!=(const CBodyFrame &rhs);

	const VEC3D &getAxis(int index) const;
	VEC3D &getAxis(int index);
	const VEC3D *getAxes() const;
	VEC3D *getAxes();

	//Don't forget to call axisToQuat
	void setAxis(int index, const VEC3D &v);

	void saveBIN(FILE *fp) const;

	void loadBIN(FILE *fp);

	void render(const RENDER_PARA &para) const;

	void make_rot_mat(double angle, int axis, MAT4D &rot_mat) const;
	void make_s_mat(double scale, int axis, MAT4D &s_mat) const;
	void make_t_mat(double t, int axis, MAT4D &t_mat) const;

	void g2l(MAT4D &output) const;
	void l2g(MAT4D &output) const;

	void transform(const MAT4D &trans_mat);
	void translate(const VEC3D &t);
	void alignTo(const CBodyFrame &rhs, bool bScale = false);

	void swap_axis(int idx1, int idx2);

	void scale(int axis, double scale);
	void scale(int axis, double scale, const VEC3D &start_pt);
	void scale(double sx, double sy, double sz, const VEC3D &start_pt);
	void scale(double sx, double sy, double sz);
};

#endif