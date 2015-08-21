#include "glwidget.h"
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QCoreApplication>
#include <QDir>
#include <QMessageBox>
#include <QVector4D>
#include <math.h>
#define T(x) (model->triangles[(x)])

//#pragma comment(lib,"PrimitiveShapes.lib")

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      m_xRot(0),
      m_yRot(0),
      m_zRot(0),
      m_program(0),
	  m_count(0),
	  coord(XZ),
	  iTransMode(0),
	  stepDPI(50),
	  stepLength(50),
	  nearPlane(0.01),
	  currentMesh(NULL),
	  rgbMap(NULL),
	  drawShapeWithColor(1),
	  grabResult(480,640,CV_8UC1),
	  rawPointCount(0),
	  triangleBegin(0),
	  triangleEnd(0)
{
    //m_core = QCoreApplication::arguments().contains(QStringLiteral("--coreprofile"));
    // --transparent causes the clear color to be transparent. Therefore, on systems that
    // support it, the widget will become transparent apart from the logo.
    m_transparent = QCoreApplication::arguments().contains(QStringLiteral("--transparent"));
    if (m_transparent)
        setAttribute(Qt::WA_TranslucentBackground);
	m_data.reserve(640 * 480 *24);
	//colorList.push_back(QVector3D(108 / 255.0f, 7 / 255.0f, 85 / 255.0f));
	//colorList.push_back(QVector3D(120 / 255.0f, 0 / 255.0f, 101 / 255.0f));
	//colorList.push_back(QVector3D(211 / 255.0f, 0 / 255.0f, 0 / 255.0f));
	colorList.push_back(QVector3D(255 / 255.0f, 255 / 255.0f, 0 / 255.0f));
	colorList.push_back(QVector3D(255 / 255.0f, 255 / 255.0f, 255 / 255.0f));
	colorList.push_back(QVector3D(0 / 255.0f, 255 / 255.0f, 255 / 255.0f));
	colorList.push_back(QVector3D(255 / 255.0f, 0 / 255.0f, 255 / 255.0f));

	colorList.push_back(QVector3D(0.0, 0.9f, 0.0f));
	colorList.push_back(QVector3D(0.0, 0.0f, 0.9f));
	colorList.push_back(QVector3D(0.729, 0.004f, 1.0f));
	colorList.push_back(QVector3D(0.639, 0.376f, 0.594f));
	colorList.push_back(QVector3D(1.0, 0.310f, 0.322f));
}

GLWidget::~GLWidget()
{
    cleanup();
	for (size_t i = 0; i < texList.size(); i++)
	{
		delete texList.at(i);
	}
	for (size_t i = 0; i < modelList.size(); i++)
	{
		delete modelList.at(i);
	}
	for (size_t i = 0; i < modelManager.size(); i++)
	{
		delete modelManager.at(i);

	}
}

QSize GLWidget::minimumSizeHint() const
{
    return QSize(320, 240);
}

QSize GLWidget::sizeHint() const
{
    return QSize(800, 600);
}

static void qNormalizeAngle(int &angle)
{
    while (angle < 0)
        angle += 360 * 16;
    while (angle > 360 * 16)
        angle -= 360 * 16;
}

void GLWidget::setXRotation(int angle)
{
    qNormalizeAngle(angle);
	if (currentMesh)
		currentMesh->m_xRot += angle;
	else
		m_xRot += angle;
    emit xRotationChanged(angle);
    update();
}

void GLWidget::setYRotation(int angle)
{
    qNormalizeAngle(angle);
	if (currentMesh)
		currentMesh->m_yRot += angle;
	else
		m_yRot += angle;
    emit yRotationChanged(angle);
    update();
}

void GLWidget::setZRotation(int angle)
{
    qNormalizeAngle(angle);
	if (currentMesh)
		currentMesh->m_zRot += angle;
	else
		m_zRot += angle;
	emit zRotationChanged(angle);
    update();
}

void GLWidget::setCamXRotation(int angle)
{
	qNormalizeAngle(angle);
	m_xRot += angle;
	update();
}

void GLWidget::setCamYRotation(int angle)
{
	qNormalizeAngle(angle);
	m_yRot += angle;
	update();
}

void GLWidget::setCamZRotation(int angle)
{
	qNormalizeAngle(angle);
	m_zRot += angle;
	update();
}

void GLWidget::meshChanged(int index){
	if (index > 0){
		currentMesh = modelManager.at(index - 1);
		emit sliderScaleChanged(currentMesh->scale * dpi);
	}
	else
		currentMesh = NULL;
	this->setFocus();
}

void GLWidget::nocoClicked(bool b){
	if (b)
		coord = NoCo;
	update();
}

void GLWidget::XYClicked(bool b){
	if (b)
		coord = XY;
	update();
}
void GLWidget::YZClicked(bool b){
	if (b)
		coord = YZ;
	update();
}
void GLWidget::XZClicked(bool b){
	if (b)
		coord = XZ;
	update();
}
void GLWidget::cleanup()
{
    makeCurrent();
    m_logoVbo.destroy();
    delete m_program;
    m_program = 0;
    doneCurrent();
}

static const char *vertexShaderSource =
    "attribute vec4 vertex;\n"
    "attribute vec3 normal;\n"
	"attribute vec2 texCoords;\n"
	"varying vec2 v_texCoord;\n"
    "varying vec3 vert;\n"
    "varying vec3 vertNormal;\n"
    "uniform mat4 projMatrix;\n"
    "uniform mat4 mvMatrix;\n"
    "uniform mat3 normalMatrix;\n"
    "void main() {\n"
    "   vert = vertex.xyz;\n"
    "   vertNormal = normalMatrix * normal;\n"
    "   gl_Position = projMatrix * mvMatrix * vertex;\n"
	"	v_texCoord = texCoords;\n"
    "}\n";

static const char *fragmentShaderSource =
	"varying highp vec3 vert;\n"
	"varying highp vec3 vertNormal;\n"
	"varying vec2 v_texCoord;\n"
	"uniform vec3 viewPos;\n"
	"uniform vec3 mAmbient;\n"
	"uniform vec3 mDiffuse;\n"
	"uniform vec3 mSpecular;\n"
	"uniform vec4 fixedColor;\n"
	"uniform float mShininess;\n"
	"uniform sampler2D s_baseMap;\n"
	"uniform highp vec3 lightPos;\n"
	"uniform bool hasTex;\n"
	"uniform bool fixedPipeline;\n"
	"void main() {\n"
	"	if(fixedPipeline){\n"
	"		gl_FragColor = fixedColor;\n"
	"		return ;\n"
	"	}\n"
	"	vec4 baseColor = texture2D(s_baseMap, v_texCoord);\n"
    "   highp vec3 L = normalize(lightPos - vert);\n"
    "   highp float NL = max(dot(normalize(vertNormal), L), 0.0);\n"
	"   vec3 lightColor = vec3(1.0, 1.0, 1.0);\n"
	"	vec3 ambient = lightColor * mAmbient;\n"
	"	vec3 diffuse = lightColor * (NL * mDiffuse);\n"
	"	vec3 viewDir = normalize(viewPos - vert);\n"
	"	vec3 reflectDir = reflect(-L, normalize(vertNormal));\n"
	"	float spec = pow(max(dot(viewDir, reflectDir), 0.0), mShininess);\n"
	"	vec3 specular = lightColor * (spec * mSpecular);\n"
	"	specular = clamp(specular, 0.0, 1.0);\n"
	"	vec3 result = clamp(ambient + diffuse + specular, 0.0, 1.0);\n"
	"	if(hasTex)\n"
	"		gl_FragColor = baseColor * vec4(result, 1.0);\n"
	"   else\n"
	"		gl_FragColor = vec4(result, 1.0);\n"
    "}\n";

