#include "StdAfx.h"
#include "MyFrame.h"
#include "GL/GL.h"
#include "../STJUtils/MatUtil.h"
#include "../STJUtils/STJMacros.h"

#define QUAT0_POSITIVE 1

CBodyFrame::CBodyFrame()
{
	m_mc = VEC3D(0.0f, 0.0f, 0.0f);
	m_rot = QUATD(1.0f, 0.0f, 0.0f, 0.0f);
	m_axis[0] = VEC3D(1.0f, 0.0f, 0.0f);
	m_axis[1] = VEC3D(0.0f, 1.0f, 0.0f);
	m_axis[2] = VEC3D(0.0f, 0.0f, 1.0f);
	m_hl[0] = m_hl[1] = m_hl[2] = 0.f;
}


CBodyFrame::CBodyFrame(const CBodyFrame &rhs)
{
	m_mc = rhs.m_mc;
	m_rot = rhs.m_rot; 
	m_axis[0] = rhs.m_axis[0];
	m_axis[1] = rhs.m_axis[1];
	m_axis[2] = rhs.m_axis[2];
	m_hl[0] = rhs.m_hl[0];
	m_hl[1] = rhs.m_hl[1];
	m_hl[2] = rhs.m_hl[2];
}

CBodyFrame& CBodyFrame::operator =(const CBodyFrame &rhs)
{
	m_mc = rhs.m_mc;
	m_rot = rhs.m_rot; 
	m_axis[0] = rhs.m_axis[0];
	m_axis[1] = rhs.m_axis[1];
	m_axis[2] = rhs.m_axis[2];
	m_hl[0] = rhs.m_hl[0];
	m_hl[1] = rhs.m_hl[1];
	m_hl[2] = rhs.m_hl[2];
	return *this;
}

void CBodyFrame::setRotation(const QUATD &q)
{
	m_rot = q;
	m_rot.Normalize();
	if (m_rot[0]<0)	//new added, let w to be positive, the rotation angle should be within [-180,180]
	{
		//printf("WARNING: bug exists! m_rot[0]<0!\n");
		m_rot = -m_rot;
	}
	MAT3D f;
	q.ToRotationMatrix(f);
	m_axis[0] = f.GetColumn(0);
	m_axis[1] = f.GetColumn(1);
	m_axis[2] = f.GetColumn(2);
	//make them othogonal and normalized
	m_axis[0].Normalize();
	m_axis[2] = m_axis[0].Cross(m_axis[1]);
	m_axis[2].Normalize();
	m_axis[1] = m_axis[2].Cross(m_axis[0]);
	m_axis[1].Normalize();
}

void CBodyFrame::setRotation(const MAT3D &f)
{
	m_rot.FromRotationMatrix(f);	//in this function, m_rot[0] always >= 0
	m_axis[0] = f.GetColumn(0);
	m_axis[1] = f.GetColumn(1);
	m_axis[2] = f.GetColumn(2);

	m_rot.Normalize();
#if QUAT0_POSITIVE
	if (m_rot[0]<0)	//new added, let w to be positive, the rotation angle should be within [-180,180]
	{
		//printf("WARNING: bug exists! m_rot[0]<0!\n");
		m_rot = -m_rot;
	}
#endif

	//make them othogonal and normalized
	m_axis[0].Normalize();
	m_axis[2] = m_axis[0].Cross(m_axis[1]);
	m_axis[2].Normalize();
	m_axis[1] = m_axis[2].Cross(m_axis[0]);
	m_axis[1].Normalize();
}

void CBodyFrame::setHL(double hl1,double hl2,double hl3)
{
	m_hl[0] = hl1;
	m_hl[1] = hl2;
	m_hl[2] = hl3;
}

void CBodyFrame::setHL(int idx, double val)
{
	m_hl[idx] = val;
}

double CBodyFrame::getHL(int idx) const
{
	return m_hl[idx];
}

void CBodyFrame::axisToQuat()
{
	MAT3D f;
	f.SetColumn(0, m_axis[0]);
	f.SetColumn(1, m_axis[1]);
	f.SetColumn(2, m_axis[2]);

	m_rot.FromRotationMatrix(f);
	m_rot.Normalize();

#if QUAT0_POSITIVE
	if (m_rot[0]<0)	//new added, let w to be positive, the rotation angle should be within [-180,180]
	{
		//printf("WARNING: bug exists! m_rot[0]<0!\n");
		m_rot = -m_rot;
	}
#endif
}

