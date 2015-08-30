#include "glwidget.h"
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QCoreApplication>
#include <QDir>
#include <QMessageBox>
#include <QVector4D>
#include <math.h>

#define T(x) (model->triangles[(x)])
std::string filePath = std::string("Resources/data4/");
std::string fileHead = std::string("data4");

VEC3D vec3fShape2VEC3D(Vec3fShape v){
	return VEC3D(v[0], v[1], v[2]);
}

Vec3fShape VEC3D2vec3fShape (VEC3D v){
	return Vec3fShape(v.X(), v.Y(), v.Z());
}

void cam2tex(int& x, int& y, float depth, float camx, float camy){
	x = (camx * 571.401f) / depth + 319.5f;
	y = (camy * 571.401f) / depth + 239.5f;
}

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      m_xRot(0),
      m_yRot(0),
      m_zRot(0),
      m_program(0),
	  m_count(0),
	  coord(NoCo),
	  iTransMode(0),
	  stepDPI(50),
	  stepLength(50),
	  nearPlane(0.01),
	  currentMesh(NULL),
	  rgbMap(NULL),
	  drawShapeWithColor(0),
	  grabResult(480,640,CV_8UC1),
	  rawPointCount(0),
	  triangleBegin(0),
	  triangleEnd(0),
	  currentJoint(NULL),
	  currentBox(-1),
	  bnormalGroundSet(false),
	  currentSelectBox(-1),
	  bEditLength(false),
	  bDrawJoint(true),
	  eyeAtMode(0),
	  boxCenter(0,0,0),
	  bDrawSelected(true)
{
    //m_core = QCoreApplication::arguments().contains(QStringLiteral("--coreprofile"));
    // --transparent causes the clear color to be transparent. Therefore, on systems that
    // support it, the widget will become transparent apart from the logo.
    m_transparent = QCoreApplication::arguments().contains(QStringLiteral("--transparent"));
    if (m_transparent)
        setAttribute(Qt::WA_TranslucentBackground);

	m_data.reserve(640 * 480 *24);

	colorList.push_back(QVector4D(255 / 255.0f, 255 / 255.0f, 0 / 255.0f,1.0));
	colorList.push_back(QVector4D(255 / 255.0f, 255 / 255.0f, 255 / 255.0f,1.0));
	colorList.push_back(QVector4D(0 / 255.0f, 255 / 255.0f, 255 / 255.0f,1.0));
	colorList.push_back(QVector4D(255 / 255.0f, 0 / 255.0f, 255 / 255.0f,1.0));
	colorList.push_back(QVector4D(0.0, 0.9f, 0.0f, 1.0));
	colorList.push_back(QVector4D(0.0, 0.0f, 0.9f, 1.0));
	colorList.push_back(QVector4D(0.729, 0.004f, 1.0f, 1.0));
	colorList.push_back(QVector4D(0.639, 0.376f, 0.594f, 1.0));
	colorList.push_back(QVector4D(1.0, 0.310f, 0.322f, 1.0));
	setThisBoxCenter = false;
	sendCenter.setToIdentity();
	dx = dy = dz = 0.0f;
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

void GLWidget::nocoClicked(){
	coord = NoCo;
	update();
}

void GLWidget::XYClicked(){
	coord = XY;
	update();
}
void GLWidget::YZClicked(){
	coord = YZ;
	update();
}
void GLWidget::XZClicked(){
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
    "attribute vec3 vertex;\n"
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
	"   gl_Position = projMatrix * mvMatrix * vec4(vertex,1.0);\n"
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
	"uniform vec4 assignedColor;\n"
	"uniform float mShininess;\n"
	"uniform sampler2D s_baseMap;\n"
	"uniform highp vec3 lightPos;\n"
	"uniform bool hasTex;\n"
	"uniform bool fixedPipeline;\n"
	"uniform bool assignedMode;\n"
	"void main() {\n"
	"	if(fixedPipeline){\n"
	"		gl_FragColor = fixedColor;\n"
	"		return ;\n"
	"	}\n"
	"	vec4 baseColor = texture2D(s_baseMap, v_texCoord);\n"
	"	if(assignedMode)\n"
	"		baseColor = assignedColor;\n"
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
	//"		gl_FragColor = vec4(gl_Normal, 1.0);\n"
    "}\n";

void GLWidget::laodRawPC(){
	FILE *stream;
	if ((stream = fopen("Resources/1_initial.box", "rb")) == NULL) /* open file TEST.$$$ */
	{
		fprintf(stderr, "Cannot open output file.\n");
		return;
	}
	int bn;
	fread(&bn, sizeof(int), 1, stream);

	double center[3];
	double scale[3];
	double xd[3];
	double yd[3];
	double zd[3];

	for (size_t i = 0; i < bn; i++)
	{
		fread(&center, 3 * sizeof(double), 1, stream);
		fread(&scale, 3 * sizeof(double), 1, stream);
		fread(&xd, 3 * sizeof(double), 1, stream);
		fread(&yd, 3 * sizeof(double), 1, stream);
		fread(&zd, 3 * sizeof(double), 1, stream);
		Box temp(0);
		Vec3fShape pc(center[0], center[1], center[2]);
		Vec3fShape xr(xd[0], xd[1], xd[2]);
		Vec3fShape yr(yd[0], yd[1], yd[2]);
		Vec3fShape zr(zd[0], zd[1], zd[2]);

		temp.vertex[0] = 1 * scale[0] * xr + -1 * scale[1] * yr + 1 * scale[2] * zr + pc;
		temp.vertex[1] = 1 * scale[0] * xr + 1 * scale[1] * yr + 1 * scale[2] * zr + pc;
		temp.vertex[2] = 1 * scale[0] * xr + 1 * scale[1] * yr + -1 * scale[2] * zr + pc;
		temp.vertex[3] = 1 * scale[0] * xr + -1 * scale[1] * yr + -1 * scale[2] * zr + pc;
		temp.vertex[4] = -1 * scale[0] * xr + -1 * scale[1] * yr + 1 * scale[2] * zr + pc;
		temp.vertex[5] = -1 * scale[0] * xr + 1 * scale[1] * yr + 1 * scale[2] * zr + pc;
		temp.vertex[6] = -1 * scale[0] * xr + 1 * scale[1] * yr + -1 * scale[2] * zr + pc;
		temp.vertex[7] = -1 * scale[0] * xr + -1 * scale[1] * yr + -1 * scale[2] * zr + pc;

		temp.m_transform.setToIdentity();
		boxList.push_back(temp);
	}

	fclose(stream);

	if ((stream = fopen("Resources/1.pts", "rb")) == NULL)
	{
		fprintf(stderr, "Cannot open output file.\n");
		return;
	}
	double non;
	for (size_t i = 0; i < 3; i++)
		fread(&non, sizeof(double), 1, stream);

	int vn, nn, cn;
	fread(&vn, sizeof(int), 1, stream);
	double pos[3];

	rawPCBegin = m_count / 8;
	m_data.resize(m_count + vn * 3 * 8);
	for (size_t i = 0; i < vn; i++)
	{
		fVertex hah;
		fread(&hah, sizeof(fVertex), 1, stream);
		vecPoints.push_back(hah);
		add(
			QVector3D(hah.items[0], hah.items[1], hah.items[2]),
			QVector3D(1, 1, 1));
		addTex(QVector2D(0.5, 0.5));
	}
	rawPCEnd = m_count / 8;
	bRawPC = true;
	boxUpdate(boxList);
	update();
}

void GLWidget::updateRawPC(double dx, double dy, double dz){
	//fVertex * ptrVertex = (fVertex*)m_data.data();
	//ptrVertex += rawPCBegin;
	for (size_t i = rawPCBegin; i < rawPCEnd; i++)
	{
		m_data[i * 8] = vecPoints.at(i - rawPCBegin).items[0] + dx;
		m_data[i * 8 +1] = vecPoints.at(i - rawPCBegin).items[1] + dy;
		m_data[i * 8 +2] = vecPoints.at(i - rawPCBegin).items[2] + dz;
	}
	update();
}

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
	//glClearColor(199 / 255.0f, 237 / 255.0f, 204/255.0f, m_transparent ? 0 : 1);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	//glClearColor(0.3, 0.3, 0.3, 1);
	
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
	m_assignedMode = m_program->uniformLocation("assignedMode");
	m_assignedColor = m_program->uniformLocation("assignedColor");	
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
	at = QVector3D(0, 0, 0);
	m_camera.lookAt(eye, at, up);

    // Light position is fixed.
    m_program->setUniformValue(m_lightPosLoc, QVector3D(0, 15, 0));
	m_program->setUniformValue(m_viewPos, eye);

	m_program->setUniformValue("s_baseMap", 0);

	if (rgbMap){
		texture = new QOpenGLTexture(*rgbMap);
	}
	shapeColor = new QImage(640,480,QImage::Format_RGB888);

	arrowModel = glmReadOBJ("Resources/arrow2.obj");
	glmUnitize(arrowModel);
	glmFacetNormals(arrowModel);
	glmVertexNormals(arrowModel, 90.0);

	arrowStraightModel = glmReadOBJ("Resources/arrowStraight2.obj");
	glmUnitize(arrowStraightModel);
	glmFacetNormals(arrowStraightModel);
	glmVertexNormals(arrowStraightModel, 90.0);

	ballModel = glmReadOBJ("Resources/ball.obj");
	glmUnitize(ballModel);
	glmFacetNormals(ballModel);
	glmVertexNormals(ballModel, 90.0);


	int size = m_count;
	size += arrowModel->numtriangles * 8 * 3;
	size += arrowStraightModel->numtriangles * 8 * 3;
	size += ballModel->numtriangles * 8 * 3;
	m_data.resize(size);
	glmVN(arrowModel);
	glmVN(arrowStraightModel);
	glmVN(ballModel);
	m_program->release();
	//laodRawPC();
	

}

void GLWidget::drawRawPC(){
	bRawPC = true;
	rawPCBegin = m_count / 8;
	for (size_t i = 0; i < vertex.size(); i++)
	{
		if (vertex[i].length() < 0.001)
			continue;
		m_data.resize(m_count + 8);
		add(
			QVector3D(vertex[i][0] / 1000.0f,
			vertex[i][1] / 1000.0f,
			vertex[i][2] / 1000.0f),

			QVector3D(normal[i][0],
			normal[i][1],
			normal[i][2])
			);
		int texX, texY;
		cam2tex(texX, texY, vertex[i][2], vertex[i][0], vertex[i][1]);
		addTex(QVector2D(texX / 640.0f, texY / 480.0f));
	}
	rawPCEnd = m_count / 8;
	update();
}

void GLWidget::save(){
	FILE *stream;
	if ((stream = fopen((filePath + std::string("centerMatrix.data")).c_str(), "wb")) == NULL) /* open file TEST.$$$ */
	{
		fprintf(stderr, "Cannot open output file.\n");
		return;
	}
	fwrite(&sendCenter, sizeof(QMatrix4x4), 1, stream);
	fclose(stream);
	int num = m_data.size();
	//***********save rendering buffer data
	
	if ((stream = fopen((filePath + std::string("m_data.data")).c_str(), "wb")) == NULL) /* open file TEST.$$$ */
	{
		fprintf(stderr, "Cannot open output file.\n");
		return;
	}

	fwrite(&num, sizeof(num), 1, stream);
	fwrite(m_data.data(), sizeof(float) * num, 1, stream);
	fclose(stream); 

	//************save box list
	num = boxList.size();
	if ((stream = fopen((filePath + std::string("boxList.data")).c_str(), "wb")) == NULL) /* open file TEST.$$$ */
	{
		fprintf(stderr, "Cannot open output file.\n");
		return;
	}
	fwrite(&num, sizeof(num), 1, stream);
	for (size_t i = 0; i <boxList.size(); i++)
	{
		fwrite(&boxList.at(i), sizeof(Box), 1, stream);
		int shape_num = boxList.at(i).shapeRange->size();
		fwrite(&shape_num, sizeof(shape_num), 1, stream);
		int begin, end;
		for (size_t j = 0; j < boxList.at(i).shapeRange->size(); j++)
		{
			begin = boxList.at(i).shapeRange->at(j).first;
			end = boxList.at(i).shapeRange->at(j).second;
			fwrite(&begin, sizeof(begin), 1, stream);
			fwrite(&end, sizeof(end), 1, stream);

		}
	}
	fclose(stream);

	//**************save the joints
	num = jointList.size();
	if ((stream = fopen((filePath + std::string("jointList.data")).c_str(), "wb")) == NULL) /* open file TEST.$$$ */
	{
		fprintf(stderr, "Cannot open output file.\n");
		return;
	}
	fwrite(&num, sizeof(num), 1, stream);
	for (size_t i = 0; i <jointList.size(); i++)
	{
		fwrite(jointList.at(i), sizeof(BoxJoint), 1, stream);
	}
	fclose(stream);
}

void GLWidget::read(){

	FILE *stream;

	if ((stream = fopen((filePath + std::string("m_data.data")).c_str(), "rb")) == NULL) /* open file TEST.$$$ */
	{
		fprintf(stderr, "Cannot open output file.\n");
		return;
	}
	int size;
	fread(&size, sizeof(int), 1, stream);
	float * read_m_data = (float *)malloc(size * sizeof(float));
	fread(read_m_data, size * sizeof(float), 1, stream);
	fclose(stream);

	m_data.clear();
	m_count = 0;

	m_data.reserve(size);
	for (size_t i = 0; i < size; i++)
	{
		m_data.push_back(read_m_data[i]);
	}
	m_count = size;

	if ((stream = fopen((filePath + std::string("boxList.data")).c_str(), "rb")) == NULL) /* open file TEST.$$$ */
	{
		fprintf(stderr, "Cannot open output file.\n");
		return;
	}
	fread(&size, sizeof(int), 1, stream);
	Box * read_box = (Box *)malloc(sizeof(Box));
	boxList.clear();

	for (size_t i = 0; i < size; i++)
	{
		fread(read_box, sizeof(Box), 1, stream);
		Box temp(*read_box);
		boxList.push_back(temp);
		boxList.at(i).shapeRange = new std::vector<std::pair<int, int>>;

		int shape_num;
		fread(&shape_num, sizeof(shape_num), 1, stream);
		int begin, end;

		for (size_t j = 0; j < shape_num; j++)
		{
			fread(&begin, sizeof(begin), 1, stream);
			fread(&end, sizeof(end), 1, stream);
			boxList.at(i).shapeRange->push_back(std::make_pair(begin, end));
		}
	}
	fclose(stream);

	if ((stream = fopen((filePath + std::string("jointList.data")).c_str(), "rb")) == NULL) /* open file TEST.$$$ */
	{
		fprintf(stderr, "Cannot open output file.\n");
		return;
	}

	fread(&size, sizeof(int), 1, stream);
	for (size_t i = 0; i < jointList.size(); i++)
	{
		jointList.at(i)->~BoxJoint();
	}
	jointList.clear();

	BoxJoint * read_joint = (BoxJoint *)malloc(sizeof(BoxJoint));
	BoxJoint * jointTemp;
	BoxJoint * jointTarget;
	for (size_t i = 0; i < size; i++)
	{
		fread(read_joint, sizeof(BoxJoint), 1, stream);
		jointTemp = new BoxJoint(*read_joint);
		if (jointTemp->getType() == BoxJoint::HINGE)
		{
			jointTarget = new BoxHingeJoint(boxList.at(jointTemp->iParent), boxList.at(jointTemp->iChild));
		}
		else if (jointTemp->getType() == BoxJoint::SLIDER){
			jointTarget = new BoxSliderJoint(boxList.at(jointTemp->iParent), boxList.at(jointTemp->iChild));
		}
		jointTarget->setPivotPoint(jointTemp->getPivotPoint(0), jointTemp->getPivotPoint(1));
		jointTarget->setBoxIndex(jointTemp->iParent, jointTemp->iChild);
		jointTarget->setRange(jointTemp->valueRange[0], jointTemp->valueRange[1]);
		//jointTemp->setBox(boxList.at(jointTemp->iParent), boxList.at(jointTemp->iChild));
		jointList.push_back(jointTarget);
	}
	fclose(stream);
	
	if ((stream = fopen((filePath + std::string("centerMatrix.data")).c_str(), "rb")) == NULL) /* open file TEST.$$$ */
	{
		fprintf(stderr, "Cannot open output file.\n");
		return;
	}
	QMatrix4x4 * newMat = new QMatrix4x4();
	fread(newMat, sizeof(QMatrix4x4), 1, stream);
	sendCenter = *newMat;
	fclose(stream);

	update();
	emit jointUpdate(jointList);
	emit boxUpdate(boxList);
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

void drawcoordinate(QOpenGLShaderProgram *m_program, int coord, QMatrix4x4 m_world, int m_fixedColor){
	float cc[4] = { 0, 0, 0, 0 };

	QVector4D normal(0.5f, 0.5f, 0.5f, 1.0f);
	m_program->setUniformValue(m_fixedColor, normal);
	
	switch (coord){
	case NoCo:break;
	case XY:{		

		glPushMatrix();
		glBegin(GL_LINES);
		for (float x = -Mc; x <= Mc; x += 0.1){
			if (abs(x - 0.0f) < 0.000001)
				continue;
			glVertex3f(x, -Mc, 0); glVertex3f(x, Mc, 0);
		}
		for (float y = -Mc; y <= Mc + 0.05; y += 0.1){
			if (abs(y - 0.0f) < 0.000001)
				continue;
			glVertex3f(-Mc, y, 0); glVertex3f(Mc, y, 0);
		}
		glEnd();
		glPopMatrix();

		m_program->setUniformValue(m_fixedColor, QVector4D(0.0f, 1.0f, 0.0f, 1.0f));
		glPushMatrix();
		glBegin(GL_LINES);
		glVertex3f(0, -Mc, 0); glVertex3f(0, Mc, 0);
		glEnd();
		glPopMatrix();

		m_program->setUniformValue(m_fixedColor, QVector4D(1.0f, 0.0f, 0.0f, 1.0f));
		glPushMatrix();
		glBegin(GL_LINES);
		glVertex3f(-Mc, 0, 0); glVertex3f(Mc, 0, 0);
		glEnd();
		glPopMatrix();
		break;
	}
	case YZ:{
		
		//glMaterialfv(GL_FRONT, GL_DIFFUSE, cc);
		glPushMatrix();
		glBegin(GL_LINES);
		for (float y = -Mc; y <= Mc + 0.05; y += 0.1){
			if (abs(y - 0.0f) < 0.000001)
				continue;
			glVertex3f(0, y, -Mc); glVertex3f(0, y, Mc);
		}
		for (float z = -Mc; z <= Mc + 0.05; z += 0.1){
			if (abs(z - 0.0f) < 0.000001)
				continue;
			glVertex3f(0, -Mc, z); glVertex3f(0, Mc, z);
		}
		glEnd();
		glColor3d(1, 1, 1);
		glPopMatrix();

		m_program->setUniformValue(m_fixedColor, QVector4D(0.0f, 1.0f, 0.0f, 1.0f));
		glPushMatrix();
		glBegin(GL_LINES);
		glVertex3f(0, -Mc, 0); glVertex3f(0, Mc, 0);
		glEnd();
		glPopMatrix();

		m_program->setUniformValue(m_fixedColor, QVector4D(0.0f, 0.0f, 1.0f, 1.0f));
		glPushMatrix();
		glBegin(GL_LINES);
		glVertex3f(0, 0, -Mc); glVertex3f(0, 0, Mc);
		glEnd();
		glPopMatrix();
		break;
	}
	case XZ:{
		//glMaterialfv(GL_FRONT, GL_DIFFUSE, cc);
		glPushMatrix();
		glBegin(GL_LINES);
		for (float z = -Mc; z <= Mc + 0.05; z += 0.1){
			if (abs(z - 0.0f) < 0.000001)
				continue;
			glVertex3f(-Mc, 0, z); glVertex3f(Mc, 0, z);
		}
		for (float x = -Mc; x <= Mc + 0.05; x += 0.1){
			if (abs(x - 0.0f) < 0.000001)
				continue;
			glVertex3f(x, 0, -Mc); glVertex3f(x, 0, Mc);
		}
		glEnd();
		glColor3d(1, 1, 1);
		glPopMatrix();

		m_program->setUniformValue(m_fixedColor, QVector4D(0.0f, 0.0f, 1.0f, 1.0f));
		glPushMatrix();
		glBegin(GL_LINES);
		glVertex3f(0, 0, -Mc); glVertex3f(0, 0, Mc);
		glEnd();
		glPopMatrix();

		m_program->setUniformValue(m_fixedColor, QVector4D(1.0f, 0.0f, 0.0f, 1.0f));
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
		m_program->setUniformValue(m_mvMatrixLoc, m_camera * m_world * boxList.at(i).m_transform);
		glLineWidth(3);
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

void GLWidget::drawPlane(Vec3fShape v1, Vec3fShape v2, Vec3fShape v3, Vec3fShape v4){
	
	glPushMatrix();

	glBegin(GL_TRIANGLES);
	glVertex3f(v1[0], v1[1], v1[2]);
	glVertex3f(v2[0], v2[1], v2[2]);
	glVertex3f(v3[0], v3[1], v3[2]);
	glEnd();

	glBegin(GL_TRIANGLES);	
	glVertex3f(v3[0], v3[1], v3[2]);
	glVertex3f(v4[0], v4[1], v4[2]);
	glVertex3f(v1[0], v1[1], v1[2]);
	glEnd();

	glPopMatrix();

}

void GLWidget::DrawArrow(float x0, float y0, float z0, float x1, float y1, float z1, QMatrix4x4 box){

	Vec3fShape dir(x1-x0, y1-y0, z1-z0);
	Vec3fShape axis = Vec3fShape(0, 0, 1).cross(dir);
	dir.normalize();
	double alpha = acos(dir.dot(Vec3fShape(0, 0, 1)));

	QMatrix4x4 trans;
	trans.translate(0,0,-1.25);

	QMatrix4x4 rotation;
	rotation.setToIdentity();
	rotation.scale(0.15);
	rotation.rotate(alpha *180.0f / 3.1415926f, axis[0], axis[1], axis[2]);

	QMatrix4x4 pos;
	pos.setToIdentity();
	pos.translate(x0,y0,z0);
		
	m_program->setUniformValue(m_mvMatrixLoc, m_camera *m_world *box* pos * rotation * trans);
	drawModel(arrowStraightModel);
	
}

void GLWidget::DrawTorus(float x0, float y0, float z0, float x1, float y1, float z1){

	Vec3fShape dir(x1 - x0, y1 - y0, z1 - z0);
	Vec3fShape axis = Vec3fShape(0, 0, 1).cross(dir);
	dir.normalize();
	double alpha = acos(dir.dot(Vec3fShape(0, 0, 1)));

	QMatrix4x4 r2;
	r2.setToIdentity();
	r2.rotate(180.0, 0, 0, 1);
	QMatrix4x4 rotation;
	rotation.setToIdentity();
	rotation.scale(0.15);
	rotation.rotate(alpha *180.0f / 3.1415926f, axis[0], axis[1], axis[2]);

	Vec3fShape temp1, temp2;
	if (z0 >z1)
	{
		temp1 = Vec3fShape(x0,y0,z0);
		temp2 = Vec3fShape(x1, y1, z1);
	}
	else{
		temp2 = Vec3fShape(x0, y0, z0);
		temp1 = Vec3fShape(x1, y1, z1);
	}
	Vec3fShape offset = temp1 - temp2;
	temp1 = offset * 1.28;
	temp2 = temp2 + temp1;
	QMatrix4x4 pos;
	pos.setToIdentity();
	//pos.translate((x0 + x1) / 2.0f, (y0 + y1) / 2.0f, (z0 + z1) / 2.0f);
	pos.translate(temp2[0], temp2[1], temp2[2]);

	m_program->setUniformValue(m_mvMatrixLoc, m_camera *m_world * pos * rotation * r2);
	drawModel(arrowModel);

}

void GLWidget::DrawBall(float x, float y, float z, float r, QMatrix4x4 boxTransform){
	
	QMatrix4x4 scale;
	scale.scale(r);

	QMatrix4x4 pos;
	pos.setToIdentity();
	pos.translate(x, y, z);

	m_program->setUniformValue(m_mvMatrixLoc, m_camera *m_world * boxTransform * pos* scale);
	drawModel(ballModel);

}

void GLWidget::DrawJoints(){
	m_program->setUniformValue(m_lightPosLoc, QVector3D(0, 15, 0));
	m_program->setUniformValue(m_viewPos, eye);
	m_program->setUniformValue(m_hasTex, true);
	m_program->setUniformValue(m_mvMatrixLoc, m_camera * m_world);
	m_program->setUniformValue(m_fixedPipeline, false);
	m_program->setUniformValue(m_assignedMode, true);
	m_program->setUniformValue(m_normalMatrixLoc, (m_world).normalMatrix());
	m_program->setUniformValue(matAmbientLoc, QVector3D(0.5, 0.5, 0.5));
	m_program->setUniformValue(matDiffuseLoc, QVector3D(0.6, 0.6, 0.6));
	m_program->setUniformValue(matSpecularLoc, QVector3D(0.0, 0.0, 0.0));
	m_program->setUniformValue(matShineLoc, (GLfloat)65.0);

	for (size_t i = 0; i < jointList.size(); i++)
	{
		//m_program->setUniformValue(m_assignedColor, colorList.at(i));
		Vec3fShape temp0 = jointList.at(i)->getPivotPoint(0);
		Vec3fShape temp1 = jointList.at(i)->getPivotPoint(1);
		if (jointList.at(i)->getType() == BoxJoint::HINGE)
			DrawTorus(temp0[0], temp0[1], temp0[2], temp1[0], temp1[1], temp1[2]);
		else{
			Vec3fShape centerPlane[6];
			Vec3fShape line[3];

			centerPlane[0] = (jointList.at(i)->getChild().vertex[0] + jointList.at(i)->getChild().vertex[2] +
				jointList.at(i)->getChild().vertex[1] + jointList.at(i)->getChild().vertex[3]) / 4;
			centerPlane[1] = (jointList.at(i)->getChild().vertex[4] + jointList.at(i)->getChild().vertex[5] +
				jointList.at(i)->getChild().vertex[6] + jointList.at(i)->getChild().vertex[7]) / 4;

			centerPlane[2] = (jointList.at(i)->getChild().vertex[0] + jointList.at(i)->getChild().vertex[3] +
				jointList.at(i)->getChild().vertex[4] + jointList.at(i)->getChild().vertex[7]) / 4;
			centerPlane[3] = (jointList.at(i)->getChild().vertex[1] + jointList.at(i)->getChild().vertex[2] +
				jointList.at(i)->getChild().vertex[5] + jointList.at(i)->getChild().vertex[6]) / 4;

			centerPlane[4] = (jointList.at(i)->getChild().vertex[2] + jointList.at(i)->getChild().vertex[3] +
				jointList.at(i)->getChild().vertex[6] + jointList.at(i)->getChild().vertex[7]) / 4;
			centerPlane[5] = (jointList.at(i)->getChild().vertex[0] + jointList.at(i)->getChild().vertex[1] +
				jointList.at(i)->getChild().vertex[4] + jointList.at(i)->getChild().vertex[5]) / 4;
			
			line[0] = centerPlane[1] - centerPlane[0];
			line[1] = centerPlane[3] - centerPlane[2];
			line[2] = centerPlane[5] - centerPlane[4];

			for (size_t k = 0; k < 3; k++)
			{
				line[k].normalize();
				Vec3fShape pc = temp1 - temp0;
				pc.normalize();
				if (abs(abs(line[k].dot(pc)) - 1) < 0.001 )
				{
					temp0 = centerPlane[2 * k];
					temp1 = centerPlane[2 * k + 1];
				}
			}
			if (temp0[2] < temp1[2])
			{
				DrawArrow(temp0[0], temp0[1], temp0[2], temp1[0], temp1[1], temp1[2], jointList.at(i)->getChild().m_transform);
			}
			else
			{
				DrawArrow(temp1[0], temp1[1], temp1[2], temp0[0], temp0[1], temp0[2], jointList.at(i)->getChild().m_transform);
			}
			
		}
	}
}

void GLWidget::drawJoint(){
	bDrawJoint = !bDrawJoint;
}

void GLWidget::drawSelected(){
	bDrawSelected = !bDrawSelected;
}

QVector4D toTranparent(QVector4D clr){
	return QVector4D(clr.x(), clr.y(), clr.z(), 0.37);
}

void GLWidget::paintGL()
{

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	texture->setMagnificationFilter(QOpenGLTexture::Linear);

	if (!drawShapeWithColor){
		if (rgbMap)
			texture->setData(*rgbMap);
	}
	else if (shapeColor)
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

	//*************************** draw the coordinate*********************************************************
	glEnable(GL_BLEND);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);  // Antialias the lines
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glLineWidth(1);
	drawcoordinate(m_program, coord, m_world, m_fixedColor);
	glDisable(GL_BLEND);
	if (setThisBoxCenter)
	{
		sendCenter.setToIdentity();
		sendCenter.translate(-boxCenter[0], -boxCenter[1], -boxCenter[2]);
		setThisBoxCenter = false;
	}
	
	m_world.rotate(m_xRot / 16.0f, 1, 0, 0);
	m_world.rotate(m_yRot / 16.0f, 0, 1, 0);
	m_world.rotate(m_zRot / 16.0f, 0, 0, 1);
	m_world.rotate(180, 0, 0, 1);
	m_world.rotate(180, 0, 1, 0);
	m_world = m_world * sendCenter;

	//*************************** draw the joints ***********************************************************
	if (bDrawJoint)
		DrawJoints();

	//*************************** draw the selected plane and points ****************************************
	m_program->setUniformValue(m_hasTex, true);
	m_program->setUniformValue(m_mvMatrixLoc, m_camera * m_world);
	m_program->setUniformValue(m_fixedPipeline, false);
	m_program->setUniformValue(m_assignedMode, true);
	m_program->setUniformValue(m_lightPosLoc, QVector3D(0, 15, 0));
	m_program->setUniformValue(m_viewPos, eye);
	m_program->setUniformValue(m_hasTex, true);
	m_program->setUniformValue(m_mvMatrixLoc, m_camera * m_world);
	m_program->setUniformValue(m_fixedPipeline, false);
	m_program->setUniformValue(m_assignedMode, true);
	m_program->setUniformValue(m_normalMatrixLoc, (m_world).normalMatrix());
	m_program->setUniformValue(matAmbientLoc, QVector3D(0.5, 0.5, 0.5));
	m_program->setUniformValue(matDiffuseLoc, QVector3D(0.6, 0.6, 0.6));
	m_program->setUniformValue(matSpecularLoc, QVector3D(0.0, 0.0, 0.0));
	m_program->setUniformValue(matShineLoc, (GLfloat)65.0);
	if (bDrawSelected)
	for (size_t i = 0; i < boxList.size(); i++){
		if (i != indexParentBox && i != indexChildBox)
			continue;
		for (size_t k = 0; k < 2; k++){
			if (boxList.at(i).selectedPointIndex[k] != -1){
				Vec3fShape p = boxList.at(i).vertex[boxList.at(i).selectedPointIndex[k]];
				DrawBall(p[0], p[1], p[2], 0.02, boxList.at(i).m_transform);
			}
		}
		
		if (boxList.at(i).selectedPlaneIndex != -1)
		{
			m_program->setUniformValue(m_assignedColor, QVector4D(colorList.at((i+4) % colorList.size())));
			m_program->setUniformValue(m_mvMatrixLoc, m_camera * m_world * boxList.at(i).m_transform);
			drawPlane(boxList.at(i).getPlane(0), boxList.at(i).getPlane(1), boxList.at(i).getPlane(2), boxList.at(i).getPlane(3));
		}	
	}

	//*************************** draw the  points cloud ****************************************************
	//m_program->setUniformValue(m_assignedMode, false);
	if (!drawShapeWithColor){
		m_program->setUniformValue(m_assignedMode, false);
	}
	else
	{
		m_program->setUniformValue(m_fixedPipeline, false);
		m_program->setUniformValue(m_assignedMode, true);
	}

	//if (false)
	for (size_t i = 0; i < boxList.size(); i++)
	for (size_t j = 0; j < boxList.at(i).shapeRange->size(); j++){
		m_program->setUniformValue(m_assignedColor, colorList.at((j - 1 + i) % colorList.size()));
		drawShape(boxList.at(i).shapeRange->at(j).first, boxList.at(i).shapeRange->at(j).second, boxList.at(i).m_transform);
	}

	if (bRawPC)
	{
		QMatrix4x4 im;
		im.setToIdentity();
		m_program->setUniformValue(m_assignedColor, colorList.at(0));
		drawShape(rawPCBegin,rawPCEnd,im);
	}
			

	//*************************** draw the transparent planes of each box ***********************************
	glEnable(GL_BLEND);
	m_program->setUniformValue(m_hasTex, true);
	m_program->setUniformValue(m_mvMatrixLoc, m_camera * m_world);
	m_program->setUniformValue(m_fixedPipeline, false);
	m_program->setUniformValue(m_assignedMode, true);
	glDepthMask(GL_FALSE);

	glEnable(GL_POLYGON_SMOOTH);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);  // Antialias the lines
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (!drawShapeWithColor)
	for (size_t i = 0; i < boxList.size(); i++){
		m_program->setUniformValue(m_assignedColor, toTranparent(colorList.at(i % colorList.size())));
		if (currentSelectBox == i)
		{
			m_program->setUniformValue(m_assignedColor, toTranparent(colorList.at(i % colorList.size())) * 1.5);
		}
		m_program->setUniformValue(m_mvMatrixLoc, m_camera * m_world * boxList.at(i).m_transform);
		for (size_t j = 0; j < 6; j++)
		{
			drawPlane(boxList.at(i).getTargetPlane(j, 0), boxList.at(i).getTargetPlane(j, 1), boxList.at(i).getTargetPlane(j, 2), boxList.at(i).getTargetPlane(j, 3));
		}
	}
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	//*************************** draw the edges of the box *************************************************
	glEnable(GL_BLEND);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);  // Antialias the lines
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	m_program->setUniformValue(m_fixedPipeline, true);
	if (!drawShapeWithColor)
		drawBoxLine();
	glDisable(GL_BLEND);
	m_program->release();
}

void GLWidget::changeDrawShape(){
	drawShapeWithColor = 1 - drawShapeWithColor;
	update();
}

void GLWidget::drawShape(int begin, int end, QMatrix4x4 boxTransform){

	m_logoVbo.bind();
	m_logoVbo.allocate(m_data.constData(), m_count* sizeof(GLfloat));

	m_program->setUniformValue(m_hasTex, true);
	m_program->setUniformValue(m_mvMatrixLoc, m_camera * m_world * boxTransform);

	glPointSize(3);
	glDrawArrays(GL_POINTS, begin, end - begin);

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
	glDrawArrays(GL_TRIANGLES, begin, end - begin);
}


void GLWidget::drawPoint(){

	m_logoVbo.bind();
	m_logoVbo.allocate(m_data.constData(), m_count * sizeof(GLfloat));

	m_program->setUniformValue(m_mvMatrixLoc, m_camera * m_world);
	m_program->setUniformValue(m_normalMatrixLoc, (m_world).normalMatrix());
	m_program->setUniformValue(m_hasTex, true);

	glPointSize(1);
	glDrawArrays(GL_POINTS, 0, rawPointCount);

}

void GLWidget::drawModel(GLMmodel * model){

	m_logoVbo.bind();
	m_logoVbo.allocate(m_data.constData(), m_count * sizeof(GLfloat));

	GLMgroup* group;
	group = model->groups;
	GLMtriangle* triangle;
	GLuint i;
	while (group) {
		if (strcmp(group->name, " pCone1") == 0)
			m_program->setUniformValue(m_assignedColor, QVector4D(0.0,0.0,0.9,0.0));
		else
			m_program->setUniformValue(m_assignedColor, QVector4D(0.9, 0.9, 0.0, 0.0));

		glDrawArrays(GL_TRIANGLES, group->arrayBegin, group->numtriangles * 3);
		group = group->next;
	}
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


//void addBoxPoint(GLWidget * gl, const Box &b, int x, int y, int z){
//	Vec3fShape it;
//	it = b.center
//		+ x * b.normal[0] * b.max[0] + (1 - x)* b.normal[0] * b.min[0]
//		+ y * b.normal[1] * b.max[1] + (1 - y)* b.normal[1] * b.min[1]
//		+ z * b.normal[2] * b.max[2] + (1 - z)* b.normal[2] * b.min[2];
//
//	gl->add(QVector3D(it[0], it[1], it[2]), QVector3D(b.normal[0][0], b.normal[0][1], b.normal[0][2]));
//	gl->addTex(QVector2D(0.0f, 0.0f));
//}
//
//void GLWidget::addBox(Box & b){
//	b.boxBegin = m_count / 8;
//	addBoxPoint(this, b, 0, 1, 1);
//	addBoxPoint(this, b, 1, 1, 1);
//	addBoxPoint(this, b, 1, 1, 0);
//
//	addBoxPoint(this, b, 1, 1, 0);
//	addBoxPoint(this, b, 0, 1, 0);
//	addBoxPoint(this, b, 0, 1, 1);
//
//	addBoxPoint(this, b, 0, 0, 1);
//	addBoxPoint(this, b, 0, 1, 1);
//	addBoxPoint(this, b, 0, 1, 0);
//
//	addBoxPoint(this, b, 0, 1, 0);
//	addBoxPoint(this, b, 0, 0, 0);
//	addBoxPoint(this, b, 0, 0, 1);
//
//	addBoxPoint(this, b, 0, 0, 0);
//	addBoxPoint(this, b, 1, 0, 0);
//	addBoxPoint(this, b, 1, 1, 0);
//
//	addBoxPoint(this, b, 1, 1, 0);
//	addBoxPoint(this, b, 0, 1, 0);
//	addBoxPoint(this, b, 0, 0, 0);
//	b.boxEnd = m_count / 8;
//}

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
			if (vertex[index].length() < 0.001)
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

struct BoxInFile
{
	float vertex[8][3];
};

void readBoxInFile(std::string name, struct BoxInFile ** kp, int * pint)
{
	FILE *stream;
	if ((stream = fopen(name.c_str(), "rb")) == NULL) /* open file TEST.$$$ */
	{
		fprintf(stderr, "Cannot open output file.\n");
		return;
	}
	int sizeInt;
	fread(&sizeInt, sizeof(int), 1, stream);
	*pint = sizeInt;
	*kp = (struct BoxInFile *)malloc(sizeInt * sizeof(struct BoxInFile));
	fread(*kp, sizeof(struct BoxInFile), sizeInt, stream);
	fclose(stream);
}

void GLWidget::boxTest(){
	cv::Mat back;

	cv::FileStorage fs((fileHead + std::string("_ground.xml")).c_str(), cv::FileStorage::READ);
	fs["vocabulary"] >> back;
	addPointCloud(back);
	shapeDetect(1);
	shapeRange.clear();
	fs.release();

	pc.clear();
	addPointCloud(grabResult);
	shapeDetect();

	////pc.clear();
	////fs.open("data0_cabnet.xml", cv::FileStorage::READ);
	////fs["vocabulary"] >> back;
	////addPointCloud(back);
	////shapeDetect();
	////fs.release();

	////pc.clear();
	////fs.open("data0_drawer1.xml", cv::FileStorage::READ);
	////fs["vocabulary"] >> back;
	////addPointCloud(back);
	////shapeDetect();
	////fs.release();

	////pc.clear();
	////fs.open("data0_drawer2.xml", cv::FileStorage::READ);
	////fs["vocabulary"] >> back;
	////addPointCloud(back);
	////shapeDetect();
	////fs.release();


	////pc.clear();
	////fs.open("data0_door1.xml", cv::FileStorage::READ);
	////fs["vocabulary"] >> back;
	////addPointCloud(back);
	////shapeDetect();
	////fs.release();

	////pc.clear();
	////fs.open("data0_door2.xml", cv::FileStorage::READ);
	////fs["vocabulary"] >> back;
	////addPointCloud(back);
	////shapeDetect();
	////fs.release();

	////BoxHingeJoint* joint1 = new BoxHingeJoint(boxList.at(0), boxList.at(3));
	////joint1->setPivotPointIndex(2,6);
	////joint1->setPivotPoint(boxList.at(3).vertex[2], boxList.at(3).vertex[6]);
	////joint1->setBoxIndex(0,3);
	//////joint1->rotate(90);
	////joint1->setRange(-180, 180);

	////BoxHingeJoint *joint2 = new BoxHingeJoint(boxList.at(0), boxList.at(4));
	////joint2->setPivotPointIndex(3, 7);
	////joint2->setPivotPoint(boxList.at(4).vertex[3], boxList.at(4).vertex[7]);
	//////joint2->rotate(-90);
	////joint2->setBoxIndex(0, 4);
	////joint2->setRange(-180, 180);

	////BoxSliderJoint *joint3 = new BoxSliderJoint(boxList.at(0), boxList.at(1));
	////joint3->setPivotPointIndex(0, 1);
	////joint3->setPivotPoint(boxList.at(1).vertex[0], boxList.at(1).vertex[1]);
	////joint3->setBoxIndex(0, 1);
	////joint3->slide(0);
	////joint3->setRange(-2, 2);

	////BoxSliderJoint *joint4 = new BoxSliderJoint(boxList.at(0), boxList.at(2));
	////joint4->setPivotPointIndex(0,1);
	////joint4->setPivotPoint(boxList.at(2).vertex[0], boxList.at(2).vertex[1]);
	////joint4->setBoxIndex(0, 2);
	////joint4->slide(0);
	////joint4->setRange(-2, 2);

	////jointList.push_back(joint1);
	////jointList.push_back(joint2);
	////jointList.push_back(joint3);
	////jointList.push_back(joint4);

	emit jointUpdate(jointList);
	emit boxUpdate(boxList);

	//struct BoxInFile k;
	//struct BoxInFile * kp = (struct BoxInFile *)malloc(boxList.size() * sizeof(struct BoxInFile));

	//int num = boxList.size();

	//for (size_t i = 0; i < boxList.size(); i++)
	//{
	//	for (size_t j = 0; j < 8; j++)
	//	{
	//		kp[i].vertex[j][0] = boxList.at(i).vertex[j][0];
	//		kp[i].vertex[j][1] = boxList.at(i).vertex[j][1];
	//		kp[i].vertex[j][2] = boxList.at(i).vertex[j][2];
	//		
	//	}
	//}

	//FILE *stream;
	//if ((stream = fopen("data0.box", "wb")) == NULL) /* open file TEST.$$$ */
	//{
	//	fprintf(stderr, "Cannot open output file.\n");
	//	return;
	//}
	//fwrite(&num, sizeof(num), 1, stream);
	//fwrite(kp, sizeof(struct BoxInFile), boxList.size(), stream);
	//fclose(stream); 

	//struct BoxInFile * ap;
	//int numBox;
	//readBoxInFile("data0.box", &ap, &numBox);

}

//void GLWidget::boxTest(){
//	cv::Mat back;
//
//	cv::FileStorage fs("data2_ground.xml", cv::FileStorage::READ);
//	fs["vocabulary"] >> back;
//	addPointCloud(back);
//	shapeDetect(1);
//	shapeRange->clear();
//	fs.release();
//
//	pc.clear();
//	addPointCloud(grabResult);
//	shapeDetect();
//
//	emit jointUpdate(jointList);
//	emit boxUpdate(boxList);
//}


void GLWidget::deleteCurrentBox(){
	if (currentBox == -1)
	{
		return;
	}
	std::vector<Box>::iterator it = boxList.begin();
	for (size_t i = 1; i < currentBox; i++)
	{
		it++;
	}
	
	boxList.erase(it);
	update();
	currentBox = -1;
}

void GLWidget::boxDoubleClick(const QModelIndex &qm){
	currentSelectBox = qm.row();
	//QMessageBox::information(0, tr("Cannot dock"), tr(QString::number(currentSelectBox).toStdString().c_str()));
	emit boxUpdate(boxList.at(currentSelectBox).selectedPlaneIndex, boxList.at(currentSelectBox).selectedPointIndex[0], boxList.at(currentSelectBox).selectedPointIndex[1]);
	update();
}

void GLWidget::jointDoubleClick(const QModelIndex & qm){
	//QMessageBox::information(0, tr("Cannot dock"), tr("Main window already closed"));
	currentJoint = (jointList.at(qm.row()));
	doubleClickMask = true;
	switch (currentJoint->getType())
	{
	case BoxJoint::HINGE:{
		emit jointSliderChanged(currentJoint->getCurrentValue(), currentJoint->getRangeMin(), currentJoint->getRangeMax());
		break;
	}
	case BoxJoint::SLIDER:{
		emit jointSliderChanged(currentJoint->getCurrentValue(), currentJoint->getRangeMin(), currentJoint->getRangeMax());
		break;
	}
	default:
		break;
	}
}

void GLWidget::parentBoxSelect(int index){
	indexParentBox = index;
	update();
}

void GLWidget::planeSelect(int index){
	if (currentSelectBox != -1)
		boxList.at(currentSelectBox).selectedPlaneIndex = index;
	update();
}

void GLWidget::vertexSelect(int index, int which){
	if (currentSelectBox != -1)
		boxList.at(currentSelectBox).selectedPointIndex[which] = index;
	update();
}

void GLWidget::childBoxSelect(int index){
	indexChildBox = index;
	update();
}

void GLWidget::jointSliderValueChanged(int pValue){
	if (doubleClickMask)
	{
		doubleClickMask = false;
		return;
	}
	if (!currentJoint)
	{
		return;
	}

	double pos = pValue / 1000.0f * (currentJoint->getRangeMax() - currentJoint->getRangeMin()) + currentJoint->getRangeMin();
	switch (currentJoint->getType())
	{
		case BoxJoint::HINGE:{
		currentJoint->rotate(pos);
		break;
		}
		case BoxJoint::SLIDER:{
		 currentJoint->slide(pos);
		 break;
		}
	default:
		break;
	}
	update();
}

void GLWidget::editSliderValueChanged(int pValue){
	if (bEditLength){
		Vec3fShape point1, point2;
		int targetPlaneIndex = boxList.at(indexChildBox).selectedPlaneIndex;
		if (targetPlaneIndex % 2 == 0)
			targetPlaneIndex++;
		else
			targetPlaneIndex--;

		point1 = boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 0);
		point2 = boxList.at(indexChildBox).getPlane(1);
		if (iEditLength)
		{
			iEditLength--;
			fixedLength = (point1 - point2).length();
			return;
		}

		double length = fixedLength + (pValue) / 500.0;
		if (length<0)
			return;

		Vec3fShape len = (point2 - point1);
		len = -boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 0) + boxList.at(indexChildBox).getPlane(1);
		len.normalize();

		boxList.at(indexChildBox).getPlane(1) =
			boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 0) + len *length;

		len = -boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 1) + boxList.at(indexChildBox).getPlane(0);
		len.normalize();
		boxList.at(indexChildBox).getPlane(0) =
			boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 1) + len *length;

		len = -boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 2) + boxList.at(indexChildBox).getPlane(3);
		len.normalize();
		boxList.at(indexChildBox).getPlane(3) =
			boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 2) + len *length;

		len = -boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 3) + boxList.at(indexChildBox).getPlane(2);
		len.normalize();
		boxList.at(indexChildBox).getPlane(2) =
			boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 3) + len *length;
	}

	if (bMoveBox)
	{
		Vec3fShape point1, point2;
		int targetPlaneIndex = boxList.at(indexChildBox).selectedPlaneIndex;
		if (targetPlaneIndex % 2 == 0)
			targetPlaneIndex++;
		else
			targetPlaneIndex--;

		point1 = boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 0);
		point2 = boxList.at(indexChildBox).getPlane(1);
		if (iEditLength)
		{
			iEditLength--;
			for (size_t i = 0; i < 8; i++)
			{
				fixedPos[i] = boxList.at(indexChildBox).vertex[i];
			}
			return;
		}

		double length = (pValue) / 500.0;

		Vec3fShape len = (point2 - point1);
		//len = -boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 0) + boxList.at(indexChildBox).getPlane(1);
		//len.normalize();
		for (size_t i = 0; i < 8; i++)
		{
			boxList.at(indexChildBox).vertex[i] = fixedPos[i] + len * length;
		}
	}

	if (bRotateBox)
	{
		if (iEditLength)
		{
			iEditLength--;
			for (size_t i = 0; i < 8; i++)
			{
				fixedPos[i] = boxList.at(indexChildBox).vertex[i];
			}
			return;
		}

		double rAngle = (pValue) / 500.0 * 180.0;

		QMatrix4x4 pos;
		pos.setToIdentity();
		pos.translate(-ctrlRotateCenter[0], -ctrlRotateCenter[1], -ctrlRotateCenter[2]);
		QMatrix4x4 rot;
		rot.setToIdentity();
		rot.rotate(rAngle, ctrlRotateAxis[0], ctrlRotateAxis[1], ctrlRotateAxis[2]);
		QMatrix4x4 dPos;
		dPos.setToIdentity();
		dPos.translate(ctrlRotateCenter[0], ctrlRotateCenter[1], ctrlRotateCenter[2]);

		QVector4D vPos;
		for (size_t i = 0; i < 8; i++)
		{
			vPos = QVector4D(fixedPos[i][0], fixedPos[i][1], fixedPos[i][2], 1.0);
			vPos = dPos * rot * pos * vPos;
			boxList.at(indexChildBox).vertex[i] = Vec3fShape(vPos.x(),vPos.y(),vPos.z());
		}
	}

	update();
}