void GLWidget::initializeGL()
{
    // In this example the widget's corresponding top-level window can change
    // several times during the widget's lifetime. Whenever this happens, the
    // QOpenGLWidget's associated context is destroyed and a new one is created.
    // Therefore we have to be prepared to clean up the resources on the
    // aboutToBeDestroyed() signal, instead of the destructor. The emission of
    // the signal will be followed by an invocation of initializeGL() where we
    // can recreate all resources.
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &GLWidget::cleanup);

    initializeOpenGLFunctions();
	//glClearColor(0.285, 0.575, 0.285, m_transparent ? 0 : 1);
	glClearColor(0.3, 0.3, 0.3, m_transparent ? 0 : 1);

    m_program = new QOpenGLShaderProgram(this);
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->bindAttributeLocation("vertex", 0);
	m_program->bindAttributeLocation("normal", 1); 
	m_program->bindAttributeLocation("texCoords", 2);
    m_program->link();

    m_program->bind();
    m_projMatrixLoc = m_program->uniformLocation("projMatrix");
    m_mvMatrixLoc = m_program->uniformLocation("mvMatrix");
    m_normalMatrixLoc = m_program->uniformLocation("normalMatrix");
    m_lightPosLoc = m_program->uniformLocation("lightPos");
	m_hasTex = m_program->uniformLocation("hasTex");
	m_fixedPipeline = m_program->uniformLocation("fixedPipeline");
	m_fixedColor = m_program->uniformLocation("fixedColor");
	matAmbientLoc = m_program->uniformLocation("mAmbient");
	matDiffuseLoc = m_program->uniformLocation("mDiffuse");
	matSpecularLoc = m_program->uniformLocation("mSpecular");
	matShineLoc = m_program->uniformLocation("mShininess");
	m_viewPos = m_program->uniformLocation("viewPos");
	//m_shapeColor = m_program->uniformLocation("shapeColor");
	//m_colorAssigned = m_program->uniformLocation("colorAssigned");

    // Create a vertex array object. In OpenGL ES 2.0 and OpenGL 2.x
    // implementations this is optional and support may not be present
    // at all. Nonetheless the below code works in all cases and makes
    // sure there is a VAO when one is needed. 
	m_vao.create();
	QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao); 

    // Setup our vertex buffer object.
    m_logoVbo.create();
    m_logoVbo.bind();
	
	m_logoVbo.allocate(m_data.constData(), m_count * sizeof(GLfloat));

    // Store the vertex attribute bindings for the program.
    setupVertexAttribs();

    // Our camera never changes in this example.
    m_camera.setToIdentity();
    
	eye = QVector3D(0, 1, 4);
	up = QVector3D(0, 1, 0);
	m_camera.lookAt(eye, QVector3D(0, 0, 0), up);

    // Light position is fixed.
    m_program->setUniformValue(m_lightPosLoc, QVector3D(0, 15, 0));
	m_program->setUniformValue(m_viewPos, eye);

	m_program->setUniformValue("s_baseMap", 0);

	if (rgbMap){
		texture = new QOpenGLTexture(*rgbMap);
	}
	shapeColor = new QImage(640,480,QImage::Format_RGB888);

	m_program->release();
}

bool GLWidget::loadTexture(GLMmodel* model){
	QString file;
	GLMgroup* group;
	group = model->groups;
	GLMtriangle* triangle;
	GLuint i;
	while (group) {
		model->materials[group->material].texIndex = texList.size();
		//if (QString(model->materials[group->material].name)!=QString("default")){
		QDir dir(model->pathname);
		QString dirName = QString("Meshes/") + dir.dirName();
		dirName = dirName.section('.', 0, 0);
		dirName += "/";

		if (model->materials[group->material].hasTex){
			file = (dirName + QString(model->materials[group->material].texName));
			QDir dirr(file);
			if (!dirr.exists())
				qWarning("Cannot find the example directory");
			
			texture = new QOpenGLTexture(QImage(file).mirrored());
			//glDrawArrays(GL_TRIANGLES, group->arrayBegin, group->numtriangles * 3);
			texList.append(texture);
		}
		group = group->next;
	}
	return true;
}

void GLWidget::setupVertexAttribs()
{
    m_logoVbo.bind();
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    f->glEnableVertexAttribArray(0);
    f->glEnableVertexAttribArray(1);
	f->glEnableVertexAttribArray(2);

	f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), 0);
	f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
	f->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void *>(6 * sizeof(GLfloat)));
		
    m_logoVbo.release();
}

void GLWidget::sliderScale(int scale){
	if (currentMesh)
	{
		currentMesh->scale = scale / dpi;
	}
	update();
	emit sliderScaleChanged(scale);
	this->setFocus();
}

void GLWidget::sliderStep(int scale){
	stepLength = scale;
	emit sliderStepChanged(scale);
	this->setFocus();
}