void CBodyFrame::toGLMatrix(MAT4D &m)
{
	m.MakeIdentity();
	((double *)m)[0] = m_axis[0][0]; 
	((double *)m)[1] = m_axis[0][1]; 
	((double *)m)[2] = m_axis[0][2]; 

	((double *)m)[4] = m_axis[1][0]; 
	((double *)m)[5] = m_axis[1][1]; 
	((double *)m)[6] = m_axis[1][2]; 

	((double *)m)[8] = m_axis[2][0]; 
	((double *)m)[9] = m_axis[2][1]; 
	((double *)m)[10] = m_axis[2][2]; 
}

void CBodyFrame::toTransformMatrix(MAT4D &mat) const
{
	double *m = (double*)mat;	//row major
	m[0] = m_axis[0][0]; m[1] = m_axis[1][0]; m[2] = m_axis[2][0];  m[3] = m_mc[0];
	m[4] = m_axis[0][1]; m[5] = m_axis[1][1]; m[6] = m_axis[2][1];  m[7] = m_mc[1];
	m[8] = m_axis[0][2]; m[9] = m_axis[1][2]; m[10] = m_axis[2][2]; m[11] = m_mc[2];
	m[12] = 0;			 m[13] = 0;			  m[14] = 0;			m[15] = 1.f;
}

void CBodyFrame::toInvTransMat(MAT4D &mat) const
{
	CBodyFrame inv_bdyFrame = getInverse();
	double *m = (double*)mat;	//row major
	m[0] = inv_bdyFrame.m_axis[0][0]; m[1] = inv_bdyFrame.m_axis[1][0]; 
	m[2] = inv_bdyFrame.m_axis[2][0];  m[3] = inv_bdyFrame.m_mc[0];
	m[4] = inv_bdyFrame.m_axis[0][1]; m[5] = inv_bdyFrame.m_axis[1][1]; 
	m[6] = inv_bdyFrame.m_axis[2][1];  m[7] = inv_bdyFrame.m_mc[1];
	m[8] = inv_bdyFrame.m_axis[0][2]; m[9] = inv_bdyFrame.m_axis[1][2]; 
	m[10] = inv_bdyFrame.m_axis[2][2]; m[11] = inv_bdyFrame.m_mc[2];
	m[12] = 0;			 m[13] = 0;			  m[14] = 0;			m[15] = 1.f;
}

void CBodyFrame::toRotTranslation(MAT3D &r, VEC3D &t) const
{
	double *rot = (double*)r;	//row major
	rot[0] = m_axis[0][0]; rot[1] = m_axis[1][0]; rot[2] = m_axis[2][0];  
	rot[3] = m_axis[0][1]; rot[4] = m_axis[1][1]; rot[5] = m_axis[2][1];  
	rot[6] = m_axis[0][2]; rot[7] = m_axis[1][2]; rot[8] = m_axis[2][2]; 
	t[0] = m_mc[0];	t[1] = m_mc[1];	t[2] = m_mc[2];
}

void CBodyFrame::inverse_toRotTranslation(MAT3D &r, VEC3D &t) const
{
	CBodyFrame inv_bdyFrame = getInverse();
	double *rot = (double*)r;	//row major
	rot[0] = inv_bdyFrame.m_axis[0][0]; rot[1] = inv_bdyFrame.m_axis[1][0]; rot[2] = inv_bdyFrame.m_axis[2][0];  
	rot[3] = inv_bdyFrame.m_axis[0][1]; rot[4] = inv_bdyFrame.m_axis[1][1]; rot[5] = inv_bdyFrame.m_axis[2][1];  
	rot[6] = inv_bdyFrame.m_axis[0][2]; rot[7] = inv_bdyFrame.m_axis[1][2]; rot[8] = inv_bdyFrame.m_axis[2][2]; 
	t[0] = inv_bdyFrame.m_mc[0];	t[1] = inv_bdyFrame.m_mc[1];	t[2] = inv_bdyFrame.m_mc[2];
}