bool less_second(const std::pair< MiscLib::RefCountPtr< PrimitiveShape >, size_t > & m1, 
const std::pair< MiscLib::RefCountPtr< PrimitiveShape >, size_t > & m2) {
	return m1.second > m2.second;
}


void GLWidget::shapeDetect(int signForGround ){
	if (signForGround && bnormalGroundSet)
		return;
	if (signForGround)
		bnormalGroundSet = true;


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

	if (!signForGround)
	for (size_t j = 0; j < shapes.size(); j++)
	{	
		singleShapeSize = shapes[j].second;
		m_data.resize(m_data.size() + singleShapeSize * 8);
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
			QVector4D colorValue = colorList.at((shapeRange.size()-1 + boxList.size()) % colorList.size());
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
		//m_data.clear();
		//m_count = 0;
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
				pnormal[2] = pnormal[0].cross(pnormal[1]);

			}
			else
			{
				pnormal[1] = pnormal[0].cross(normalGround);
				pnormal[2] = pnormal[0].cross(pnormal[1]);
				pnormal[1] = pnormal[0].cross(pnormal[2]);
				pnormal[2] = pnormal[0].cross(pnormal[1]);
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
					pnormal[2] = pnormal[0].cross(pnormal[1]);
				}
			}
			else
				pnormal[1] = normalGround;
			pnormal[2] = pnormal[0].cross(pnormal[1]);
			pnormal[1] = pnormal[0].cross(pnormal[2]);
			pnormal[2] = pnormal[0].cross(pnormal[1]);
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
	boxCenter = center;
	float pmax[3] = { maxLength[0], maxLength[1], maxLength[2] };
	float pmin[3] = { minLength[0], minLength[1], minLength[2] };

	Box nBox(center, pnormal, pmax, pmin);
	for (size_t i = 0; i < shapeRange.size(); i++)
		nBox.shapeRange->push_back(shapeRange.at(i));
	shapeRange.clear();
	
	boxList.push_back(nBox);
	currentBox = boxList.size();
	update();
}