void drawcoordinate(QOpenGLShaderProgram *m_program, int coord, QMatrix4x4 m_world, int m_fixedColor){
	float cc[4] = { 0, 0, 0, 0 };

	QVector4D normal(0.5f, 0.5f, 0.5f, 0.0f);
	m_program->setUniformValue(m_fixedColor, normal);
	
	switch (coord){
	case NoCo:break;
	case XY:{		

		glPushMatrix();
		glBegin(GL_LINES);
		for (float x = -Mc; x <= Mc; x += 1){
			if (abs(x - 0.0f) < 0.0000000001)
				continue;
			glVertex3f(x, -Mc, 0); glVertex3f(x, Mc, 0);
		}
		for (float y = -Mc; y <= Mc; y += 1){
			if (abs(y - 0.0f) < 0.0000000001)
				continue;
			glVertex3f(-Mc, y, 0); glVertex3f(Mc, y, 0);
		}
		glEnd();
		glPopMatrix();

		m_program->setUniformValue(m_fixedColor, QVector4D(0.0f, 1.0f, 0.0f, 0.0f));
		glPushMatrix();
		glBegin(GL_LINES);
		glVertex3f(0, -Mc, 0); glVertex3f(0, Mc, 0);
		glEnd();
		glPopMatrix();

		m_program->setUniformValue(m_fixedColor, QVector4D(1.0f, 0.0f, 0.0f, 0.0f));
		glPushMatrix();
		glBegin(GL_LINES);
		glVertex3f(-Mc, 0, 0); glVertex3f(Mc, 0, 0);
		glEnd();
		glPopMatrix();
		break;
	}
	case YZ:{
		
		glMaterialfv(GL_FRONT, GL_DIFFUSE, cc);
		glPushMatrix();
		glBegin(GL_LINES);
		for (float y = -Mc; y <= Mc; y += 1){
			if (abs(y - 0.0f) < 0.0000000001)
				continue;
			glVertex3f(0, y, -Mc); glVertex3f(0, y, Mc);
		}
		for (float z = -Mc; z <= Mc; z += 1){
			if (abs(z - 0.0f) < 0.0000000001)
				continue;
			glVertex3f(0, -Mc, z); glVertex3f(0, Mc, z);
		}
		glEnd();
		glColor3d(1, 1, 1);
		glPopMatrix();

		m_program->setUniformValue(m_fixedColor, QVector4D(0.0f, 1.0f, 0.0f, 0.0f));
		glPushMatrix();
		glBegin(GL_LINES);
		glVertex3f(0, -Mc, 0); glVertex3f(0, Mc, 0);
		glEnd();
		glPopMatrix();

		m_program->setUniformValue(m_fixedColor, QVector4D(0.0f, 0.0f, 1.0f, 0.0f));
		glPushMatrix();
		glBegin(GL_LINES);
		glVertex3f(0, 0, -Mc); glVertex3f(0, 0, Mc);
		glEnd();
		glPopMatrix();
		break;
	}
	case XZ:{
		glMaterialfv(GL_FRONT, GL_DIFFUSE, cc);
		glPushMatrix();
		glBegin(GL_LINES);
		for (float z = -Mc; z <= Mc; z += 1){
			if (abs(z - 0.0f) < 0.0000000001)
				continue;
			glVertex3f(-Mc, 0, z); glVertex3f(Mc, 0, z);
		}
		for (float x = -Mc; x <= Mc; x += 1){
			if (abs(x - 0.0f) < 0.0000000001)
				continue;
			glVertex3f(x, 0, -Mc); glVertex3f(x, 0, Mc);
		}
		glEnd();
		glColor3d(1, 1, 1);
		glPopMatrix();

		m_program->setUniformValue(m_fixedColor, QVector4D(0.0f, 0.0f, 1.0f, 0.0f));
		glPushMatrix();
		glBegin(GL_LINES);
		glVertex3f(0, 0, -Mc); glVertex3f(0, 0, Mc);
		glEnd();
		glPopMatrix();

		m_program->setUniformValue(m_fixedColor, QVector4D(1.0f, 0.0f, 0.0f, 0.0f));
		glPushMatrix();
		glBegin(GL_LINES);
		glVertex3f(-Mc, 0, 0); glVertex3f(Mc, 0, 0);
		glEnd();
		glPopMatrix();
		break;
	}
	}
}

void GLWidget::drawBoxLine(){
	
	for (size_t i = 0; i < boxList.size(); i++)
	{
		m_program->setUniformValue(m_fixedColor, QVector4D(colorList.at(i % colorList.size())));

		glPushMatrix();
		glBegin(GL_LINES);
		glVertex3f(boxList.at(i).vertex[0][0], boxList.at(i).vertex[0][1], boxList.at(i).vertex[0][2]);
		glVertex3f(boxList.at(i).vertex[1][0], boxList.at(i).vertex[1][1], boxList.at(i).vertex[1][2]);
		glEnd();

		glBegin(GL_LINES);
		glVertex3f(boxList.at(i).vertex[1][0], boxList.at(i).vertex[1][1], boxList.at(i).vertex[1][2]); 
		glVertex3f(boxList.at(i).vertex[2][0], boxList.at(i).vertex[2][1], boxList.at(i).vertex[2][2]);
		glEnd();

		glBegin(GL_LINES);
		glVertex3f(boxList.at(i).vertex[2][0], boxList.at(i).vertex[2][1], boxList.at(i).vertex[2][2]);
		glVertex3f(boxList.at(i).vertex[3][0], boxList.at(i).vertex[3][1], boxList.at(i).vertex[3][2]);
		glEnd();

		glBegin(GL_LINES);
		glVertex3f(boxList.at(i).vertex[3][0], boxList.at(i).vertex[3][1], boxList.at(i).vertex[3][2]);
		glVertex3f(boxList.at(i).vertex[0][0], boxList.at(i).vertex[0][1], boxList.at(i).vertex[0][2]);
		glEnd();

		glBegin(GL_LINES);
		glVertex3f(boxList.at(i).vertex[0][0], boxList.at(i).vertex[0][1], boxList.at(i).vertex[0][2]);
		glVertex3f(boxList.at(i).vertex[4][0], boxList.at(i).vertex[4][1], boxList.at(i).vertex[4][2]);
		glEnd();

		glBegin(GL_LINES);
		glVertex3f(boxList.at(i).vertex[1][0], boxList.at(i).vertex[1][1], boxList.at(i).vertex[1][2]);
		glVertex3f(boxList.at(i).vertex[5][0], boxList.at(i).vertex[5][1], boxList.at(i).vertex[5][2]);
		glEnd();

		glBegin(GL_LINES);
		glVertex3f(boxList.at(i).vertex[2][0], boxList.at(i).vertex[2][1], boxList.at(i).vertex[2][2]);
		glVertex3f(boxList.at(i).vertex[6][0], boxList.at(i).vertex[6][1], boxList.at(i).vertex[6][2]);
		glEnd();

		glBegin(GL_LINES);
		glVertex3f(boxList.at(i).vertex[3][0], boxList.at(i).vertex[3][1], boxList.at(i).vertex[3][2]);
		glVertex3f(boxList.at(i).vertex[7][0], boxList.at(i).vertex[7][1], boxList.at(i).vertex[7][2]);
		glEnd();

		glBegin(GL_LINES);
		glVertex3f(boxList.at(i).vertex[7][0], boxList.at(i).vertex[7][1], boxList.at(i).vertex[7][2]);
		glVertex3f(boxList.at(i).vertex[4][0], boxList.at(i).vertex[4][1], boxList.at(i).vertex[4][2]);
		glEnd();

		glBegin(GL_LINES);
		glVertex3f(boxList.at(i).vertex[4][0], boxList.at(i).vertex[4][1], boxList.at(i).vertex[4][2]);
		glVertex3f(boxList.at(i).vertex[5][0], boxList.at(i).vertex[5][1], boxList.at(i).vertex[5][2]);
		glEnd();

		glBegin(GL_LINES);
		glVertex3f(boxList.at(i).vertex[5][0], boxList.at(i).vertex[5][1], boxList.at(i).vertex[5][2]);
		glVertex3f(boxList.at(i).vertex[6][0], boxList.at(i).vertex[6][1], boxList.at(i).vertex[6][2]);
		glEnd();

		glBegin(GL_LINES);
		glVertex3f(boxList.at(i).vertex[6][0], boxList.at(i).vertex[6][1], boxList.at(i).vertex[6][2]);
		glVertex3f(boxList.at(i).vertex[7][0], boxList.at(i).vertex[7][1], boxList.at(i).vertex[7][2]);
		glEnd();

		glPopMatrix();

	}
	
	
	
}