bool CBodyFrame::save(std::ofstream &ofs) 
{
	ofs<<"<bodyframe>"<<std::endl;
	ofs<<"<center>"<<m_mc[0]<<" "
		<<m_mc[1]<<" "
		<<m_mc[2]<<"</center>"<<std::endl;
	ofs<<"<x_axis>"<<m_axis[0][0]<<" "
		<<m_axis[0][1]<<" "
		<<m_axis[0][2]<<"</x_axis>"<<std::endl;
	ofs<<"<y_axis>"<<m_axis[1][0]<<" "
		<<m_axis[1][1]<<" "
		<<m_axis[1][2]<<"</y_axis>"<<std::endl;
	ofs<<"<z_axis>"<<m_axis[2][0]<<" "
		<<m_axis[2][1]<<" "
		<<m_axis[2][2]<<"</z_axis>"<<std::endl;
	ofs<<"</bodyframe>"<<std::endl;
	return true;
}

//bool CBodyFrame::load(xml_istream<std::ifstream> &ixml)
//{
//	std::string section, data;
//	while(1)
//	{
//		section = ixml.peek();
//		if (section == "Center")
//		{
//			ixml.push();
//			ixml >> data;
//			std::istringstream ins(data);
//			ins >> m_mc[0] >> m_mc[1] >> m_mc[2];
//			ixml.pop();
//		}	
//		else if (section == "X_Axis")
//		{
//			ixml.push();
//			ixml >> data;
//			std::istringstream ins(data);
//			ins >> m_axis[0][0] >> m_axis[0][1] >> m_axis[0][2];
//			ixml.pop();
//		}
//		else if (section == "Y_Axis")
//		{
//			ixml.push();
//			ixml >> data;
//			std::istringstream ins(data);
//			ins >> m_axis[1][0] >> m_axis[1][1] >> m_axis[1][2];
//			ixml.pop();
//		}	
//		else if (section == "Z_Axis")
//		{
//			ixml.push();
//			ixml >> data;
//			std::istringstream ins(data);
//			ins >> m_axis[2][0] >> m_axis[2][1] >> m_axis[2][2];
//			ixml.pop();
//		}
//		else
//			break;
//	}
//
//	if (!checkFrameAxis())
//	{
//		printf("Incorrect body frame\n");
//
//		m_axis[2] = m_axis[0].Cross(m_axis[1]);
//		m_axis[2].Normalize();
//		m_axis[1] = m_axis[2].Cross(m_axis[0]);
//		m_axis[1].Normalize();
//
//		if (checkFrameAxis())
//		{
//			printf("Revised! May be still wrong, please check manually!\n");
//		}else
//		{
//			printf("Still Incorrect body frame! \n");
//		}
//	}
//	axisToQuat();
//
//	return true;
//}

bool CBodyFrame::checkFrameAxis()
{
	m_axis[0].Normalize();
	m_axis[1].Normalize();
	m_axis[2].Normalize();

	double val = m_axis[0].Dot(m_axis[1]);
	if (fabs(val) > 1e-6)
		return false;

	val = m_axis[0].Dot(m_axis[2]);
	if (fabs(val) > 1e-6)
		return false;

	val = m_axis[1].Dot(m_axis[2]);
	if (fabs(val) > 1e-6)
		return false;

	return true;
}
void CBodyFrame::setCenter(const double *center)
{
	m_mc[0] = center[0];
	m_mc[1] = center[1];
	m_mc[2] = center[2];
}
void CBodyFrame::setCenter(const VEC3D &c)
{
	m_mc = c;
}

void CBodyFrame::setAxisFromRotationMat(const double *mat)		//dMatrix3d
{
	m_axis[0][0] = mat[0];
	m_axis[0][1] = mat[4];
	m_axis[0][2] = mat[8];

	m_axis[1][0] = mat[1];
	m_axis[1][1] = mat[5];
	m_axis[1][2] = mat[9];

	m_axis[2][0] = mat[2];
	m_axis[2][1] = mat[6];
	m_axis[2][2] = mat[10];

	axisToQuat();
}

