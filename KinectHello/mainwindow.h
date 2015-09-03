#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include<stdio.h>
#include<stdlib.h>
#include<iostream>

#include <QMainWindow>
#include <QString>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QWidgetAction>
#include <QComboBox>
#include <QToolBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSize>
#include <QSizePolicy>
#include <QGroupBox>
#include <QSplitter>
#include <QAction>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QTreeView>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include "paintwidget.h"
#include "glwidget.h"
#include "opencv2/highgui/highgui.hpp"
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
	~MainWindow(){
		if (rgbImage)
			delete rgbImage;
		if (depthImage)
			delete depthImage;
		delete rgbWidget;
		delete depthWidget;
		delete glWidget;
	};
	QImage * rgbImage;
	QImage * depthImage;
	QStandardItemModel * jointStd;
	QStandardItemModel * boxStd;
	QTreeView * jointTree;
	QTreeView *boxTree;
	QSlider * jointSlider;
	QSlider * editSlider;
	QComboBox * parentBox;
	QComboBox * childBox;
	QComboBox * planeSelectBox;
	QComboBox * constraintBox;
	QCheckBox * vertexCheck[8];
	std::vector<int>* glSelectList;

signals:
	void vertexSelect(int,int);
	void planeSelect(int);
	void addConstraint(int);

public slots:
	void jointUpdate(std::vector<BoxJoint *> pJointList);
	void jointUpdate(std::vector<associateNode> pJointList);
	void boxUpdate(std::vector<Box> pboxList);
	void jointDoubleClick(const QModelIndex&);
	void jointSliderUpdate(double, double, double);
	void boxUpdate(int,int,int);
	void addConstraint();
	void editSliderReset();
	void glRightSelect(int);

private slots:
	void openFolder();
	void grabResUpdated();
	void checkCheck();
	void selectSubmit();
	void jointTreeRight(const QPoint);
	void boxTreeRight(const QPoint);

private:
	QString path;

	//QImage rgbImage = QImage("image26.bmp");
	//QImage depthImage = QImage("test.jpg");
	PaintWidget * rgbWidget;
	PaintWidget * depthWidget;
	GLWidget *glWidget;

};

#endif
