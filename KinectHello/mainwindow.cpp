#include "mainwindow.h"

MainWindow::MainWindow() :rgbImage(NULL), depthImage(NULL)
{
    QMenuBar *menuBar = new QMenuBar;
    QMenu *menuWindow = menuBar->addMenu(tr("&Window"));
	QToolBar *toolBar = addToolBar(tr("&TEST"));
	QFont ft;
	ft.setBold(true);
	//actions of the tool bar
	QAction *widgetAction = new QAction(QIcon("Resources/open.png"),tr("&Open"), this);
	QAction *openAction = new QAction(tr("&open"), this);
	QAction *brutalModeWidgetAction = new QAction(QIcon("Resources/changeBrutal.png"), tr("&Mode"), this); 
	QAction *brutalFinishWidgetAction = new QAction(QIcon("Resources/brutal.png"), tr("&Done"), this);
	QAction *grabCutWidgetAction = new QAction(QIcon("Resources/grabcut.png"), tr("&Cut"), this);
	QAction *shapeWidgetAction = new QAction(QIcon("Resources/shape.png"), tr("&Shape"), this);
	QAction *boxTestWidgetAction = new QAction(QIcon("Resources/box.png"), tr("&Box"), this);
	QAction *shapeDrawWidgetAction = new QAction(QIcon("Resources/color.png"), tr("&Color"), this);
	QAction *deleteWidgetAction = new QAction(QIcon("Resources/delete.png"), tr("&Delete"), this);
	
	QAction *saveWidgetAction = new QAction(QIcon("Resources/save.png"), tr("&Save"), this);
	QAction *readWidgetAction = new QAction(QIcon("Resources/load.png"), tr("&Load"), this);

	openAction->setShortcut(QKeySequence::Open);
	openAction->setStatusTip(tr("Open a file."));
	menuWindow->addAction(openAction);

	QAction *coNoAction = new QAction(tr("&No coordinate"), this);
	QAction *coXYAction = new QAction(tr("&XY coordinate"), this);
	QAction *coYZAction = new QAction(tr("&YZ coordinate"), this);
	QAction *coXZAction = new QAction(tr("&XZ coordinate"), this);

	menuWindow->addAction(openAction);
	menuWindow->addAction(coNoAction);
	menuWindow->addAction(coXYAction);
	menuWindow->addAction(coYZAction);
	menuWindow->addAction(coXZAction);

	toolBar->addAction(widgetAction);
	toolBar->addAction(brutalModeWidgetAction); 
	toolBar->addAction(brutalFinishWidgetAction);
	toolBar->addAction(grabCutWidgetAction);
	toolBar->addAction(shapeWidgetAction);
	toolBar->addAction(boxTestWidgetAction); 
	toolBar->addAction(shapeDrawWidgetAction);
	toolBar->addAction(deleteWidgetAction);
	toolBar->addAction(saveWidgetAction);
	toolBar->addAction(readWidgetAction);
	toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

	//center area widget
	QWidget *centerWidget = new QWidget(this);
	QHBoxLayout *mainLayout = new QHBoxLayout(this);
	QVBoxLayout *leftLayout = new QVBoxLayout(this);
	glWidget = new GLWidget(this);
	

	rgbWidget = new PaintWidget(centerWidget);
	rgbWidget->setDrawable(true);
	depthWidget = new PaintWidget(centerWidget);
	depthWidget->setDrawable(false);

	rgbWidget->setMinimumSize(QSize(592, 444));
	depthWidget->setMinimumSize(QSize(592, 444));
	rgbWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	depthWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

	connect(grabCutWidgetAction, SIGNAL(triggered()), rgbWidget, SLOT(grabCutIteration()));
	
	leftLayout->addWidget(rgbWidget);
	leftLayout->addWidget(depthWidget);

	QWidget* imageWidget = new QWidget(this);
	imageWidget->setLayout(leftLayout);
	QTabWidget * leftTab = new QTabWidget();
	leftTab->addTab(imageWidget, tr("RGB + Depth"));

	QGroupBox *groupBox = new QGroupBox(tr("Graphics"));
	QVBoxLayout *cg = new QVBoxLayout(this);
	cg->addWidget(glWidget);
	groupBox->setLayout(cg);
	groupBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	mainLayout->addWidget(leftTab,23);
	mainLayout->addWidget(groupBox,45);
	centerWidget->setLayout(mainLayout);

	connect(openAction, SIGNAL(triggered()), this, SLOT(openFolder()));
	connect(coNoAction, SIGNAL(triggered()), glWidget, SLOT(nocoClicked()));
	connect(coXYAction, SIGNAL(triggered()), glWidget, SLOT(XYClicked()));
	connect(coYZAction, SIGNAL(triggered()), glWidget, SLOT(YZClicked()));
	connect(coXZAction, SIGNAL(triggered()), glWidget, SLOT(XZClicked()));

	connect(widgetAction, SIGNAL(triggered()), this, SLOT(openFolder()));
	connect(shapeWidgetAction, SIGNAL(triggered()), this, SLOT(grabResUpdated()));
	connect(shapeDrawWidgetAction, SIGNAL(triggered()), glWidget, SLOT(changeDrawShape()));
	connect(deleteWidgetAction, SIGNAL(triggered()), glWidget, SLOT(deleteCurrentBox()));
	connect(saveWidgetAction, SIGNAL(triggered()), glWidget, SLOT(save()));
	connect(readWidgetAction, SIGNAL(triggered()), glWidget, SLOT(read()));
	
	connect(boxTestWidgetAction, SIGNAL(triggered()), glWidget, SLOT(boxTest()));
	connect(brutalFinishWidgetAction, SIGNAL(triggered()), rgbWidget, SLOT(brutalModeState()));
	connect(brutalModeWidgetAction, SIGNAL(triggered()), rgbWidget, SLOT(brutalModeSwitch()));

	rgbWidget->setSlave(depthWidget);
    setMenuBar(menuBar);
	setCentralWidget(centerWidget);
	setWindowTitle(tr("Hello Kinect"));

	//the tab page of the boxed and joints
	QVBoxLayout *leftSecondLayout = new QVBoxLayout(this);
	QGroupBox *boxGroupBox = new QGroupBox(tr("Boxes"));
	//depthGroupBox->setLayout(depth);

	QVBoxLayout *boxGroupLayout = new QVBoxLayout(this);

	boxStd = new QStandardItemModel(this);
	boxStd->setHorizontalHeaderLabels(QStringList() << QStringLiteral("Boxes"));
	boxTree = new QTreeView(this);
	boxTree->setModel(boxStd);
	boxTree->setContextMenuPolicy(Qt::CustomContextMenu);
	boxGroupLayout->addWidget(boxTree);
	boxGroupBox->setLayout(boxGroupLayout);
	

	QGroupBox * controlPanel = new  QGroupBox(tr("Control Panel"));
	QVBoxLayout *controlPanelLayout = new QVBoxLayout(this);

	parentBox = new QComboBox(this);
	parentBox->setMaximumSize(QSize(180, 30));
	parentBox->setMinimumSize(QSize(0, 30));
	//parentBox->setFont(ft);

	childBox = new QComboBox(this);
	childBox->setMaximumSize(QSize(180, 30));
	childBox->setMinimumSize(QSize(0, 30));
	//childBox->setFont(ft);

	jointSlider = new QSlider(Qt::Horizontal, this);
	jointSlider->setRange(1, 100);
	jointSlider->setSingleStep(1);
	jointSlider->setPageStep(1);

	QLabel *sliderLabel = new QLabel("Joint Pose", this);
	controlPanelLayout->addWidget(sliderLabel);
	controlPanelLayout->addWidget(jointSlider);

	QLabel *editLabel = new QLabel("Length Edit", this);
	editSlider = new QSlider(Qt::Horizontal, this);
	editSlider->setRange(-499, 499);
	editSlider->setSingleStep(1);
	editSlider->setPageStep(1);

	controlPanelLayout->addWidget(editLabel);
	controlPanelLayout->addWidget(editSlider);

	QHBoxLayout * parentBoxLayout = new QHBoxLayout(this);
	QLabel *parentLabel = new QLabel("Parent Box Selection", this);
	parentBoxLayout->addWidget(parentLabel);
	parentBoxLayout->addWidget(parentBox);
	controlPanelLayout->addLayout(parentBoxLayout);

	QHBoxLayout * childBoxLayout = new QHBoxLayout(this);
	QLabel *childLabel = new QLabel("Child Box Selection", this);
	childBoxLayout->addWidget(childLabel);
	childBoxLayout->addWidget(childBox);
	controlPanelLayout->addLayout(childBoxLayout);
	controlPanel->setLayout(controlPanelLayout);
	leftSecondLayout->addWidget(controlPanel);

	QHBoxLayout * constraintLayout = new QHBoxLayout(this);
	QLabel *constraintLabel = new QLabel("Constraint Selection", this);
	constraintLayout->addWidget(constraintLabel);
	constraintBox = new QComboBox(this);
	constraintBox->setMaximumSize(QSize(240, 30));
	constraintBox->setMinimumSize(QSize(0, 30));
	constraintBox->addItem("Plane Edge Alignment");
	constraintBox->addItem("Edge Edge Alignment");
	constraintBox->addItem("Equal Length");
	constraintBox->addItem("Set Length");
	constraintBox->addItem("Rotatation");
	constraintBox->addItem("Length Editing");

	constraintLayout->addWidget(constraintBox);
	controlPanelLayout->addLayout(constraintLayout);

	QPushButton * addConsButton = new QPushButton("Add Constraint", this);
	controlPanelLayout->addWidget(addConsButton);


	//******************************************selection group******************************************
	QVBoxLayout *selectGroupLayout = new QVBoxLayout(this);
	QGroupBox *selectGroupBox = new QGroupBox(tr("Selection"));

	QLabel * vertexSetLabel = new QLabel(QString("Vertex: "), this);
	vertexSetLabel->setFont(ft);
	
	QLabel * vertexLabel[8];
	for (size_t i = 0; i < 8; i++)
	{
		vertexCheck[i] = new QCheckBox(this);
		vertexLabel[i] = new QLabel(QString("Vertex ") + QString::number(i), this);
	}
	QHBoxLayout* checkLayout1 = new QHBoxLayout(this);
	QHBoxLayout* checkLayout2 = new QHBoxLayout(this);

	for (size_t i = 0; i < 4; i++)
	{
		checkLayout1->addWidget(vertexLabel[i]);
		checkLayout1->addWidget(vertexCheck[i]);
		checkLayout2->addWidget(vertexLabel[4 + i]);
		checkLayout2->addWidget(vertexCheck[4 + i]);

	}
	QLabel * planeSetLabel = new QLabel(QString("Plane: "), this);
	planeSetLabel->setFont(ft);
	planeSelectBox = new QComboBox(this);
	planeSelectBox->setMaximumSize(QSize(180, 30));
	planeSelectBox->setMinimumSize(QSize(0, 30));

	for (size_t i = 0; i < 6; i++)
	{
		planeSelectBox->addItem(QString("Plane ") + QString::number(i));
	}

	selectGroupLayout->addWidget(vertexSetLabel);
	selectGroupLayout->addLayout(checkLayout1);
	selectGroupLayout->addLayout(checkLayout2);
	selectGroupLayout->addSpacing(10);
	QHBoxLayout * planeSetLayout = new QHBoxLayout(this);

	planeSetLayout->addWidget(planeSetLabel);
	planeSetLayout->addWidget(planeSelectBox);
	selectGroupLayout->addLayout(planeSetLayout);
	selectGroupBox->setLayout(selectGroupLayout);

	QPushButton * submitButton = new QPushButton("Submit", this);
	selectGroupLayout->addWidget(submitButton);
	leftSecondLayout->addWidget(selectGroupBox);

	//************************************************************************************


	leftSecondLayout->addWidget(boxGroupBox);

	QVBoxLayout *jointGroupLayout = new QVBoxLayout(this);
	QGroupBox *jointGroupBox = new QGroupBox(tr("Joints"));
	jointStd = new QStandardItemModel(this);
	jointStd->setHorizontalHeaderLabels(QStringList() << QStringLiteral("Joints"));
	jointTree = new QTreeView(this);
	jointTree->setModel(jointStd);
	jointTree->setContextMenuPolicy(Qt::CustomContextMenu);

	jointGroupLayout->addWidget(jointTree);
	//jointGroupLayout->addWidget(jointSlider);

	jointGroupBox->setLayout(jointGroupLayout);
	leftSecondLayout->addWidget(jointGroupBox);

	QWidget *leftSecond = new QWidget();
	leftSecond->setLayout(leftSecondLayout);
	leftTab->addTab(leftSecond,tr("Boxes + Joints"));

	connect(glWidget, SIGNAL(jointUpdate(std::vector<BoxJoint *>)), this, SLOT(jointUpdate(std::vector<BoxJoint *>)));
	connect(glWidget, SIGNAL(boxUpdate(std::vector<Box>)), this, SLOT(boxUpdate(std::vector<Box>)));
	connect(jointTree, SIGNAL(doubleClicked(const QModelIndex)), glWidget, SLOT(jointDoubleClick(const QModelIndex)));
	connect(boxTree, SIGNAL(doubleClicked(const QModelIndex)), glWidget, SLOT(boxDoubleClick(const QModelIndex)));

	connect(glWidget, SIGNAL(jointSliderChanged(double, double, double)), this, SLOT(jointSliderUpdate(double,double,double)));
	connect(jointSlider, SIGNAL(valueChanged(int)), glWidget, SLOT(jointSliderValueChanged(int)));
	connect(editSlider, SIGNAL(valueChanged(int)), glWidget, SLOT(editSliderValueChanged(int)));
	connect(submitButton, SIGNAL(clicked()), this, SLOT(selectSubmit()));
	connect(parentBox, SIGNAL(currentIndexChanged(int)), glWidget, SLOT(parentBoxSelect(int)));
	connect(childBox, SIGNAL(currentIndexChanged(int)), glWidget, SLOT(childBoxSelect(int)));
	//connect(planeSelectBox, SIGNAL(currentIndexChanged(int)), glWidget, SLOT(planeSelect(int)));
	connect(this, SIGNAL(planeSelect(int)), glWidget, SLOT(planeSelect(int)));
	connect(addConsButton, SIGNAL(clicked()), this, SLOT(addConstraint()));
	connect(this, SIGNAL(addConstraint(int)), glWidget, SLOT(addConstraint(int)));
	connect(this, SIGNAL(vertexSelect(int,int)), glWidget, SLOT(vertexSelect(int,int)));
	connect(glWidget, SIGNAL(boxUpdate(int, int, int)), this, SLOT(boxUpdate(int, int, int)));
	connect(glWidget, SIGNAL(editSliderReset()), this, SLOT(editSliderReset()));

	for (size_t i = 0; i < 8; i++)
	{
		connect(vertexCheck[i], SIGNAL(clicked()), this, SLOT(checkCheck()));
	}
	openFolder();
}