void GLWidget::paintGL()
{

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	texture->setMagnificationFilter(QOpenGLTexture::Linear);

	if (!drawShapeWithColor){
		if (rgbMap)
			texture->setData(*rgbMap);
	}
	else
		texture->setData(*shapeColor);

	texture->bind();

    m_world.setToIdentity();

    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);
    m_program->bind();
    m_program->setUniformValue(m_projMatrixLoc, m_proj);
    
    QMatrix3x3 normalMatrix = m_world.normalMatrix();
    m_program->setUniformValue(m_normalMatrixLoc, normalMatrix);
	
	m_program->setUniformValue(m_fixedPipeline, true);
	m_program->setUniformValue(m_mvMatrixLoc, m_camera * m_world);
	drawcoordinate(m_program, coord, m_world, m_fixedColor);

	m_world.rotate((m_xRot / 16.0f), 1, 0, 0);
	m_world.rotate(m_yRot / 16.0f, 0, 1, 0);
	m_world.rotate(m_zRot / 16.0f, 0, 0, 1);
	
	m_world.rotate(180, 0, 0, 1);
	m_world.rotate(180, 0, 1, 0);
	m_world.translate(0, 0, -1.5);

	m_program->setUniformValue(m_fixedPipeline, false);
	//m_program->setUniformValue(m_colorAssigned, false);
	m_program->setUniformValue(m_fixedColor, QVector4D(0.0f, 1.0f, 0.0f, 0.0f));

	for (int i = 0; i < shapeRange.size(); i++)
		drawShape(shapeRange[i].first, shapeRange[i].second, i % colorList.size());

	m_program->setUniformValue(m_fixedPipeline, true);

	if (!drawShapeWithColor)
		drawBoxLine();
	m_program->release();
}

void GLWidget::changeDrawShape(){
	drawShapeWithColor = 1 - drawShapeWithColor;
	update();
}

void GLWidget::drawTriangle(int begin, int end, int color){
	m_logoVbo.bind();
	m_logoVbo.allocate(m_data.constData(), m_count * sizeof(GLfloat));
	m_program->setUniformValue(m_hasTex, true);
	m_program->setUniformValue(m_mvMatrixLoc, m_camera * m_world);
	m_program->setUniformValue(m_normalMatrixLoc, (m_world).normalMatrix());
	m_program->setUniformValue(matAmbientLoc, QVector3D(0.4, 0.4, 0.4));
	m_program->setUniformValue(matDiffuseLoc, QVector3D(0.6, 0.6, 0.6));
	m_program->setUniformValue(matSpecularLoc, QVector3D(0.0, 0.0, 0.0));
	m_program->setUniformValue(matShineLoc, (GLfloat)65.0);
	glPointSize(3);
	glDrawArrays(GL_TRIANGLES, begin, end);
}

void GLWidget::drawShape(int begin, int end, int color){
	m_logoVbo.bind();
	m_logoVbo.allocate(m_data.constData(), m_count * sizeof(GLfloat));
	m_program->setUniformValue(m_hasTex, true);
	m_program->setUniformValue(m_mvMatrixLoc, m_camera * m_world);
	m_program->setUniformValue(m_normalMatrixLoc, (m_world).normalMatrix());
	m_program->setUniformValue(matAmbientLoc, QVector3D(0.4, 0.4, 0.4));
	m_program->setUniformValue(matDiffuseLoc, QVector3D(0.6, 0.6, 0.6));
	m_program->setUniformValue(matSpecularLoc, QVector3D(0.0, 0.0, 0.0));
	m_program->setUniformValue(matShineLoc, (GLfloat)65.0);

	glPointSize(3);
	glDrawArrays(GL_POINTS, begin, end);

}

void GLWidget::drawPoint(){

	m_logoVbo.bind();
	m_logoVbo.allocate(m_data.constData(), m_count * sizeof(GLfloat));

	m_program->setUniformValue(m_mvMatrixLoc, m_camera * m_world);
	m_program->setUniformValue(m_normalMatrixLoc, (m_world).normalMatrix());
	m_program->setUniformValue(m_hasTex, true);
	m_program->setUniformValue(matAmbientLoc, QVector3D(0.4, 0.4, 0.4));
	m_program->setUniformValue(matDiffuseLoc, QVector3D(0.6, 0.6, 0.6));
	m_program->setUniformValue(matSpecularLoc, QVector3D(0.0, 0.0, 0.0));
	m_program->setUniformValue(matShineLoc, (GLfloat)65.0);
	glPointSize(3);
	glDrawArrays(GL_POINTS, 0, rawPointCount);

}

void GLWidget::draw(){

	for (int j = 0; j < modelManager.size(); ++j) {
		GLMmodel* model = modelManager.at(j)->model_ptr;
		modelGuy* modelM = modelManager.at(j);
		if (!model->toDraw)
			continue;
		modelM->m_world.setToIdentity();
		modelM->m_world.translate(modelM->m_xTrans, modelM->m_yTrans, modelM->m_zTrans);
		modelM->m_world.rotate(modelM->m_xRot / 16.0f, 1, 0, 0);
		modelM->m_world.rotate(modelM->m_yRot / 16.0f, 0, 1, 0);
		modelM->m_world.rotate(modelM->m_zRot / 16.0f, 0, 0, 1);
		modelM->m_world.scale(modelM->scale);

		GLMgroup* group;
		group = model->groups;
		GLMtriangle* triangle;
		GLuint i;
		while (group) {
			if (model->nummaterials > 0 && model->materials[group->material].hasTex)
			{
				if (!model->texLoaded){
					loadTexture(model);
					model->texLoaded = true;
				}
				
				//texture = new QOpenGLTexture(QImage("Meshes/arakkoa/Arakkoa_Gray.png").mirrored());
				texList[model->materials[group->material].texIndex]->bind();
				//QOpenGLTexture * haha = this->texList[model->materials[group->material].texIndex];
				//texture->bind();
			}
			m_program->setUniformValue(m_mvMatrixLoc, m_camera * m_world * modelM->m_world);
			m_program->setUniformValue(m_normalMatrixLoc, (/*m_world * */modelM->m_world).normalMatrix());
			if (model->nummaterials > 0){
				m_program->setUniformValue(m_hasTex, model->materials[group->material].hasTex);
				m_program->setUniformValue(matAmbientLoc, QVector3D(model->materials[group->material].ambient[0], model->materials[group->material].ambient[1], model->materials[group->material].ambient[2]/*, model->materials[group->material].ambient[3]*/));
				m_program->setUniformValue(matDiffuseLoc, QVector3D(model->materials[group->material].diffuse[0], model->materials[group->material].diffuse[1], model->materials[group->material].diffuse[2]/*, model->materials[group->material].diffuse[3]*/));
				m_program->setUniformValue(matSpecularLoc, QVector3D(model->materials[group->material].specular[0], model->materials[group->material].specular[1], model->materials[group->material].specular[2]/*, model->materials[group->material].specular[3]*/));
				m_program->setUniformValue(matShineLoc, model->materials[group->material].shininess);
			}
			else
			{
				m_program->setUniformValue(m_hasTex, false);
				m_program->setUniformValue(matAmbientLoc, QVector3D(0.4,0.4,0.4));
				m_program->setUniformValue(matDiffuseLoc, QVector3D(0.6, 0.6, 0.6));
				m_program->setUniformValue(matSpecularLoc, QVector3D(0.0, 0.0, 0.0));
				m_program->setUniformValue(matShineLoc, (GLfloat)65.0);
			}
			glDrawArrays(GL_TRIANGLES, group->arrayBegin, group->numtriangles * 3);
			group = group->next;
		}
	}
}