void CBodyFrame::setAxis(const VEC3D &a1,  const VEC3D &a2, const VEC3D &a3)
{
	m_axis[0] = a1;
	m_axis[1] = a2;
	m_axis[2] = a3;
	axisToQuat();
}

VEC3D CBodyFrame::transformPt(const VEC3D &pt) const
{
	VEC3D t;

	t = m_rot.Rotate(pt) + m_mc;
	return t;
}

VEC3D CBodyFrame::transformVec(const VEC3D &vec) const
{
	VEC3D t;

	t = m_rot.Rotate(vec);
	return t;
}

CBodyFrame CBodyFrame::getInverse() const
{
	CBodyFrame tBF;

	tBF.setRotation(m_rot.Conjugate());
	tBF.m_mc = tBF.m_rot.Rotate(m_mc)*-1.0;

	return tBF;
}


CBodyFrame CBodyFrame::operator*(const CBodyFrame &rhs)
{
	CBodyFrame tBF;

	tBF.setRotation(m_rot*rhs.m_rot);
	tBF.m_mc = m_rot.Rotate(rhs.m_mc) + m_mc;

	return tBF;
}

bool CBodyFrame::operator!=(const CBodyFrame &rhs)
{
	VEC3D t = m_mc - rhs.m_mc;
	if (t.Length() > 1e-6)
		return false;

	QUATD d = m_rot.Conjugate()*rhs.m_rot;
	if (fabs(d.W() - 1.0)>1e-6 || fabs(d.X())>1e-6 || fabs(d.Y())>1e-6 || fabs(d.Z())>1e-6)
		return false;

	return true;
}

const VEC3D &CBodyFrame::getAxis(int index) const
{
	ASSERT(index >= 0 && index < 3);
	return m_axis[index];
}

VEC3D &CBodyFrame::getAxis(int index)
{
	ASSERT(index >= 0 && index < 3);
	return m_axis[index];
}

const VEC3D *CBodyFrame::getAxes() const
{
	return m_axis;
}

VEC3D *CBodyFrame::getAxes()
{
	return m_axis;
}
//Don't forget to call axisToQuat
void CBodyFrame::setAxis(int index, const VEC3D &v)
{
	assert(index >= 0 && index < 3);
	m_axis[index] = v;

	axisToQuat();
}

void CBodyFrame::saveBIN(FILE *fp) const
{
	fwrite((const double *)m_mc, sizeof(double), 3, fp);
	fwrite((const double *)m_rot, sizeof(double), 4, fp);

	for(int i = 0; i < 3; i++)
		fwrite((const double *)m_axis[i], sizeof(double), 3, fp);
	fwrite(m_hl,sizeof(double),3,fp);
}

void CBodyFrame::loadBIN(FILE *fp)
{
	double t[4];

	fread(t, sizeof(double), 3, fp);
	m_mc = VEC3D(t[0], t[1], t[2]);

	//NOTE: this part will cause error!!! we will update q from axis
	fread(t, sizeof(double), 4, fp);
	m_rot = QUATD(t[0], t[1], t[2], t[3]);

	for(int i = 0; i < 3; i++)
	{
		fread(t, sizeof(double), 3, fp);
		m_axis[i] = VEC3D(t[0], t[1], t[2]);
	}

	//make them orthogonal
	m_axis[2].Normalize();
	m_axis[0] = m_axis[0] -  m_axis[2]*(m_axis[0].Dot(m_axis[2]));
	m_axis[0].Normalize();
	m_axis[1] = m_axis[2].Cross(m_axis[0]);
	m_axis[1].Normalize();
	//Here we will revise the loaded q
	m_rot.FromRotationMatrix(MAT3D(m_axis[0],m_axis[1],m_axis[2],true));

	m_rot.Normalize();

#if QUAT0_POSITIVE
	if (m_rot[0]<0)	//new added, let w to be positive, the rotation angle should be within [-180,180]
	{
		//printf("WARNING: bug exists! m_rot[0]<0!\n");
		m_rot = -m_rot;
	}
#endif

	fread(m_hl,sizeof(double),3,fp);
}