void MainWindow::addConstraint(){
	emit addConstraint(constraintBox->currentIndex());
}

void MainWindow::editSliderReset(){
	editSlider->setRange(-499, 499);
	editSlider->setSingleStep(1);
	editSlider->setPageStep(1);
	editSlider->setValue(0);
}

void MainWindow::selectSubmit(){
	int size = 0;
	int index[2];
	for (size_t i = 0; i < 8; i++)
	{
		if (vertexCheck[i]->isChecked()){
			size++;
			if (size == 1)
				index[0] = i;
			else
				index[1] = i;
		}
	}

	if (size > 2)
	{
		QMessageBox::information(0, tr("Please select 2 points."), tr("Please select 2 points."));
	}
	else if (size == 2)
	{
		emit vertexSelect(index[0], 0);
		emit vertexSelect(index[1], 1);
		emit planeSelect(planeSelectBox->currentIndex());
	}

}

void MainWindow::boxUpdate(int plane, int point1, int point2){
	if (plane == -1 || point1 == -1 || point2 == -1){
		QMessageBox::information(0, tr("Not set."), tr("Please set the points and plane selected."));
		for (size_t i = 0; i < 8; i++)
			vertexCheck[i]->setChecked(false);
		return;
	}
	planeSelectBox->setCurrentIndex(plane);
	for (size_t i = 0; i < 8; i++)
		vertexCheck[i]->setChecked(false);
	vertexCheck[point1]->setChecked(true);
	vertexCheck[point2]->setChecked(true);

}