void GLWidget::resizeGL(int w, int h)
{
    m_proj.setToIdentity();
	m_proj.perspective(45.0f, GLfloat(w) / h, nearPlane, 1000.0f);
	width = w;
	height = h;
	glViewport(0,0,w,h);
}

float rayTriangleIntersection(const QVector3D &pos, const QVector3D &dir, const QVector3D & v0, const QVector3D & edge1, const QVector3D & edge2){
	QVector3D rayPos = pos;

	QVector3D rayDir = dir;

	QVector3D tvec = rayPos - v0;
	QVector3D pvec = QVector3D::crossProduct(rayDir, edge2);
	float  det = QVector3D::dotProduct(edge1, pvec);
	//det = __fdividef(1.0f, det);
	det = 1.0f / det;

	float u = QVector3D::dotProduct(tvec, pvec) * det;

	if (u < 0.0f || u > 1.0f){
		return -1.0f;
	}

	QVector3D qvec = QVector3D::crossProduct(tvec, edge1);

	float v = QVector3D::dotProduct(rayDir, qvec) * det;


	if (v < 0.0f || (u + v) > 1.0f){
		return -1.0f;
	}
	float tt = QVector3D::dotProduct(edge2, qvec) * det;
	return tt;
}

void cam2tex(int& x, int& y, float depth, float camx, float camy){
	x = (camx * 571.401f) / depth + 319.5f;
	y = (camy * 571.401f) / depth + 239.5f;
}

void addBoxPoint(GLWidget * gl, const Box &b, int x, int y, int z){
	Vec3fShape it;
	it = b.center
		+ x * b.normal[0] * b.max[0] + (1 - x)* b.normal[0] * b.min[0]
		+ y * b.normal[1] * b.max[1] + (1 - y)* b.normal[1] * b.min[1]
		+ z * b.normal[2] * b.max[2] + (1 - z)* b.normal[2] * b.min[2];

	gl->add(QVector3D(it[0], it[1], it[2]), QVector3D(b.normal[0][0], b.normal[0][1], b.normal[0][2]));
	gl->addTex(QVector2D(0.0f, 0.0f));
}

void GLWidget::addBox(Box & b){
	b.boxBegin = m_count / 8;
	addBoxPoint(this, b, 0, 1, 1);
	addBoxPoint(this, b, 1, 1, 1);
	addBoxPoint(this, b, 1, 1, 0);

	addBoxPoint(this, b, 1, 1, 0);
	addBoxPoint(this, b, 0, 1, 0);
	addBoxPoint(this, b, 0, 1, 1);

	addBoxPoint(this, b, 0, 0, 1);
	addBoxPoint(this, b, 0, 1, 1);
	addBoxPoint(this, b, 0, 1, 0);

	addBoxPoint(this, b, 0, 1, 0);
	addBoxPoint(this, b, 0, 0, 0);
	addBoxPoint(this, b, 0, 0, 1);

	addBoxPoint(this, b, 0, 0, 0);
	addBoxPoint(this, b, 1, 0, 0);
	addBoxPoint(this, b, 1, 1, 0);

	addBoxPoint(this, b, 1, 1, 0);
	addBoxPoint(this, b, 0, 1, 0);
	addBoxPoint(this, b, 0, 0, 0);
	b.boxEnd = m_count / 8;
}

void GLWidget::addPointCloud(cv::Mat mask){
	Vec3fShape min, max;
	int index = 0;
	this->height = mask.rows;
	this->width = mask.cols;
	for (int y = 0; y < this->height; ++y) {
		for (int x = 0; x < this->width; ++x) {
			index = y * width + x;
			if (index == 0)
			{
				min = vertex[0];
				max = vertex[0];
			}
			if (vertex[index].length() < 0.000000001)
				continue;
			if (mask.empty())
				pc.push_back(PointShape(vertex[index], normal[index]));
			else{
				if (!(int)mask.at<unsigned char>(y, x))
					continue;
				pc.push_back(PointShape(vertex[index], normal[index]));
			}

			if (vertex[index][0] < min[0])
				min[0] = vertex[index][0];
			if (vertex[index][1] < min[1])
				min[1] = vertex[index][1];
			if (vertex[index][2] < min[2])
				min[2] = vertex[index][2];
			if (vertex[index][0] > max[0])
				max[0] = vertex[index][0];
			if (vertex[index][1] > max[1])
				max[1] = vertex[index][1];
			if (vertex[index][2] > max[2])
				max[2] = vertex[index][2];
		}
	}
	int size = pc.size();
	pc.setBBox(min, max);

}

void GLWidget::boxTest(){
	cv::Mat back;

	cv::FileStorage fs("data0_ground.xml", cv::FileStorage::READ);
	fs["vocabulary"] >> back;
	addPointCloud(back);
	shapeDetect(1);
	shapeRange.clear();
	fs.release();

	//pc.clear();
	//addPointCloud(grabResult);
	//shapeDetect();

	pc.clear();
	fs.open("data0_cabnet.xml", cv::FileStorage::READ);
	fs["vocabulary"] >> back;
	addPointCloud(back);
	shapeDetect();
	fs.release();

	pc.clear();
	fs.open("data0_drawer1.xml", cv::FileStorage::READ);
	fs["vocabulary"] >> back;
	addPointCloud(back);
	shapeDetect();
	fs.release();

	pc.clear();
	fs.open("data0_drawer2.xml", cv::FileStorage::READ);
	fs["vocabulary"] >> back;
	addPointCloud(back);
	shapeDetect();
	fs.release();

	pc.clear();
	fs.open("data0_door1.xml", cv::FileStorage::READ);
	fs["vocabulary"] >> back;
	addPointCloud(back);
	shapeDetect();
	fs.release();

	pc.clear();
	fs.open("data0_door2.xml", cv::FileStorage::READ);
	fs["vocabulary"] >> back;
	addPointCloud(back);
	shapeDetect();
	fs.release();
}


bool less_second(const std::pair< MiscLib::RefCountPtr< PrimitiveShape >, size_t > & m1, 
const std::pair< MiscLib::RefCountPtr< PrimitiveShape >, size_t > & m2) {
	return m1.second > m2.second;
}