void CBodyFrame::render(const RENDER_PARA &para) const
{
	VEC3D destination;;
	VEC3D tmp,points[4];
	double render_len = m_hl[0];
	if (render_len>m_hl[1]) render_len = m_hl[1];
	if (render_len>m_hl[2]) render_len = m_hl[2];

	if(render_len < 0.1f) render_len = 0.1f;

	double delta = render_len*0.1f;		

	glPushAttrib(GL_ALL_ATTRIB_BITS );

	glDisable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	glLineWidth(2.0);
	//x_axis
	{
		glColor3d(1.0,0.0,0.0);
		destination = m_mc + m_axis[0]*render_len;
		glBegin(GL_LINES);						//draw line
		glVertex3d(m_mc.X(),m_mc.Y(),m_mc.Z());
		glVertex3d(destination.X(),destination.Y(),destination.Z());
		glEnd();

		tmp = m_mc + m_axis[0]*(render_len-delta);
		points[0] = tmp + m_axis[1]*delta/2;
		points[1] = tmp + m_axis[2]*delta/2;
		points[2] = tmp - m_axis[1]*delta/2;
		points[3] = tmp - m_axis[2]*delta/2;
		glBegin(GL_TRIANGLES);
		for (int i=0; i<3; i++)
		{
			glVertex3d(points[i].X(),points[i].Y(),points[i].Z());
			glVertex3d(points[i+1].X(),points[i+1].Y(),points[i+1].Z());
			glVertex3d(destination.X(),destination.Y(),destination.Z());
		}
		glVertex3d(points[3].X(),points[3].Y(),points[3].Z());
		glVertex3d(points[0].X(),points[0].Y(),points[0].Z());
		glVertex3d(destination.X(),destination.Y(),destination.Z());
		glEnd();
	}
	//y_axis
	{
		glColor3d(0.0,1.0,0.0);
		destination = m_mc + m_axis[1]*render_len;
		glBegin(GL_LINES);						//draw line
		glVertex3d(m_mc.X(),m_mc.Y(),m_mc.Z());
		glVertex3d(destination.X(),destination.Y(),destination.Z());
		glEnd();

		tmp = m_mc + m_axis[1]*(render_len-delta);
		points[0] = tmp + m_axis[2]*delta/2;
		points[1] = tmp + m_axis[0]*delta/2;
		points[2] = tmp - m_axis[2]*delta/2;
		points[3] = tmp - m_axis[0]*delta/2;
		glBegin(GL_TRIANGLES);
		for (int i=0; i<3; i++)
		{
			glVertex3d(points[i].X(),points[i].Y(),points[i].Z());
			glVertex3d(points[i+1].X(),points[i+1].Y(),points[i+1].Z());
			glVertex3d(destination.X(),destination.Y(),destination.Z());
		}
		glVertex3d(points[3].X(),points[3].Y(),points[3].Z());
		glVertex3d(points[0].X(),points[0].Y(),points[0].Z());
		glVertex3d(destination.X(),destination.Y(),destination.Z());
		glEnd();
	}
	//z_axis
	{
		glColor3d(0.0,0.0,1.0);
		destination = m_mc + m_axis[2]*render_len;
		glBegin(GL_LINES);						//draw line
		glVertex3d(m_mc.X(),m_mc.Y(),m_mc.Z());
		glVertex3d(destination.X(),destination.Y(),destination.Z());
		glEnd();

		tmp = m_mc + m_axis[2]*(render_len-delta);
		points[0] = tmp + m_axis[0]*delta/2;
		points[1] = tmp + m_axis[1]*delta/2;
		points[2] = tmp - m_axis[0]*delta/2;
		points[3] = tmp - m_axis[1]*delta/2;
		glBegin(GL_TRIANGLES);
		for (int i=0; i<3; i++)
		{
			glVertex3d(points[i].X(),points[i].Y(),points[i].Z());
			glVertex3d(points[i+1].X(),points[i+1].Y(),points[i+1].Z());
			glVertex3d(destination.X(),destination.Y(),destination.Z());
		}
		glVertex3d(points[3].X(),points[3].Y(),points[3].Z());
		glVertex3d(points[0].X(),points[0].Y(),points[0].Z());
		glVertex3d(destination.X(),destination.Y(),destination.Z());
		glEnd();
	}

	glDisable(GL_BLEND);
	glDisable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glPopAttrib();
}

