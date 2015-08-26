#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include <QOpenglTexture>
#include <QModelIndex>
#include <QPair>
#include <QVector4D>
#include <opencv\cxcore.h>
#include <opencv2\highgui\highgui.hpp>
#include "util.h"

//#pragma comment(lib,"PrimitiveShapes.lib")
#include "box.h"
#include "glm.h"
#include "pca.h"
//#include "boxJoint.h"
#include "boxHingeJoint.h"
#include "boxSliderJoint.h"

#define NoCo 0x1001
#define XY   0x1002
#define YZ   0x1003
#define XZ   0x1004
#define Mc 1.0
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

typedef struct vertex{
	float items[8];
}fVertex;

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    GLWidget(QWidget *parent = 0);
    ~GLWidget();
	//**************************************	used by another project		**************************************
	float dpi = 5.0;
	QVector <GLMmodel *> modelList;

	GLMmodel * addModel(QString name);
	GLMmodel * removeModel(QString name);
	modelGuy* currentMesh;
	bool loadTexture(GLMmodel* model);
	QVector<QOpenGLTexture*> texList;

	GLvoid glmVN(GLMmodel* model);
	GLvoid glmBox(GLMmodel* model);
	bool updateBuffer();
	QString fileNameRightClick;
	QVector<modelGuy *> modelManager;
	//*************************************************************************************************************

    QSize minimumSizeHint() const Q_DECL_OVERRIDE;
    QSize sizeHint() const Q_DECL_OVERRIDE;
	QOpenGLTexture* texture;
	
	void add(const QVector3D &v, const QVector3D &n);
	void addV(const QVector3D &v);
	void addTex(const QVector2D &t);
	//void addBox(Box & b);
	void addPointCloud(cv::Mat mask);

	int coord = XZ;
	int m_count;
	int width;
	int height;
	int rawPointCount;
	QVector<GLfloat> m_data;
	
	void draw();
	void drawPoint();
	void drawShape(int begin, int end, QMatrix4x4 boxTransform);
	void drawTriangle(int begin, int end, int color);
	void drawBoxLine();
	
	QImage *rgbMap;
	QImage *shapeColor;
	std::vector<Vec3fShape> vertex;
	std::vector<Vec3fShape> normal;
	std::vector<Box> boxList;
	std::vector<BoxJoint *> jointList;
	PointShapeCloud pc;
	std::vector<std::pair<int, int>> shapeRange;
	QVector<QVector4D> colorList;
	cv::Mat grabResult;
	std::vector<Vec3fShape> normalList;
	Vec3fShape normalGround;
	bool bnormalGroundSet;
	int triangleBegin;
	int triangleEnd;
	void shapeDetect(int signForGround = 0);
	BoxJoint * currentJoint;
	void DrawArrow(float x0, float y0, float z0, float x1, float y1, float z1);
	void DrawTorus(float x0, float y0, float z0, float x1, float y1, float z1);
	void DrawBall(float x0, float y0, float z0, float x1, QMatrix4x4 boxTransform);
	void DrawTorus(float in, float out);
	void DrawJoints();
	int currentBox;
	int currentSelectBox;
	int indexParentBox;
	int indexChildBox;
	GLMmodel * arrowModel;
	GLMmodel * arrowStraightModel;
	GLMmodel * ballModel;
	void drawPlane(Vec3fShape v1, Vec3fShape v2, Vec3fShape v3, Vec3fShape v4);
	void drawModel(GLMmodel *);

public slots:
    void setXRotation(int angle);
    void setYRotation(int angle);
    void setZRotation(int angle);
	void setCamXRotation(int angle);
	void setCamYRotation(int angle);
	void setCamZRotation(int angle);
	void setTrans(float dx, float dy, float dz);
	void meshChanged(int index);
	void nocoClicked();
	void XYClicked();
	void YZClicked();
	void XZClicked();
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
	void jointDoubleClick(const QModelIndex &qm);
	void boxDoubleClick(const QModelIndex &qm);
	void jointSliderValueChanged(int);
	void deleteCurrentBox();
	void parentBoxSelect(int);
	void childBoxSelect(int);
	void planeSelect(int);
	void vertexSelect(int, int);
	void addConstraint(int);

signals:
    void xRotationChanged(int angle);
    void yRotationChanged(int angle);
    void zRotationChanged(int angle);
	void addComboItem(modelGuy* model);
	void refreshComboItem();
	void sliderScaleChanged(int scale);
	void sliderStepChanged(int scale);
	void currentMeshChanged(int index);
	//*************** furniture
	void jointUpdate(std::vector<BoxJoint *> pJointList);
	void boxUpdate(std::vector<Box> pBoxList);
	void jointSliderChanged(double, double, double);
	void boxUpdate(int plane, int point1, int point2);

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
	GLMmodel * model_ptr;
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
	int m_assignedColor;
	int m_assignedMode;
    QMatrix4x4 m_proj;
    QMatrix4x4 m_camera;
    QMatrix4x4 m_world;
	QVector3D eye;
	QVector3D at;
	QVector3D up;
	QVector3D selectRay;
    bool m_transparent;
	float stepDPI;
	float stepLength;
	float nearPlane;
	float length;
	float lengthFormer;
	int interIndex;
	bool mouseMoved;
	int drawShapeWithColor;
	bool doubleClickMask;
};

#endif