void GLWidget::shapeDetect(int signForGround ){

	//fill or load point cloud from file
	//...
	//don't forget to set the bbox in pc

	RansacShapeDetector::Options ransacOptions;
	//ransacOptions.m_epsilon = .01f * pc.getScale(); // set distance threshold to .01f of bounding box width
	ransacOptions.m_epsilon = 0.01f * pc.getScale(); // set distance threshold to .01f of bounding box width
	// NOTE: Internally the distance threshold is taken as 3 * ransacOptions.m_epsilon!!!

	//ransacOptions.m_bitmapEpsilon = .02f * pc.getScale(); // set bitmap resolution to .02f of bounding box width
	ransacOptions.m_bitmapEpsilon = 0.02f * pc.getScale(); // set bitmap resolution to .02f of bounding box width
	// NOTE: This threshold is NOT multiplied internally!

	ransacOptions.m_normalThresh = 0.9f; // this is the cos of the maximal normal deviation
	ransacOptions.m_minSupport = 500; // this is the minimal numer of points required for a primitive
	//ransacOptoins.m_probability = .001f; // this is the "probability" with which a primitive is overlooked
	ransacOptions.m_probability = 0.001f;

	RansacShapeDetector detector(ransacOptions); // the detector object

	// set which primitives are to be detected by adding the respective constructors
	detector.Add(new PlanePrimitiveShapeConstructor());

	MiscLib::Vector< std::pair< MiscLib::RefCountPtr< PrimitiveShape >, size_t > > shapes; // stores the detected shapes
	size_t remaining = detector.Detect(pc, 0, pc.size(), &shapes); // run detection

	int singleShapeSize = 0;
	//m_data.clear();
	//m_count = 0;

	int maxSize = 0;
	int secondSize = 0;
	int maxflag = -1;
	int secondflag = -1;

	int texX, texY;
	int offset = pc.size();
	int shapesize = shapes.size();

	sort(shapes.begin(), shapes.end(), less_second);

	for (size_t j = 0; j < shapes.size(); j++)
	{
		m_data.resize(m_data.size() + singleShapeSize * 8);
		singleShapeSize = shapes[j].second;
		shapeRange.push_back(std::make_pair(m_count/8, m_count/8 + singleShapeSize));
		for (size_t i = offset; i > offset - singleShapeSize; i--)
		{
			add(
				QVector3D(pc[i].pos[0] / 1000.0f, 
				pc[i].pos[1] / 1000.0f, 
				pc[i].pos[2] / 1000.0f), 
				QVector3D(pc[i].normal[0], 
				pc[i].normal[1], 
				pc[i].normal[2])
				);
			cam2tex(texX, texY, pc[i].pos[2], pc[i].pos[0], pc[i].pos[1]);
			addTex(QVector2D(texX / 640.0f, texY / 480.0f));
			QVector3D colorValue = colorList.at((shapeRange.size()-1) % colorList.size());
			shapeColor->setPixel(texX, texY, qRgb(colorValue.x() * 255, colorValue.y() * 255, colorValue.z() * 255));
		}
		offset -= singleShapeSize;
	}

	MiscLib::RefCountPtr< PrimitiveShape > ptrRef = shapes.at(0).first;
	PlanePrimitiveShape * ptr = (PlanePrimitiveShape *)ptrRef.Ptr();

	Vec3fShape center(0.0f, 0.0f, 0.0f);
	for (size_t i = 0; i < pc.size(); i++)
	{
		center += pc.at(i);
	}

	center /= pc.size();

	Vec3fShape pnormal[3];
	pnormal[0] = ptr->Internal().getNormal();
	if (signForGround){
		normalGround = pnormal[0];
		return;
	}
	else{
		if (shapes.size() < 2)
		{
			if (abs(pnormal[0].dot(normalGround)) > 0.707)
			{
				Vec3fShape ax[2];
				float axis_x[2], axis_y[2];
				std::vector<float> dataXY;
				dataXY.reserve(2 * pc.size());

				for (size_t k = 0; k < pc.size(); k++)
				{
					float lamda = 0.0f;
					Vec3fShape delta = pc.at(k) - center;
					lamda = -(delta[0] * pnormal[0][0] + delta[1] * pnormal[0][1] + delta[2] * pnormal[0][2]) / (pnormal[0].length() * pnormal[0].length());
					Vec3fShape onPlane = lamda*pnormal[0] + delta;
					if (k == 0)
					{
						ax[0] = onPlane - center;
						ax[1] = pnormal[0].cross(ax[0]);
						ax[0] = pnormal[0].cross(ax[1]);
						ax[0].normalize();
						ax[1].normalize();
					}

					dataXY.push_back(onPlane.dot(ax[0]));
					dataXY.push_back(onPlane.dot(ax[1]));

				}
				
				ldp::pca2D(pc.size(), dataXY.data(), axis_x, axis_y);
				pnormal[1] = ax[0] * axis_x[0] + ax[1] * axis_x[1];
				pnormal[2] = pnormal[0].cross(pnormal[1]);
				pnormal[1] = pnormal[0].cross(pnormal[2]);

			}
			else
			{
				pnormal[1] = pnormal[0].cross(normalGround);
				pnormal[2] = pnormal[0].cross(pnormal[1]);
				pnormal[1] = pnormal[0].cross(pnormal[2]);
			}

		}
		else
		{
			normalGround.normalize();
			pnormal[0].normalize();
			if (pnormal[0].dot(normalGround) > 0.7){
				pnormal[1] = ((PlanePrimitiveShape *)shapes[1].first.Ptr())->Internal().getNormal();

				if (pnormal[0].dot(pnormal[1]) > 0.7)
				{
					Vec3fShape ax[2];
					float axis_x[2], axis_y[2];
					std::vector<float> dataXY;
					dataXY.reserve(2 * pc.size());

					for (size_t k = 0; k < pc.size(); k++)
					{
						float lamda = 0.0f;
						Vec3fShape delta = pc.at(k) - center;
						lamda = -(delta[0] * pnormal[0][0] + delta[1] * pnormal[0][1] + delta[2] * pnormal[0][2]) / (pnormal[0].length() * pnormal[0].length());
						Vec3fShape onPlane = lamda*pnormal[0] + delta;
						if (k == 0)
						{
							ax[0] = onPlane - center;
							ax[1] = pnormal[0].cross(ax[0]);
							ax[0] = pnormal[0].cross(ax[1]);
							ax[0].normalize();
							ax[1].normalize();
						}

						dataXY.push_back(onPlane.dot(ax[0]));
						dataXY.push_back(onPlane.dot(ax[1]));

					}

					ldp::pca2D(pc.size(), dataXY.data(), axis_x, axis_y);
					pnormal[1] = ax[0] * axis_x[0] + ax[1] * axis_x[1];
					pnormal[2] = pnormal[0].cross(pnormal[1]);
					pnormal[1] = pnormal[0].cross(pnormal[2]);
				}
			}
			else
				pnormal[1] = normalGround;
			pnormal[2] = pnormal[0].cross(pnormal[1]);
			pnormal[1] = pnormal[0].cross(pnormal[2]);
		}
	
	}


	for (size_t i = 0; i < 3; i++)
	{
		pnormal[i].normalize();
	}

	float maxLength[3] = { 0.0f, 0.0f, 0.0f };
	int maxFlag = 0;
	float minLength[3] = { 0.0f, 0.0f, 0.0f };
	int minFlag = 0;
	float sumLength[3] = { 0.0f, 0.0f, 0.0f };
	float len;

	for (size_t i = 0; i < pc.size(); i++)
	{
		for (size_t j = 0; j < 3; j++)
		{
			Vec3fShape d = pc.at(i).pos - center;
			len = (pc.at(i).pos - center).dot(pnormal[j]);

			if (len>maxLength[j]){
				maxLength[j] = len;
				maxFlag = i;
			}
			if (len < minLength[j]){
				minLength[j] = len;
				minFlag = i;
			}

		}
	}

	for (size_t i = 0; i < 3; i++)
	{
		maxLength[i] /= 1000.0f;
		minLength[i] /= 1000.0f;
	}
	center /= 1000.0f;	

	float pmax[3] = { maxLength[0], maxLength[1], maxLength[2] };
	float pmin[3] = { minLength[0], minLength[1], minLength[2] };

	Box nBox(center, pnormal, pmax, pmin);
	//addBox(nBox);
	boxList.push_back(nBox);
	update();
}