void CBodyFrame::l2g(MAT4D &output) const
{
	output = MAT4D::IDENTITY;
	output(0,0) = m_axis[0][0]; output(0,1) = m_axis[1][0]; output(0,2) = m_axis[2][0];
	output(1,0) = m_axis[0][1]; output(1,1) = m_axis[1][1]; output(1,2) = m_axis[2][1];
	output(2,0) = m_axis[0][2]; output(2,1) = m_axis[1][2]; output(2,2) = m_axis[2][2];
	output(0,3) = m_mc[0]; output(1,3) = m_mc[1]; output(2,3) = m_mc[2];
}

void CBodyFrame::g2l(MAT4D &output) const
{
	output = MAT4D::IDENTITY;
	output(0,0) = m_axis[0][0]; output(1,0) = m_axis[1][0]; output(2,0) = m_axis[2][0];
	output(0,1) = m_axis[0][1]; output(1,1) = m_axis[1][1]; output(2,1) = m_axis[2][1];
	output(0,2) = m_axis[0][2]; output(1,2) = m_axis[1][2]; output(2,2) = m_axis[2][2];

	VEC4D RTC(m_mc[0],m_mc[1],m_mc[2],1.f);
	RTC = output*RTC;
	output(0,3) = -RTC[0]; output(1,3) = -RTC[1]; output(2,3) = -RTC[2];
}


void CBodyFrame::make_rot_mat(double angle, int axis, MAT4D &rot_mat) const
{
	//ROTATE: trans into local coordinate, then rotate, then trans into global
	//[R C]	* ROT  *	[RT -RTC] * [R 0]
	//[0 1]	 	        [0    1]    [0 0]
	rot_mat = MAT4D::IDENTITY;
	double cosine = cos(angle), sine = sin(angle);
	//make mat
	if (axis == 0)
	{
		rot_mat(1,1) = cosine;
		rot_mat(1,2) = -sine;
		rot_mat(2,1) = sine;
		rot_mat(2,2) = cosine;
	}else if (axis == 1)
	{
		rot_mat(0,0) = cosine;
		rot_mat(0,2) = sine;
		rot_mat(2,0) = -sine;
		rot_mat(2,2) = cosine;
	}else
	{
		rot_mat(0,0) = cosine;
		rot_mat(0,1) = -sine;
		rot_mat(1,0) = sine;
		rot_mat(1,1) = cosine;
	}
	MAT4D l2g_trans,g2l_trans;
	l2g(l2g_trans);
	g2l(g2l_trans);
	rot_mat = l2g_trans * rot_mat * g2l_trans;
}

void CBodyFrame::make_s_mat(double scale, int axis, MAT4D &s_trans) const
{
	s_trans = MAT4D::IDENTITY;
	if (axis == 0)
	{
		s_trans(0,0) = scale;
	}else if (axis == 1)
	{
		s_trans(1,1) = scale;
	}else
	{
		s_trans(2,2) = scale;	
	}
	MAT4D l2g_trans,g2l_trans;
	l2g(l2g_trans);
	g2l(g2l_trans);
	s_trans = l2g_trans * s_trans * g2l_trans;
}

void CBodyFrame::make_t_mat(double t, int axis, MAT4D &t_trans) const
{
	t_trans = MAT4D::IDENTITY;
	VEC3D translation = m_axis[axis]*t;
	t_trans(0,3) = translation[0];
	t_trans(1,3) = translation[1];
	t_trans(2,3) = translation[2];
}

void CBodyFrame::fromTransformMatrix(const MAT4D &m)
{
	m_mc[0] = m(0,3);	m_mc[1] = m(1,3);	m_mc[2] = m(2,3);
	MAT3D rot_mat = STJUtil::mat4d_2_mat3d(m);

	m_axis[0] = rot_mat.GetColumn(0);
	m_axis[1] = rot_mat.GetColumn(1);
	m_axis[2] = rot_mat.GetColumn(2);

	//make them orthogonal
	m_axis[2].Normalize();
	m_axis[0] = m_axis[0] -  m_axis[2]*(m_axis[0].Dot(m_axis[2]));
	m_axis[0].Normalize();
	m_axis[1] = m_axis[2].Cross(m_axis[0]);
	m_axis[1].Normalize();

	m_rot.FromRotationMatrix(MAT3D(m_axis[0],m_axis[1],m_axis[2],true));
	
	m_rot.Normalize();

#if QUAT0_POSITIVE
	if (m_rot[0]<0)	//new added, let w to be positive, the rotation angle should be within [-180,180]
	{
		//printf("WARNING: bug exists! m_rot[0]<0!\n");
		m_rot = -m_rot;
	}
#endif
}

