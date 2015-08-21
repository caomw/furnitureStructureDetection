#ifndef KINECTHELLO_H
#define KINECTHELLO_H

#include <QtWidgets/QMainWindow>
#include "ui_kinecthello.h"

class KinectHello : public QMainWindow
{
	Q_OBJECT

public:
	KinectHello(QWidget *parent = 0);
	~KinectHello();

private:
	Ui::KinectHelloClass ui;
};

#endif // KINECTHELLO_H