void GLWidget::mousePressEvent(QMouseEvent *event)
{
    m_lastPos = event->pos();
	//QMessageBox::information(0, tr("Point"), QString::number(m_lastPos.x()) + QString("_") +QString::number(m_lastPos.y()));
	/*GLdouble x, y, z;
	GLint xt, yt;
	xt = m_lastPos.x();
	yt = m_lastPos.y();
	GLint view[4];
	GLdouble mvmatrix[16], projmatrix[16];
	glGetIntegerv(GL_VIEWPORT, view);
	glGetDoublev(GL_MODELVIEW_MATRIX, mvmatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projmatrix);
	yt = height - m_lastPos.y() - 1;
	view[0] = 0;
	view[1] = 0;
	view[2] = width;
	view[3] = height;*/
	QVector3D right;
	right = QVector3D::crossProduct(at - eye, up).normalized();
	QVector3D center;
	center = eye + (at - eye).normalized() * nearPlane;
	QVector3D upp;
	upp = QVector3D::crossProduct(right,(at - eye)).normalized();
	float t = tan(M_PI / 8);

	float h = 2 * nearPlane * t;
	float w = h * (float(width) / height);
	QVector3D leftUp = center + right * (-w / 2) + upp * (h / 2);

	QVector3D point =  leftUp + (m_lastPos.x() / (float)width) * w * right + upp * (-m_lastPos.y() / (float)height) * h;

	selectRay = point;
	float f = (0.4964-eye.z()) / (point - eye).z();
	QVector3D verify = f * (point - eye) + eye;
	triangle * triangleList[6];
	length = -1.0;
	lengthFormer = -1.0;
	

	for (size_t i = 0; i < modelManager.size(); i++)
	{
		modelGuy* modelM = modelManager.at(i);
		QVector4D p0 = m_world * modelM->m_world * QVector4D(modelM->model_ptr->x_min, modelM->model_ptr->y_max, modelM->model_ptr->z_max,1.0);
		QVector4D p1 = m_world * modelM->m_world * QVector4D(modelM->model_ptr->x_max, modelM->model_ptr->y_max, modelM->model_ptr->z_max, 1.0);
		QVector4D p2 = m_world * modelM->m_world * QVector4D(modelM->model_ptr->x_max, modelM->model_ptr->y_min, modelM->model_ptr->z_max, 1.0);
		QVector4D p3 = m_world * modelM->m_world * QVector4D(modelM->model_ptr->x_min, modelM->model_ptr->y_min, modelM->model_ptr->z_max, 1.0);
		QVector4D p4 = m_world * modelM->m_world * QVector4D(modelM->model_ptr->x_min, modelM->model_ptr->y_max, modelM->model_ptr->z_min, 1.0);
		QVector4D p5 = m_world * modelM->m_world * QVector4D(modelM->model_ptr->x_max, modelM->model_ptr->y_max, modelM->model_ptr->z_min, 1.0);
		QVector4D p6 = m_world * modelM->m_world * QVector4D(modelM->model_ptr->x_max, modelM->model_ptr->y_min, modelM->model_ptr->z_min, 1.0);
		QVector4D p7 = m_world * modelM->m_world * QVector4D(modelM->model_ptr->x_min, modelM->model_ptr->y_min, modelM->model_ptr->z_min, 1.0);
		
		triangleList[0] = new triangle(p1, p0 - p1, p2 - p1);
		triangleList[1] = new triangle(p3, p0 - p3, p2 - p3);
		triangleList[2] = new triangle(p5, p4 - p5, p1 - p5);
		triangleList[3] = new triangle(p0, p1 - p0, p4 - p0);
		triangleList[4] = new triangle(p0, p4 - p0, p3 - p0);
		triangleList[5] = new triangle(p7, p3 - p7, p4 - p7);

		for (size_t j = 0; j < 6; j++)
		{
			length = rayTriangleIntersection(eye, point - eye, triangleList[j]->getVectorByNum(0), triangleList[j]->getVectorByNum(1), triangleList[j]->getVectorByNum(2));
			if (length > 0)
			{
				if (lengthFormer < 0)
				{
					lengthFormer = length;
					interIndex = i;
				}
				else if(length < lengthFormer){
					lengthFormer = length;
					interIndex = i;
				}
			}
		}
	}

	mouseMoved = false;
	return;

}

void GLWidget::mouseReleaseEvent(QMouseEvent *event){
	if (!mouseMoved)
	if (lengthFormer > 0)
	{
		currentMesh = modelManager.at(interIndex);
		emit currentMeshChanged(interIndex);
	}
}

QVector3D triangle::getVectorByNum(int index){
	switch (index)
	{
	case 0:
		return(QVector3D(v0.x(), v0.y(), v0.z()));
	case 1:
		return(QVector3D(e1.x(), e1.y(), e1.z()));
	case 2:
		return(QVector3D(e2.x(), e2.y(), e2.z()));
	default:
		break;
	}
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - m_lastPos.x();
    int dy = event->y() - m_lastPos.y();

    if (event->buttons() & Qt::LeftButton) {
		if (iTransMode == 0){
			setXRotation(/*m_xRot + */8 * dy);
			setYRotation(/*m_yRot + */8 * dx);
		}
		else{
			setYRotation(/*m_xRot + */8 * dy);
			setYRotation(/*m_xRot + */8 * dx);
		}
    } else if (event->buttons() & Qt::RightButton) {
		if (iTransMode == 0){
			setXRotation(/*m_xRot + */8 * dy);
			setZRotation(/*m_zRot + */8 * dx);
		}
		else{
			setYRotation(/*m_xRot + */8 * dy);
			setYRotation(/*m_xRot + */8 * dx);
		}
    } else if (event->buttons() & Qt::MidButton) {
		setCamXRotation(/*m_xRot + */8 * dy);
		setCamYRotation(/*m_zRot + */8 * dx);
	}
    m_lastPos = event->pos();
	mouseMoved = true;
}

void GLWidget::setTransMode(int mode){
	iTransMode = mode;
	update();
	this->setFocus();
}

void GLWidget::setTrans(float dx, float dy, float dz){
	if (currentMesh){
		float d = stepLength / stepDPI;
		if (iTransMode == 1){
			QVector4D dl = QVector4D(dx, dy, dz, 1.0);
			currentMesh->m_world.setToIdentity();
			currentMesh->m_world.rotate(currentMesh->m_xRot / 16.0f, 1, 0, 0);
			currentMesh->m_world.rotate(currentMesh->m_yRot / 16.0f, 0, 1, 0);
			currentMesh->m_world.rotate(currentMesh->m_zRot / 16.0f, 0, 0, 1);
			QVector4D dl_rotated = currentMesh->m_world *dl;

			currentMesh->m_xTrans += dl_rotated.x() * d;
			currentMesh->m_yTrans += dl_rotated.y() * d;
			currentMesh->m_zTrans += dl_rotated.z() * d;
			update();
			return;
		}
		else{
			currentMesh->m_xTrans += dx * d;
			currentMesh->m_yTrans += dy * d;
			currentMesh->m_zTrans += dz * d;
		}
	}
	else{
		m_camera.setToIdentity();
		eye += QVector3D(dx, dy, dz);
		m_camera.lookAt(eye, QVector3D(0, 0, 0), QVector3D(0, 1, 0));
		m_xRot = m_yRot = m_zRot = 0;
	}
	update();
}