void MainWindow::checkCheck(){

	
}

void MainWindow::jointDoubleClick(const QModelIndex & qm){
	//jointTree->model()->data
	//qm.row();
}

void MainWindow::jointSliderUpdate(double value, double min, double max){
	int pos = (value - min) / (max - min) * 100 +1;
	jointSlider->setValue(pos);
	jointSlider->setRange(0, 99);
	jointSlider->setSingleStep(1);
	jointSlider->setPageStep(1);
}

void MainWindow::jointUpdate(std::vector<BoxJoint *> pJointList){
	jointStd->clear();
	jointStd->setHorizontalHeaderLabels(QStringList() << QStringLiteral("Joint"));
	for (size_t i = 0; i < pJointList.size(); i++)
	{
		QStandardItem* itemProject = new QStandardItem(QString("Joint") + QString::number(i));
		itemProject->setEditable(false);
		jointStd->appendRow(itemProject);
	}
}

void MainWindow::boxUpdate(std::vector<Box> pBoxList){
	boxStd->clear();
	boxStd->setHorizontalHeaderLabels(QStringList() << QStringLiteral("Box"));
	parentBox->clear();
	childBox->clear();
	for (size_t i = 0; i < pBoxList.size(); i++)
	{
		QStandardItem* itemProject = new QStandardItem(QString("Box") + QString::number(i));
		itemProject->setEditable(false);
		boxStd->appendRow(itemProject);

		parentBox->addItem(QString("box") + QString::number(i));
		childBox->addItem(QString("box") + QString::number(i));
	}

}

