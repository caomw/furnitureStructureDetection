#include "mainwindow.h"

MainWindow::MainWindow() :rgbImage(NULL), depthImage(NULL)
{
    QMenuBar *menuBar = new QMenuBar;
    QMenu *menuWindow = menuBar->addMenu(tr("&Window"));
	QToolBar *toolBar = addToolBar(tr("&TEST"));

	QAction *widgetAction = new QAction(QIcon("Resources/open.png"),tr("&Open"), this);
	QAction *openAction = new QAction(tr("&open"), this);
	QAction *brutalModeWidgetAction = new QAction(QIcon("Resources/changeBrutal.png"), tr("&Mode"), this); 
	QAction *brutalFinishWidgetAction = new QAction(QIcon("Resources/brutal.png"), tr("&Done"), this);
	QAction *grabCutWidgetAction = new QAction(QIcon("Resources/grabcut.png"), tr("&Cut"), this);
	QAction *shapeWidgetAction = new QAction(QIcon("Resources/shape.png"), tr("&Shape"), this);
	QAction *boxTestWidgetAction = new QAction(QIcon("Resources/box.png"), tr("&Box"), this);
	QAction *shapeDrawWidgetAction = new QAction(QIcon("Resources/color.png"), tr("&Color"), this);

	openAction->setShortcut(QKeySequence::Open);
	openAction->setStatusTip(tr("Open a file."));
	menuWindow->addAction(openAction);

	toolBar->addAction(widgetAction);
	toolBar->addAction(brutalModeWidgetAction); 
	toolBar->addAction(brutalFinishWidgetAction);
	toolBar->addAction(grabCutWidgetAction);
	toolBar->addAction(shapeWidgetAction);
	toolBar->addAction(boxTestWidgetAction); 
	toolBar->addAction(shapeDrawWidgetAction);
	
	toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

	QWidget *centerWidget = new QWidget(this);
	QHBoxLayout *mainLayout = new QHBoxLayout(this);
	QVBoxLayout *leftLayout = new QVBoxLayout(this);
	glWidget = new GLWidget(this);
	
	rgbWidget = new PaintWidget(centerWidget);
	rgbWidget->setDrawable(true);
	depthWidget = new PaintWidget(centerWidget);

	rgbWidget->setMinimumSize(QSize(548, 411));
	depthWidget->setMinimumSize(QSize(548, 411));
	rgbWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	depthWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

	connect(grabCutWidgetAction, SIGNAL(triggered()), rgbWidget, SLOT(grabCutIteration()));
	

	QVBoxLayout *rgb = new QVBoxLayout(this);
	rgb->addWidget(rgbWidget);

	QGroupBox *rgbGroupBox = new QGroupBox(tr("RGB"));
	rgbGroupBox->setLayout(rgb);

	leftLayout->addWidget(rgbGroupBox);
	leftLayout->addSpacing(10);

	QVBoxLayout *depth = new QVBoxLayout(this);
	depth->addWidget(depthWidget);

	QGroupBox *depthGroupBox = new QGroupBox(tr("Depth"));
	depthGroupBox->setLayout(depth);

	leftLayout->addWidget(depthGroupBox);

	QWidget* imageWidget = new QWidget(this);
	imageWidget->setLayout(leftLayout);

	QGroupBox *groupBox = new QGroupBox(tr("Graphics"));
	QVBoxLayout *cg = new QVBoxLayout(this);
	cg->addWidget(glWidget);
	groupBox->setLayout(cg);
	groupBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	mainLayout->addWidget(imageWidget,20);
	mainLayout->addWidget(groupBox,45);
	centerWidget->setLayout(mainLayout);

	connect(openAction, SIGNAL(triggered()), this, SLOT(openFolder()));
	connect(widgetAction, SIGNAL(triggered()), this, SLOT(openFolder()));
	connect(shapeWidgetAction, SIGNAL(triggered()), this, SLOT(grabResUpdated()));
	connect(shapeDrawWidgetAction, SIGNAL(triggered()), glWidget, SLOT(changeDrawShape()));
	connect(boxTestWidgetAction, SIGNAL(triggered()), glWidget, SLOT(boxTest()));
	connect(brutalFinishWidgetAction, SIGNAL(triggered()), rgbWidget, SLOT(brutalModeState()));
	connect(brutalModeWidgetAction, SIGNAL(triggered()), rgbWidget, SLOT(brutalModeSwitch()));
	//connect(rgbWidget, SIGNAL(grabCutResult()), this, SLOT(grabResUpdated(/*cv::Mat**/)/*grabCutResultUpdated(cv::Mat*)*/));

	//glWidget->grabResult = &(rgbWidget->gcapp.binMask);
	rgbWidget->setSlave(depthWidget);
    setMenuBar(menuBar);
	setCentralWidget(centerWidget);
	setWindowTitle(tr("Hello Kinect"));
	openFolder();
}

void MainWindow::grabResUpdated(/*cv::Mat**/){
	
	//glWidget->shapeDetect();
	//imwrite("back.bmp", rgbWidget->gcapp.binMask);

	//FileStorage fs("data0_drawer1.xml", FileStorage::WRITE);
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
//C:\Users\LeslieRong\Desktop\data0
	path = QString("C:\\Users\\LeslieRong\\Desktop\\data0");

	//QFileDialog* openFilePath = new QFileDialog(this, "Please choose a folder", "Folder");
	//openFilePath->setFileMode(QFileDialog::DirectoryOnly);
	//if (openFilePath->exec() == QDialog::Accepted)
	//{
	//	//code here£¡
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

	glWidget->m_count = 0;
	glWidget->m_data.resize(width * height * 8);

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
			
			glWidget->add(QVector3D(camX / 1000.0f, camY / 1000.0f, depth_img[y * width + x] / 1000.0f), QVector3D(0, -1, -1));
			glWidget->addTex(QVector2D((float)(x - 6) / (float)width, (float)y / (float)height));
		}
	}

	glWidget->rawPointCount = glWidget->m_count / 8;

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

