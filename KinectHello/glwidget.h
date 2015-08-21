#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include <QOpenglTexture>
#include <QPair>
#include <QVector4D>
#include <opencv\cxcore.h>
#include <opencv2\highgui\highgui.hpp>

//#pragma comment(lib,"PrimitiveShapes.lib")
#include "RansacShapeDetector.h"
#include "PlanePrimitiveShapeConstructor.h"
#include "PlanePrimitiveShape.h"

//#include "logo.h"
#include "glm.h"
#include "pca.h"

#define NoCo 0x1001
#define XY   0x1002
#define YZ   0x1003
#define XZ   0x1004
#define Mc 10.0
#define HEIGHT_IMG 480
#define WIDTH_IMG 640

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

typedef struct node{
	GLMmodel * model_ptr = NULL;
	QString meshName;
	QMatrix4x4 m_world;
	float scale = 1.0;
	int m_xRot = 0;
	int m_yRot = 0;
	int m_zRot = 0;

	float m_xTrans = 0;
	float m_yTrans = 0;
	float m_zTrans = 0;

	int modelIndex = 0;
} modelGuy;

class triangle{
public:
	triangle(const QVector4D & a0, const QVector4D & a1, const QVector4D & a2):v0(a0),e1(a1),e2(a2){
	
	};
	QVector3D getVectorByNum(int index);
	QVector4D v0;
	QVector4D e1;
	QVector4D e2;
};

class Shape{
public:
	Shape(const int & pbegin , const int & pend ):begin(pbegin),end(pend),next(NULL){

	};
	int begin;
	int end;
	Shape * next;
};

class Box{
public:
	Box(Vec3fShape pcenter, Vec3fShape* pnormal, float* pmax, float* pmin ):boxBegin(0),boxEnd(0){
		Vec3fShape it;
		static int mat[8][3] = {
			{ 0, 1, 1 },
			{ 1, 1, 1 },
			{ 1, 1, 0 },
			{ 0, 1, 0 },
			{ 0, 0, 1 },
			{ 1, 0, 1 },
			{ 1, 0, 0 },
			{ 0, 0, 0 },
		};
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


	};
	Vec3fShape center;
	Vec3fShape normal[3];
	float max[3];
	float min[3];
	int boxBegin;
	int boxEnd;
	Vec3fShape vertex[8];
};

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    GLWidget(QWidget *parent = 0);
    ~GLWidget();

    QSize minimumSizeHint() const Q_DECL_OVERRIDE;
    QSize sizeHint() const Q_DECL_OVERRIDE;
	QOpenGLTexture* texture;
	QVector<QOpenGLTexture*> texList;
	int coord = XZ;
	float dpi = 5.0;
	modelGuy* currentMesh;
	bool loadTexture(GLMmodel* model);
	QVector <GLMmodel *> modelList;
	GLMmodel * addModel(QString name);
	GLMmodel * removeModel(QString name);
	QVector<GLfloat> m_data;
	int m_count;
	int width;
	int height;
	int rawPointCount;
	void add(const QVector3D &v, const QVector3D &n);
	void addV(const QVector3D &v);
	void addTex(const QVector2D &t);
	void addBox(Box & b);
	void addPointCloud(cv::Mat mask);
	GLvoid glmVN(GLMmodel* model);
	GLvoid glmBox(GLMmodel* model);
	void draw();
	bool updateBuffer();
	void drawPoint();
	void drawShape(int begin, int end, int color);
	void drawTriangle(int begin, int end, int color);
	void drawBoxLine();
	QString fileNameRightClick;
	QVector<modelGuy *> modelManager;
	QImage *rgbMap;
	QImage *shapeColor;
	std::vector<Vec3fShape> vertex;
	std::vector<Vec3fShape> normal;
	std::vector<Box> boxList;
	PointShapeCloud pc;
	std::vector<std::pair<int, int>> shapeRange;
	QVector<QVector3D> colorList;
	cv::Mat grabResult;
	void shapeDetect(int signForGround = 0);
	std::vector<Vec3fShape> normalList;
	Vec3fShape normalGround;
	int triangleBegin;
	int triangleEnd;
	

public slots:
    void setXRotation(int angle);
    void setYRotation(int angle);
    void setZRotation(int angle);
	void setCamXRotation(int angle);
	void setCamYRotation(int angle);
	void setCamZRotation(int angle);
	void setTrans(float dx, float dy, float dz);
	void meshChanged(int index);
	void nocoClicked(bool b);
	void XYClicked(bool b);
	void YZClicked(bool b);
	void XZClicked(bool b);
    void cleanup();
	void addMesh(QString name);
	void addMeshRightClick();
	void removeMesh(QString name);
	void removeCurrentMesh();
	void sliderScale(int scale);
	void sliderStep(int scale);
	void setTransMode(int);
	void addMeshDoubleClick();
	void changeDrawShape();
	void boxTest();

signals:
    void xRotationChanged(int angle);
    void yRotationChanged(int angle);
    void zRotationChanged(int angle);
	void addComboItem(modelGuy* model);
	void refreshComboItem();
	void sliderScaleChanged(int scale);
	void sliderStepChanged(int scale);
	void currentMeshChanged(int index);

protected:
    void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void resizeGL(int width, int height) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	void keyPressEvent(QKeyEvent * event) Q_DECL_OVERRIDE;
	void keyReleaseEvent(QKeyEvent * event) Q_DECL_OVERRIDE;
	void wheelEvent(QWheelEvent * event) Q_DECL_OVERRIDE;

private:
    void setupVertexAttribs();
	int iTransMode;
    bool m_core;
    int m_xRot;
    int m_yRot;
    int m_zRot;
    QPoint m_lastPos;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_logoVbo;
    QOpenGLShaderProgram *m_program;
    int m_projMatrixLoc;
    int m_mvMatrixLoc;
    int m_normalMatrixLoc;
    int m_lightPosLoc;
	int m_hasTex;
	int m_fixedPipeline;
	int m_fixedColor;
	int m_mapColor;
	int matAmbientLoc;
	int matDiffuseLoc;
	int matSpecularLoc;
	int matShineLoc;
	int m_viewPos;
	int m_specificColor;
	int m_colorAssigned;
	int m_shapeColor;
    QMatrix4x4 m_proj;
    QMatrix4x4 m_camera;
    QMatrix4x4 m_world;
	QVector3D eye;
	QVector3D at;
	QVector3D up;
	QVector3D selectRay;
    bool m_transparent;
	GLMmodel * model_ptr;
	float stepDPI;
	float stepLength;
	float nearPlane;
	float length;
	float lengthFormer;
	int interIndex;
	bool mouseMoved;
	int drawShapeWithColor;
};

#endif