void MainWindow::grabResUpdated(/*cv::Mat**/){
	
	//glWidget->shapeDetect();
	//imwrite("back.bmp", rgbWidget->gcapp.binMask);

	//FileStorage fs("data2_ground.xml", FileStorage::WRITE);
	//fs << "vocabulary" << rgbWidget->gcapp.binMask;
	//fs.release();

	rgbWidget->gcapp.binMask.copyTo(glWidget->grabResult);
}

void pixel2cam(int x, int y, float depth, float&camx, float& camy)
{
	camx = (static_cast<float>(x)-319.5f) * depth / 571.401f;
	camy = (static_cast<float>(y)-239.5f) * depth / 571.401f;
}

void MainWindow::openFolder(){

	path = QString("C:\\Users\\LeslieRong\\Desktop\\data0");

	//QFileDialog* openFilePath = new QFileDialog(this, "Please choose a folder", "Folder");
	//openFilePath->setFileMode(QFileDialog::DirectoryOnly);
	//if (openFilePath->exec() == QDialog::Accepted)
	//{
	//	//code here��
	//	if (!openFilePath->selectedFiles()[0].isEmpty())
	//		path = openFilePath->selectedFiles()[0];
	//	else
	//		QMessageBox::information(NULL, tr("Empty Path"), tr("You didn't select any path."));
	//}
	//delete openFilePath;
	//if (path.isEmpty())
	//	return ;
	
	QDir dir(path);

	rgbImage = new QImage(path + QString("/") + dir.dirName() + QString(".png"));
	glWidget->rgbMap = rgbImage;
	rgbWidget->setGrabCut(true);
	rgbWidget->setImage(*rgbImage);

	std::string dep_file = (path + QString("/") + dir.dirName() + QString(".depth")).toStdString();

	FILE* fb = fopen(dep_file.c_str(), "rb+");
	unsigned short *depth_img = NULL;
	int width, height;
	fread(&width, sizeof(width), 1, fb);
	fread(&height, sizeof(height), 1, fb);
	depth_img = new unsigned short[width*height];
	fread(depth_img, sizeof(unsigned short), width*height, fb);
	fclose(fb);
	fb = NULL;

	float mat_data[16]; 
	//fb = fopen((path + QString("/") + dir.dirName() + QString(".mat")).toStdString().c_str(), "rb+");
	//fread(mat_data, sizeof(mat_data), 1, fb);
	//fclose(fb);

	depthImage = new QImage(width, height, QImage::Format_ARGB32);
	int haha = rgbImage->width();
	int hehe = rgbImage->height();
	float camX = 0.0f;
	float camY = 0.0f;

	//glWidget->m_count = 0;
	//glWidget->m_data.resize(width * height * 8);

	glWidget->width = width;
	glWidget->height = height;

	for (int y = 0; y < height; ++y) {
		QRgb *destrow = (QRgb*)depthImage->scanLine(y);
		for (int x = 0; x < width; ++x) {
			unsigned int color = static_cast<short>((float)depth_img[y*width + x] / 4500.0f*255.0f);
			destrow[x] = qRgba(color, color, color, 255);
			pixel2cam(x, y, depth_img[y*width + x], camX, camY);
			glWidget->vertex.push_back(Vec3fShape(camX, camY, (float)depth_img[y * width + x]));

			if (QVector3D(camX / 1000.0f, camY / 1000.0f, depth_img[y * width + x] / 1000.0f).length() < 0.000000001)
				continue;
			
			//glWidget->add(QVector3D(camX / 1000.0f, camY / 1000.0f, depth_img[y * width + x] / 1000.0f), QVector3D(0, -1, -1));
			//glWidget->addTex(QVector2D((float)(x - 6) / (float)width, (float)y / (float)height));
		}
	}

	//glWidget->rawPointCount = glWidget->m_count / 8;

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if ( x == 0 || y == 0 || x == width-1 || y == height-1)
				glWidget->normal.push_back(Vec3fShape(0.0f, 0.0f, 0.0f));
			else
			{
				glWidget->normal.push_back((glWidget->vertex[(y) * width + x - 1] - glWidget->vertex[(y) * width + x + 1]).cross(
				glWidget->vertex[(y + 1) * width + x] - glWidget->vertex[(y - 1) * width + x]
				));
			}
		}
	}

	depthWidget->setImage(*depthImage);
	rgbWidget->setDepth(*depthImage);
	glWidget->update();

}