void GLWidget::addConstraint(int index){
	bEditLength = false;
	bMoveBox = false;
	bRotateBox = false;
	switch (index)
	{
		case 0:{
				   VEC4D abcd = VEC4D(0, 0, 0, 0);
				   VEC3D corners[3] = {
					   VEC3D(0, 0, 0),
					   VEC3D(0, 1, 0),
					   VEC3D(1, 0, 0)
				   };
				   
				   for (size_t i = 0; i < 3; i++)
				   {
					   corners[i] = vec3fShape2VEC3D(boxList.at(indexParentBox).getPlane(i));
				   }
				   gatherFaces(abcd,corners);
				   //void align_seg_2_plane(const VEC3D seg_ends[2], const VEC4D &plane, MAT3D &out_R, VEC3D &out_t)
				   VEC3D seg_ends[2];
				   seg_ends[0] = vec3fShape2VEC3D(boxList.at(indexChildBox).vertex[boxList.at(indexChildBox).selectedPointIndex[0]]);
				   seg_ends[1] = vec3fShape2VEC3D(boxList.at(indexChildBox).vertex[boxList.at(indexChildBox).selectedPointIndex[1]]);
				   MAT3D out_R;
				   VEC3D out_t;
				   align_seg_2_plane(seg_ends,abcd,out_R,out_t);
				   for (size_t i = 0; i < 8; i++)
				   {
					   boxList.at(indexChildBox).vertex[i] = 
						   VEC3D2vec3fShape(out_R * vec3fShape2VEC3D(boxList.at(indexChildBox).vertex[i]) + out_t);
				   }
				   if (boxList.at(indexChildBox).joint)
				   boxList.at(indexChildBox).joint->setPivotPointIndex(boxList.at(indexChildBox).pivotPoint[0], boxList.at(indexChildBox).pivotPoint[1]);

				   update();
				   
		}
			break;
		case 1:{
				   VEC3D seg_ends_parent[2];
				   seg_ends_parent[0] = vec3fShape2VEC3D(boxList.at(indexParentBox).vertex[boxList.at(indexParentBox).selectedPointIndex[0]]);
				   seg_ends_parent[1] = vec3fShape2VEC3D(boxList.at(indexParentBox).vertex[boxList.at(indexParentBox).selectedPointIndex[1]]);

				   VEC3D seg_ends_child[2];
				   seg_ends_child[0] = vec3fShape2VEC3D(boxList.at(indexChildBox).vertex[boxList.at(indexChildBox).selectedPointIndex[0]]);
				   seg_ends_child[1] = vec3fShape2VEC3D(boxList.at(indexChildBox).vertex[boxList.at(indexChildBox).selectedPointIndex[1]]);

				   MAT3D out_R;
				   VEC3D out_t;

				   //make_seg_parallel(const VEC3D seg1_ends[2], const VEC3D seg2_ends[2], MAT3D &out_R, VEC3D &out_t)
				   make_seg_parallel(seg_ends_child, seg_ends_parent, out_R, out_t);
				   for (size_t i = 0; i < 8; i++)
				   {
					   boxList.at(indexChildBox).vertex[i] =
						   VEC3D2vec3fShape(out_R * vec3fShape2VEC3D(boxList.at(indexChildBox).vertex[i]) + out_t);
				   }
				   if (boxList.at(indexChildBox).joint)
				   boxList.at(indexChildBox).joint->setPivotPointIndex(boxList.at(indexChildBox).pivotPoint[0], boxList.at(indexChildBox).pivotPoint[1]);


		}
			break;
		case 2:{	//equal length
				   Vec3fShape point1, point2;

				   point1 = boxList.at(indexParentBox).vertex[boxList.at(indexParentBox).selectedPointIndex[0]];
				   point2 = boxList.at(indexParentBox).vertex[boxList.at(indexParentBox).selectedPointIndex[1]];
				   double length = (point1 - point2).length();
				   int targetPlaneIndex = boxList.at(indexChildBox).selectedPlaneIndex;
				   if (targetPlaneIndex % 2 == 0)
					   targetPlaneIndex++;
				   else
					   targetPlaneIndex--;
				   Vec3fShape len;
				   len = boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 0) - boxList.at(indexChildBox).getPlane(1);
				   len.normalize();
				   boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 0) = 
					   boxList.at(indexChildBox).getPlane(1) + len *length;

				   len = boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 1) - boxList.at(indexChildBox).getPlane(0);
				   len.normalize();
				   boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 1) =
					   boxList.at(indexChildBox).getPlane(0) + len *length;

				   len = boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 2) - boxList.at(indexChildBox).getPlane(3);
				   len.normalize();
				   boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 2) =
					   boxList.at(indexChildBox).getPlane(3) + len *length;

				   len = boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 3) - boxList.at(indexChildBox).getPlane(2);
				   len.normalize();
				   boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 3) =
					   boxList.at(indexChildBox).getPlane(2) + len *length;

		}
			break;
		case 3:{	//set length
				   //Vec3fShape pointParent;
				   //Vec3fShape pointChild;
				   //pointParent = boxList.at(indexParentBox).vertex[boxList.at(indexParentBox).selectedPointIndex[0]];
				   //pointChild = boxList.at(indexChildBox).vertex[boxList.at(indexChildBox).selectedPointIndex[0]];

				   QVector4D qVertex[3];
				   Vec3fShape pointsParent[3];
				   Vec3fShape pointsChild[3];
				   Vec3fShape temp;
				   if (boxList.at(indexParentBox).joint)
				   {
					   //boxList.at(indexParentBox).m_transform * QVector4D((boxList.at(indexParentBox)))
					   for (size_t i = 0; i < 3; i++)
					   {
						   temp = boxList.at(indexParentBox).getPlane(i);
						   qVertex[i] = QVector4D(temp[0], temp[1], temp[2], 1.0);
						   qVertex[i] = boxList.at(indexParentBox).m_transform * qVertex[i];
						   pointsParent[i] = Vec3fShape(qVertex[i].x(), qVertex[i].y(), qVertex[i].z());
					   }
				   }
				   else
				   {
					   for (size_t i = 0; i < 3; i++)
					   {
						   pointsParent[i] = boxList.at(indexParentBox).getPlane(i);
					   }
				   }

				   if (boxList.at(indexChildBox).joint)
				   {
					   //boxList.at(indexChildBox).m_transform * QVector4D((boxList.at(indexChildBox)))
					   for (size_t i = 0; i < 3; i++)
					   {
						   temp = boxList.at(indexChildBox).getPlane(i);
						   qVertex[i] = QVector4D(temp[0], temp[1], temp[2], 1.0);
						   qVertex[i] = boxList.at(indexChildBox).m_transform * qVertex[i];
						   pointsChild[i] = Vec3fShape(qVertex[i].x(), qVertex[i].y(), qVertex[i].z());
					   }
				   }
				   else
				   {
					   for (size_t i = 0; i < 3; i++)
					   {
						   pointsChild[i] = boxList.at(indexChildBox).getPlane(i);
					   }
				   }

				   Vec3fShape normal;
				   normal = (pointsParent[2] - pointsParent[1]).cross(pointsParent[0] - pointsParent[1]);
				   normal.normalize();

				   double dis[2];
				   for (size_t i = 0; i < 2; i++)
				   {
					   dis[i] = (pointsChild[0] - pointsParent[0]).dot(normal);
				   }

				   if (dis[0] < 0 && dis[1] < 0){
					   normal = -normal;
					   for (size_t i = 0; i < 2; i++)
					   {
						   dis[i] = (pointsChild[0] - pointsParent[0]).dot(normal);
					   }
				   }

				   if (dis[0] > dis[1]){
					   dis[1] = dis[0];

				   }
				   else{
					   dis[0] = dis[1];

				   }

				   int targetPlaneIndex = boxList.at(indexParentBox).selectedPlaneIndex;

				   if (targetPlaneIndex % 2 == 0)
					   targetPlaneIndex++;
				   else
					   targetPlaneIndex--;

				   double length = dis[0];
				   Vec3fShape len;
				   
				   targetPlaneIndex = boxList.at(indexChildBox).selectedPlaneIndex;
				   if (targetPlaneIndex % 2 == 0)
					   targetPlaneIndex++;
				   else
					   targetPlaneIndex--;
				   len = boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 0) - boxList.at(indexChildBox).getPlane(1);
				   len.normalize();

				   boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 0) =
					   boxList.at(indexChildBox).getPlane(1) + len *length;

				   boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 1) =
					   boxList.at(indexChildBox).getPlane(0) + len *length;

				   boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 2) =
					   boxList.at(indexChildBox).getPlane(3) + len *length;

				   boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 3) =
					   boxList.at(indexChildBox).getPlane(2) + len *length;

		}
			break;
		case 4:{	//rotation

				   VEC3D seg_ends_parent[2];
				   seg_ends_parent[0] = vec3fShape2VEC3D(boxList.at(indexParentBox).vertex[boxList.at(indexParentBox).selectedPointIndex[0]]);
				   seg_ends_parent[1] = vec3fShape2VEC3D(boxList.at(indexParentBox).vertex[boxList.at(indexParentBox).selectedPointIndex[1]]);

				   VEC3D seg_ends_child[2];
				   seg_ends_child[0] = vec3fShape2VEC3D(boxList.at(indexChildBox).vertex[boxList.at(indexChildBox).selectedPointIndex[0]]);
				   seg_ends_child[1] = vec3fShape2VEC3D(boxList.at(indexChildBox).vertex[boxList.at(indexChildBox).selectedPointIndex[1]]);

				   MAT3D out_R;
				   VEC3D out_t;

				   make_seg_parallel(seg_ends_child, seg_ends_parent, out_R, out_t);

				   //double calc_jnt_angle(const MAT3D &input_R, const VEC3D &jnt_axis);
				   VEC3D jnt_axis;
				   if (!boxList.at(indexChildBox).joint)
					   return;

				   jnt_axis = vec3fShape2VEC3D(boxList.at(indexChildBox).joint->getPivotPoint(1) - boxList.at(indexChildBox).joint->getPivotPoint(0));
				   boxList.at(indexChildBox).joint->rotate(calc_jnt_angle(out_R, jnt_axis));
				   
		}
			break;
		case 5:{
				   iEditLength = 1;
				   emit editSliderReset();
				   bEditLength = true;
				   
		}
			break;
		case 6:{	//move boxes, using the same signals as the length edit
				   iEditLength = 1;
				   emit editSliderReset();
				   bMoveBox = true;

		}
			break;
		case 7:{	//equal cover
				   QVector4D qVertex[3];
				   Vec3fShape pointsParent[3];
				   Vec3fShape pointsChild[3];
				   Vec3fShape temp;
				   if (boxList.at(indexParentBox).joint)
				   {
					   //boxList.at(indexParentBox).m_transform * QVector4D((boxList.at(indexParentBox)))
					   for (size_t i = 0; i < 3; i++)
					   {
						   temp = boxList.at(indexParentBox).getPlane(i);
						   qVertex[i] = QVector4D(temp[0], temp[1], temp[2], 1.0);
						   qVertex[i] = boxList.at(indexParentBox).m_transform * qVertex[i];
						   pointsParent[i] = Vec3fShape(qVertex[i].x(), qVertex[i].y(), qVertex[i].z());
					   }
				   }
				   else
				   {
					   for (size_t i = 0; i < 3; i++)
					   {
						   pointsParent[i] = boxList.at(indexParentBox).getPlane(i);
					   }
				   }

				   if (boxList.at(indexChildBox).joint)
				   {
					   //boxList.at(indexChildBox).m_transform * QVector4D((boxList.at(indexChildBox)))
					   for (size_t i = 0; i < 3; i++)
					   {
						   temp = boxList.at(indexChildBox).getPlane(i);
						   qVertex[i] = QVector4D(temp[0], temp[1], temp[2], 1.0);
						   qVertex[i] = boxList.at(indexChildBox).m_transform * qVertex[i];
						   pointsChild[i] = Vec3fShape(qVertex[i].x(), qVertex[i].y(), qVertex[i].z());
					   }
				   }
				   else
				   {
					   for (size_t i = 0; i < 3; i++)
					   {
						   pointsChild[i] = boxList.at(indexChildBox).getPlane(i);
					   }
				   }

				   Vec3fShape normal;
				   normal = (pointsParent[2] - pointsParent[1]).cross(pointsParent[0] - pointsParent[1]);
				   normal.normalize();

				   double dis[2];
				   for (size_t i = 0; i < 2; i++)
				   {
					   dis[i] = (pointsChild[0] - pointsParent[0]).dot(normal);
				   }

				   if (dis[0] < 0 && dis[1] < 0){
					   normal = -normal;
					   for (size_t i = 0; i < 2; i++)
					   {
						   dis[i] = (pointsChild[0] - pointsParent[0]).dot(normal);
					   }
				   }
					  
				   if (dis[0] > dis[1]){
					   dis[1] = dis[0];
					   
				   }
				   else{
					   dis[0] = dis[1];
					  
				   }

				   int targetPlaneIndex = boxList.at(indexParentBox).selectedPlaneIndex;

				   if (targetPlaneIndex % 2 == 0)
					   targetPlaneIndex++;
				   else
					   targetPlaneIndex--;

				   double length = dis[0] / 2;
				   Vec3fShape len;
				   len = boxList.at(indexParentBox).getTargetPlane(targetPlaneIndex, 0) - boxList.at(indexParentBox).getPlane(1);
				   len.normalize();

				   boxList.at(indexParentBox).getTargetPlane(targetPlaneIndex, 0) =
					   boxList.at(indexParentBox).getPlane(1) + len *length;

				   boxList.at(indexParentBox).getTargetPlane(targetPlaneIndex, 1) =
					   boxList.at(indexParentBox).getPlane(0) + len *length;

				   boxList.at(indexParentBox).getTargetPlane(targetPlaneIndex, 2) =
					   boxList.at(indexParentBox).getPlane(3) + len *length;

				   boxList.at(indexParentBox).getTargetPlane(targetPlaneIndex, 3) =
					   boxList.at(indexParentBox).getPlane(2) + len *length;

				   targetPlaneIndex = boxList.at(indexChildBox).selectedPlaneIndex;
				   if (targetPlaneIndex % 2 == 0)
					   targetPlaneIndex++;
				   else
					   targetPlaneIndex--;
				   len = boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 0) - boxList.at(indexChildBox).getPlane(1);
				   len.normalize();

				   boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 0) =
					   boxList.at(indexChildBox).getPlane(1) + len *length;

				   boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 1) =
					   boxList.at(indexChildBox).getPlane(0) + len *length;

				   boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 2) =
					   boxList.at(indexChildBox).getPlane(3) + len *length;

				   boxList.at(indexChildBox).getTargetPlane(targetPlaneIndex, 3) =
					   boxList.at(indexChildBox).getPlane(2) + len *length;
					

				   
		}
			break;
		case 8:{	//add hinge joint

				   BoxHingeJoint* joint1 = new BoxHingeJoint(boxList.at(indexParentBox), boxList.at(indexChildBox));
				   joint1->setPivotPointIndex(boxList.at(indexChildBox).selectedPointIndex[0], boxList.at(indexChildBox).selectedPointIndex[1]);
				   joint1->setBoxIndex(indexParentBox, indexChildBox);
				   joint1->setRange(-180, 180);
				   jointList.push_back(joint1);
				   emit jointUpdate(jointList);
		}
			break;
		case 9:{
				   BoxSliderJoint *joint3 = new BoxSliderJoint(boxList.at(indexParentBox), boxList.at(indexChildBox));
				   joint3->setPivotPointIndex(boxList.at(indexChildBox).selectedPointIndex[0], boxList.at(indexChildBox).selectedPointIndex[1]);
				   
				   joint3->setBoxIndex(indexParentBox, indexChildBox);
				   joint3->slide(0);
				   joint3->setRange(-3, 3);
				   jointList.push_back(joint3);
				   emit jointUpdate(jointList);

		}
			break;
		case 10:{
					ctrlRotateCenter = Vec3fShape(0, 0, 0);
					for (size_t i = 0; i < 8; i++)
					{
						ctrlRotateCenter += boxList.at(indexChildBox).vertex[i];
					}
					ctrlRotateCenter /= 8;

					ctrlRotateAxis = boxList.at(indexChildBox).vertex[boxList.at(indexChildBox).selectedPointIndex[0]] -
						boxList.at(indexChildBox).vertex[boxList.at(indexChildBox).selectedPointIndex[1]];
					ctrlRotateAxis.normalize();

					iEditLength = 1;
					emit editSliderReset();
					bRotateBox = true;

		}
			break;
		default:
			break;
	}
	update();
}

