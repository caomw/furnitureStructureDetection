#ifndef PAINTWIDGET_H
#define PAINTWIDGET_H

#include <QWidget>
#include <QPaintEvent>
#include <QImage>
#include <QPainter>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QPoint>
#include <QRect>
//#include "cvgrabcut.h"
#include "grabcutcv.h"

#define LEFTB 1
#define RIGHTB 2

class PaintWidget :public QWidget{
	Q_OBJECT
public:
	PaintWidget(QWidget * parent = 0);
	void setImage(const QImage &img);
	void setDepth(const QImage &img);
	QSize sizeHint() const Q_DECL_OVERRIDE;
	void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	void keyPressEvent(QKeyEvent * event) Q_DECL_OVERRIDE;
	void keyReleaseEvent(QKeyEvent * event) Q_DECL_OVERRIDE;
	void setDrawable(bool);
	void setGrabCut(bool);
	void setSlave(PaintWidget* pslave);
	GCApplication gcapp;
	int keyFlag;
	int brutalMode;
	bool brutalDone;
	QRect rect;
	QPoint rectLU;
	enum{ NOT_SET = 0, IN_PROCESS = 1, SET = 2 };
	uchar rectState, lblsState, prLblsState;
	vector<QPoint> fgdPxls, bgdPxls, brutalPxls;
	vector<vector<QPoint>> fgdLine, bgdLine;
	void setLblsInMask(int flags, QPoint p);
	PaintWidget* slave;
	bool isRelease;

signals:
	void grabCutResult();

public slots:
	void grabCutIteration();
	void brutalModeState();
	void brutalModeSwitch();
protected:
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent * event);

private:
	QImage m_image;
	QImage m_depth;
	QVector<QPoint> *line;
	QVector<QVector<QPoint> *> lineList;
	bool leftButtonPressed;
	bool drawable;
	float ratio;
	int widgetWidth;
	int widgetHeight;
	int mouseButton;


};

#endif