GLMmodel*  GLWidget::addModel(QString name){
	bool inMem = false;
	QString pathname = QString("Meshes/") + name + QString("/") + name + QString(".obj");
	int sign = -1;
	int count = 0;

	for (size_t i = 0; i < modelList.size(); i++)
	{
		GLMmodel* guy = modelList.at(i);
		if (pathname == guy->pathname)
		{
			inMem = true;
			sign = i;
		}
	}
	modelGuy * newGuy = new modelGuy;

	if (inMem){
		newGuy->model_ptr = modelList.at(sign);
		newGuy->model_ptr->countDraw++;
		newGuy->meshName = name + QString("_") + QString::number(newGuy->model_ptr->countDraw);
		modelManager.append(newGuy);
	}
	else{
		QByteArray ba = pathname.toLatin1();
		model_ptr = glmReadOBJ(ba.data());
		if (!model_ptr)
			exit(0);

		glmUnitize(model_ptr);
		glmFacetNormals(model_ptr);
		glmVertexNormals(model_ptr, 90.0);
		modelList.append(model_ptr);

		newGuy->modelIndex = modelManager.size();
		newGuy->meshName = name;
		newGuy->model_ptr = model_ptr;
		modelManager.append(newGuy);

		int size = 0;
		for (int i = 0; i < modelList.size(); ++i) {
			size += modelList.at(i)->numtriangles * 8 * 3;
		}

		m_data.resize(size);
		glmVN(model_ptr);
		glmBox(model_ptr);

		m_logoVbo.bind();
		m_logoVbo.allocate(m_data.constData(), m_count * sizeof(GLfloat));
	}
	emit addComboItem(newGuy);
	update();
	return model_ptr;
}

bool GLWidget::updateBuffer(){
	m_data.erase(m_data.begin(), m_data.end());
	m_count = 0;
	int size = 0;
	for (int i = 0; i < modelManager.size(); ++i) {
		size += modelManager.at(i)->model_ptr->numtriangles * 8 * 3;
	}
	m_data.resize(size);
	for (int i = 0; i < modelManager.size(); ++i) {
		glmVN(modelManager.at(i)->model_ptr);
	}
	return true;
}

GLMmodel*  GLWidget::removeModel(QString name){
	for (int j = 0; j < modelManager.size(); ++j) {
		GLMmodel* model = modelManager.at(j)->model_ptr;
		QDir dir(model->pathname);
		QString dirName = dir.dirName();
		dirName = dirName.section('.', 0, 0);
		if (name == dirName){
			//model->toDraw = false;
			delete modelManager.at(j);
			modelManager.remove(j);
			//delete(model);
		}
	}
	updateBuffer();
	emit refreshComboItem();
	update();
	return NULL;
}

void GLWidget::removeCurrentMesh(){
	if (!currentMesh)
		return;
	modelManager.remove(currentMesh->modelIndex);
	for (int i = 0; i < modelManager.size(); i++)
	{
		modelManager.at(i)->modelIndex = i;
	}
	emit refreshComboItem();
	update();
}

void GLWidget::add(const QVector3D &v, const QVector3D &n)
{
	GLfloat *p = m_data.data() + m_count;
	*p++ = v.x();
	*p++ = v.y();
	*p++ = v.z();
	*p++ = n.x();
	*p++ = n.y();
	*p++ = n.z();
	m_count += 6;
}

void GLWidget::addV(const QVector3D &v)
{
	GLfloat *p = m_data.data() + m_count;
	*p++ = v.x();
	*p++ = v.y();
	*p++ = v.z();
	m_count += 3;
}

void GLWidget::addTex(const QVector2D &t)
{
	GLfloat *p = m_data.data() + m_count;
	*p++ = t.x();
	*p++ = t.y();
	m_count += 2;
}

GLvoid GLWidget::glmBox(GLMmodel* model){
	for (size_t i = 1; i <= model->numvertices; i++)
	{
		if (model->vertices[3 * i] > model->x_max)
			model->x_max = model->vertices[3 * i];
		if (model->vertices[3 * i] < model->x_min)
			model->x_min = model->vertices[3 * i];

		if (model->vertices[3 * i + 1] > model->y_max)
			model->y_max = model->vertices[3 * i + 1];
		if (model->vertices[3 * i + 1] < model->y_min)
			model->y_min = model->vertices[3 * i + 1];

		if (model->vertices[3 * i + 2] > model->z_max)
			model->z_max = model->vertices[3 * i + 2];
		if (model->vertices[3 * i + 2] < model->z_min)
			model->z_min = model->vertices[3 * i + 2];
	}
}

GLvoid GLWidget::glmVN(GLMmodel* model)
{
	GLMgroup* group;
	group = model->groups;
	GLMtriangle* triangle;
	GLuint i;
	while (group) {
		group->arrayBegin = m_count / 8;
		for (i = 0; i < group->numtriangles; i++) {
			triangle = &T(group->triangles[i]);
			for (int j = 0; j < 3; j++)
			{
				QVector3D v(model->vertices[3 * triangle->vindices[j]], 
					model->vertices[3 * triangle->vindices[j] + 1], 
					model->vertices[3 * triangle->vindices[j] + 2]);
				QVector3D n(model->normals[3 * triangle->nindices[j]], 
					model->normals[3 * triangle->nindices[j] + 1], 
					model->normals[3 * triangle->nindices[j] + 2]);
				add(v, n);
				if ( model->nummaterials>0 && model->materials[group->material].hasTex){
					QVector2D t(model->texcoords[2 * triangle->tindices[j]], model->texcoords[2 * triangle->tindices[j] + 1]);
					addTex(t);
				}
				else
				{
					QVector2D t(0, 0);
					addTex(t);
				}
			}
		}
		group = group->next;
	}
}

void GLWidget::addMesh(QString name){
	addModel(name);
}

void GLWidget::addMeshRightClick(){
	addModel(fileNameRightClick);
}

void GLWidget::addMeshDoubleClick(){
	addModel(fileNameRightClick);
}

void GLWidget::removeMesh(QString name){
	removeModel(name);
}

void GLWidget::wheelEvent(QWheelEvent * event){

	m_camera.setToIdentity();
	QVector3D dv = at - eye;
	int x = event->delta();
	if (event->delta() > 0 && event->delta() / 100.0 > (eye - at).length())
		eye = (eye - at).normalized() + at;
	else
		eye += event->delta() / 200.0 * dv.normalized();
	m_camera.lookAt(eye, QVector3D(0, 0, 0), QVector3D(0, 1, 0));
	update();

}

void GLWidget::keyPressEvent(QKeyEvent * event){
	
	switch (event->key()){
	case 'W':
		setTrans(0, 0, -1);
		break;
	case 'S':
		setTrans(0, 0, 1);
		break;
	case 'A':
		setTrans(-1, 0, 0);
		break;
	case 'D':
		setTrans(1, 0, 0);
		break;
	case 'Q':
		setTrans(0, 1, 0);
		break;
	case 'E':
		setTrans(0, -1, 0);
		break;
	}
}

void GLWidget::keyReleaseEvent(QKeyEvent * event){

}