void GLWidget::delJoint(){
	std::vector<BoxJoint *>::iterator it = jointList.begin();
	for (size_t i = 0; i < inxDelJoint; i++)
		it++;
	(*it)->child.joint = NULL;
	(*it)->~BoxJoint();

	jointList.erase(it);
	emit jointUpdate(jointList);

	currentJoint = NULL;
	update();
}
void GLWidget::delBox(){
	std::vector<Box>::iterator it = boxList.begin();
	for (size_t i = 0; i < inxDelBox; i++)
		it++;

	boxList.erase(it);
	emit boxUpdate(boxList);
	currentBox = -1;
	update();
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
	this->setFocus();
    m_lastPos = event->pos();

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

void GLWidget::setTrans(float dx, float dy, float dz,int mode){
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
		if (mode == 0){
			eye += QVector3D(dx, dy, dz);
			at += QVector3D(dx, dy, dz);
		}
		else
		{
			eye += QVector3D(dx, dy, dz);
			//at += QVector3D(dx, dy, dz);
		}

		m_camera.lookAt(eye, at, QVector3D(0, 1, 0));
		//m_xRot = m_yRot = m_zRot = 0;
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
	//int x = event->delta();
	if (event->delta() > 0 && event->delta() / 400.0 > (eye - at).length())
		eye = eye;
	else
		eye += event->delta() / 400.0 * dv.normalized();
	m_camera.lookAt(eye, at, QVector3D(0, 1, 0));
	update();
}

void GLWidget::keyPressEvent(QKeyEvent * event){
	
	switch (event->key()){
	case 'W':
		setTrans(0, -0.01, 0,eyeAtMode); 
		break;
	case 'S':
		setTrans(0, 0.01, 0, eyeAtMode);
		break;
	case 'A':
		setTrans(0.01, 0, 0, eyeAtMode);
		break;
	case 'D':
		setTrans(-0.01, 0, 0, eyeAtMode);
		break;
	case 'Q':
		setTrans(0, 0, -0.01, eyeAtMode);
		break;
	case 'E':
		setTrans(0, 0, 0.01, eyeAtMode);
		break;
	case Qt::Key_Space:{
		eyeAtMode = 1 - eyeAtMode;
	}
		break;
	case 'C':{
		setThisBoxCenter = true;
		update();
	}
		break;
	case 'Z':{
		for (size_t i = 0; i < jointList.size(); i++)
		{
			switch (jointList.at(i)->getType()){
			case BoxJoint::HINGE:{
				jointList.at(i)->rotate(0);
			}
				break;
			case BoxJoint::SLIDER:{
				jointList.at(i)->slide(0);
			}
				break;
			}
		}
		update();
	}
		break;
	//case '1':
	//	dx -= 0.005;
	//	updateRawPC(dx,dy,dz);
	//	break;
	//case '2':
	//	dx += 0.005;
	//	updateRawPC(dx, dy, dz);
	//	break;
	//case '3':
	//	dy -= 0.005;
	//	updateRawPC(dx, dy, dz);
	//	break;
	//case '4':
	//	dy += 0.005;
	//	updateRawPC(dx, dy, dz);
	//	break;
	//case '5':
	//	dz -= 0.005;
	//	updateRawPC(dx, dy, dz);
	//	break;
	//case '6':
	//	dz += 0.005;
	//	updateRawPC(dx, dy, dz);
	//	break;
	case 'N':
		storedEye = eye;
		storedAt = at;
		storedRx = m_xRot;
		storedRy = m_yRot;
		storedRz = m_zRot;
		break;
	case 'M':
		eye = storedEye;
		at = storedAt;
		m_xRot = storedRx;
		m_yRot = storedRy;
		m_zRot = storedRz;
		m_camera.setToIdentity();
		m_camera.lookAt(eye, at, QVector3D(0, 1, 0));
		update();
		break;
	}
}

void GLWidget::keyReleaseEvent(QKeyEvent * event){

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