void CBodyFrame::transform(const MAT4D &trans)
{
	VEC4D center(m_mc[0],m_mc[1],m_mc[2],1.f);
	center = trans * center;
	m_mc[0] = center[0];	m_mc[1] = center[1];	m_mc[2] = center[2];
	MAT3D rot_mat = STJUtil::mat4d_2_mat3d(trans);

	m_axis[0] = rot_mat * m_axis[0];
	m_axis[1] = rot_mat * m_axis[1];
	m_axis[2] = rot_mat * m_axis[2];

	//make them orthogonal
	m_axis[2].Normalize();
	m_axis[0] = m_axis[0] -  m_axis[2]*(m_axis[0].Dot(m_axis[2]));
	m_axis[0].Normalize();
	m_axis[1] = m_axis[2].Cross(m_axis[0]);
	m_axis[1].Normalize();

	m_rot.FromRotationMatrix(MAT3D(m_axis[0],m_axis[1],m_axis[2],true));

	m_rot.Normalize();

#if QUAT0_POSITIVE
	if (m_rot[0]<0)	//new added, let w to be positive, the rotation angle should be within [-180,180]
	{
		//printf("WARNING: bug exists! m_rot[0]<0!\n");
		m_rot = -m_rot;
	}
#endif
}

void CBodyFrame::translate(const VEC3D &t)
{
	m_mc += t;
}

void CBodyFrame::alignTo(const CBodyFrame &rhs, bool bScale)
{
	m_axis[0] = rhs.m_axis[0];
	m_axis[1] = rhs.m_axis[1];
	m_axis[2] = rhs.m_axis[2];
	m_mc = rhs.m_mc;
	if (bScale)
	{
		//we use uniform scaling, and based on the up direction
		double scale = rhs.m_hl[1] / m_hl[1];
		m_hl[0] *= scale;
		m_hl[1] *= scale;
		m_hl[2] *= scale;
		//m_hl[0] = rhs.m_hl[0];
		//m_hl[1] = rhs.m_hl[1];
		//m_hl[2] = rhs.m_hl[2];
		//m_hl[0] = rhs.m_hl[0];
		//m_hl[1] = rhs.m_hl[1];
		//m_hl[2] = rhs.m_hl[2];
	}
}

void CBodyFrame::swap_axis(int idx1, int idx2)
{
	SWAP(m_hl[idx1],m_hl[idx2]);
	VEC3D tmp = m_axis[idx1];
	m_axis[idx1] = m_axis[idx2];
	m_axis[idx2] = -tmp;
}

void CBodyFrame::scale(int axis, double scale)
{
	m_hl[axis] *= scale;
}

void CBodyFrame::scale(double sx, double sy, double sz)
{
	m_hl[0] *= sx;
	m_hl[1] *= sy;
	m_hl[2] *= sz;
}

void CBodyFrame::scale(int axis, double scale, const VEC3D &start_pt)
{
	m_hl[axis] *= scale;
	//also update the position
	VEC3D global_vec = m_mc - start_pt;
	VEC3D local_vec = getInverse().transformVec(global_vec);
	local_vec[axis] *= scale;
	global_vec = transformVec(local_vec);
	m_mc = start_pt + global_vec;
}

void CBodyFrame::scale(double sx, double sy, double sz, const VEC3D &start_pt)
{
	m_hl[0] *= sx;
	m_hl[1] *= sy;
	m_hl[2] *= sz;
	//also update the position
	VEC3D global_vec = m_mc - start_pt;
	VEC3D local_vec = getInverse().transformVec(global_vec);
	local_vec[0] *= sx;
	local_vec[1] *= sy;
	local_vec[2] *= sz;
	global_vec = transformVec(local_vec);
	m_mc = start_pt + global_vec;
}