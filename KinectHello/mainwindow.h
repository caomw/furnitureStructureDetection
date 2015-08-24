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

public slots:
void jointUpdate(std::vector<BoxJoint *> pJointList);
	void boxUpdate(std::vector<Box> pboxList);
	void jointDoubleClick(const QModelIndex&);
	void jointSliderUpdate(double, double, double);

private slots:
	void openFolder();
	void grabResUpdated();

private:
	QString path;

	//QImage rgbImage = QImage("image26.bmp");
	//QImage depthImage = QImage("test.jpg");
	PaintWidget * rgbWidget;
	PaintWidget * depthWidget;
	GLWidget *glWidget;

};

#endif
