#include "paintwidget.h"

cv::Mat* qimage_to_mat_cpy(QImage const &img, int format)
{
	Mat * s = new Mat(img.height(), img.width(), format,
		const_cast<uchar*>(img.bits()),
		img.bytesPerLine());
	return s;
}

PaintWidget::PaintWidget(QWidget *parent) :QWidget(parent), keyFlag(0), mouseButton(0)
{
	leftButtonPressed = false;
	drawable = false;
	rectState = NOT_SET;
	rectState = NOT_SET;
	lblsState = NOT_SET;
	prLblsState = NOT_SET;
	brutalMode = 1;
	brutalDone = false;
}

cv::Mat *qimage2mat(const QImage& qimage)
{
	cv::Mat mat = cv::Mat(qimage.height(), qimage.width(), CV_8UC4, (uchar*)qimage.bits(), qimage.bytesPerLine());
	cv::Mat* mat2 = new cv::Mat(mat.rows, mat.cols, CV_8UC3);
	int from_to[] = { 0, 0, 1, 1, 2, 2 };
	cv::mixChannels(&mat, 1, mat2, 1, from_to, 3);
	return mat2;
}

void PaintWidget::setImage(const QImage & img){
	m_image = img;
	ratio = m_image.width() / (float)widgetWidth;
	{
		const string winName = "image";
		gcapp.setImageAndWinName(qimage2mat(m_image), winName);

	}

	update();
}
void PaintWidget::setSlave(PaintWidget* pslave){
	slave = pslave;
}

void PaintWidget::setDepth(const QImage & img){
	m_depth = img;
	//gcapp.setImageAndWinName(qimage2mat(m_image), winName);
	gcapp.setDepth(qimage2mat(m_depth));

	update();
}

void PaintWidget::setDrawable(bool a){
	drawable = a;
}

void PaintWidget::setGrabCut(bool a){
	if (a)
		;
}

QSize PaintWidget::sizeHint() const
{
	if (m_image.isNull())
		return QSize(548, 411);
	else
		return QSize(m_image.width(), m_image.height());
}

QImage* mat2qimage(const cv::Mat& mat)
{
	cv::Mat rgb;
	cv::cvtColor(mat, rgb, CV_BGR2RGB);
	return new QImage((const unsigned char*)(rgb.data), rgb.cols, rgb.rows, QImage::Format_RGB888);
}

void PaintWidget::paintEvent(QPaintEvent *event)
{
	if (m_image.isNull())
		return;
	//this->resize(QSize(m_image.width(), m_image.height()));
	QPainter painter(this);

	// Copy input Mat
	const uchar *qImageBuffer = (const uchar*)gcapp.res.data;
	// Create QImage with same dimensions as input Mat
	QImage img(qImageBuffer, gcapp.res.cols, gcapp.res.rows, gcapp.res.step, QImage::Format_RGB888);
	painter.drawImage(QWidget::rect(), img.rgbSwapped());

	painter.setPen(QPen(Qt::green, 3));
	
	painter.drawRect(rect);


	painter.setPen(QPen(Qt::blue, 3));
	painter.drawPolyline(bgdPxls.data(), bgdPxls.size());
	for (size_t i = 0; i < bgdLine.size(); i++)
	{
		painter.drawPolyline(bgdLine.at(i).data(), bgdLine.at(i).size());
	}

	painter.setPen(QPen(Qt::red, 3));
	painter.drawPolyline(fgdPxls.data(), fgdPxls.size());
	for (size_t i = 0; i < fgdLine.size(); i++)
	{
		painter.drawPolyline(fgdLine.at(i).data(),fgdLine.at(i).size());
	}

	if (brutalMode){
		if (brutalDone)
			painter.drawPolygon(brutalPxls.data(), brutalPxls.size(), Qt::WindingFill);
		else
			painter.drawPolyline(brutalPxls.data(), brutalPxls.size());
	}

	painter.setPen(QPen(Qt::yellow, 4));
	for (size_t i = 0; i < brutalPxls.size(); i++)
	{
		painter.drawPoint(brutalPxls.at(i));
	}
}

void PaintWidget::resizeEvent(QResizeEvent * event){
	int w = event->size().width();
	int h = event->size().height();
	widgetWidth = w;
	widgetHeight = h;
	float pw = (float)w / m_image.width();
	float ph = (float)h / m_image.height();

	if (pw < ph){
		this->resize(pw * m_image.width(), pw * m_image.height());
		ratio = 1 / pw;
	}
	else{
		this->resize(ph * m_image.width(), ph * m_image.height());
		ratio = 1 / ph;
	}

}


void PaintWidget::setLblsInMask(int flags, QPoint p)
{
	vector<QPoint> *bpxls, *fpxls;
	{
		bpxls = &bgdPxls;
		fpxls = &fgdPxls;
	}

	if (flags & Qt::ControlModifier)
	{
		bpxls->push_back(p);
		slave->bgdPxls.push_back(p);
		if (isRelease)
		{
			bgdLine.push_back(bgdPxls);
			bpxls->clear();
			slave->bgdLine.push_back(bgdPxls);
			slave->bgdPxls.clear();
		}
	}
	if (flags & Qt::ShiftModifier)
	{
		fpxls->push_back(p);
		slave->fgdPxls.push_back(p);
		if (isRelease)
		{
			fgdLine.push_back(fgdPxls);
			fgdPxls.clear();
			slave->fgdLine.push_back(fgdPxls);
			slave->fgdPxls.clear();
		}

	}
}

void PaintWidget::mousePressEvent(QMouseEvent *event){
	if (drawable){
		leftButtonPressed = true;
		isRelease = false;
		if (event->buttons() & Qt::LeftButton){
			mouseButton = LEFTB;
			bool isb = (event->modifiers() & Qt::ControlModifier),
				isf = (event->modifiers() & Qt::ShiftModifier);
			gcapp.QMouseClick(CV_EVENT_LBUTTONDOWN, event->x()*ratio, event->y()*ratio, event->modifiers());
			if (brutalMode)
			{
				brutalPxls.push_back(QPoint(event->x(), event->y()));
				slave->brutalPxls.push_back(QPoint(event->x(), event->y()));
			}
			else{
			
				if (rectState == NOT_SET && !isb && !isf)
				{
					rectState = IN_PROCESS;
					rectLU = QPoint(event->x(), event->y());
					rect = QRect(event->x(), event->y(), 1, 1);
				}
				if ((isb || isf) && gcapp.rectState == gcapp.SET)
					lblsState = IN_PROCESS;
			}
		}
		if (event->buttons() & Qt::RightButton){
			mouseButton = RIGHTB;
			gcapp.QMouseClick(CV_EVENT_RBUTTONDOWN, event->x()*ratio, event->y()*ratio, event->modifiers());
			bool isb = (event->modifiers() & Qt::ControlModifier),
				isf = (event->modifiers() & Qt::ShiftModifier);
			if ((isb || isf) && rectState == SET)
				prLblsState = IN_PROCESS;
		}
	
	update();
	slave->update();
	}
}

void PaintWidget::mouseMoveEvent(QMouseEvent *event){

	if (event->buttons() & Qt::LeftButton) {
		if (drawable){
			//line->push_back(QPoint(event->x(), event->y()));
			//gcapp->fgdPxls.push_back(Point(event->x()*ratio, event->y()*ratio));
			//circle(gcapp->mask, Point(event->x()*ratio, event->y()*ratio), gcapp->radius, GC_FGD, gcapp->thickness);
			gcapp.QMouseClick(CV_EVENT_MOUSEMOVE, event->x()*ratio, event->y()*ratio, event->modifiers());

			if (rectState == IN_PROCESS)
			{
				rect = QRect(rectLU, QPoint(event->x(), event->y()));
				//assert(bgdPxls.empty() && fgdPxls.empty() && prBgdPxls.empty() && prFgdPxls.empty());
			}
			else if (lblsState == IN_PROCESS)
			{
				//				setLblsInMask(flags, Point(x, y), false);
				setLblsInMask(event->modifiers(), QPoint(event->x(), event->y()));
			}
			else if (prLblsState == IN_PROCESS)
			{
				//					setLblsInMask(flags, Point(x, y), true);
				setLblsInMask(event->modifiers(), QPoint(event->x(), event->y()));
			}

		}
	}
	//}
	if (drawable)
	{
		update();
		slave->update();
	}


}

void PaintWidget::mouseReleaseEvent(QMouseEvent *event){
	if (drawable)
	{
		leftButtonPressed = true;
		if (mouseButton == LEFTB){
			gcapp.QMouseClick(CV_EVENT_LBUTTONUP, event->x()*ratio, event->y()*ratio, event->modifiers());
			//rectState = gcapp.IN_PROCESS;
			if (rectState == IN_PROCESS)
			{
				//rect = Rect(Point(rect.x, rect.y), Point(x, y));
				rect = QRect(rectLU, QPoint(event->x(), event->y()));
				rectState = SET;
				slave->rect = rect;
			}
			if (lblsState == IN_PROCESS)
			{
				isRelease = true;
				setLblsInMask(event->modifiers(), QPoint(event->x(), event->y()));
				lblsState = SET;

			}

		}
		
		if (mouseButton == RIGHTB){
		
			gcapp.QMouseClick(CV_EVENT_RBUTTONUP, event->x()*ratio, event->y()*ratio, event->modifiers());
			if (prLblsState == IN_PROCESS)
			{
				setLblsInMask(event->modifiers(), QPoint(event->x(), event->y()));
				prLblsState = SET;
				//showImage();
			}
		}
		
		mouseButton = 0;

		update();
		slave->update();
	}


}

void PaintWidget::keyPressEvent(QKeyEvent * event){
	keyFlag = event->key();
}
void PaintWidget::keyReleaseEvent(QKeyEvent * event){
	keyFlag = 0;
}

void PaintWidget::grabCutIteration(){
	//gcapp.nextIterRaw();
	gcapp.nextIter();
	//gcapp.showImage();
	fgdPxls.clear();
	fgdLine.clear();
	bgdPxls.clear();
	bgdLine.clear();
	slave->fgdPxls.clear();
	slave->bgdPxls.clear();
	slave->bgdLine.clear();
	slave->fgdLine.clear();
	brutalPxls.clear();
	brutalDone = false;
	update();
	slave->update();
	this->update();
}

void PaintWidget::brutalModeState(){
	if (brutalDone){
		//brutalMode = 0;
		brutalDone = false;
		gcapp.brutalDone = false;
	}
	else
	{
		//brutalMode = 1;
		brutalDone = true;
		slave->brutalDone = true;
		gcapp.brutalDone = true;
		gcapp.brutalSelectDone();
	}
	update();
	slave->update();
}

void PaintWidget::brutalModeSwitch(){
	if (brutalMode)
	{
		brutalDone = false;
		brutalPxls.clear();
		gcapp.brutalPxls.clear();
		slave->brutalPxls.clear();
		rect = QRect(0, 0, 0, 0);
		gcapp.rect = Rect(0,0,0,0);
		rectState = NOT_SET;
		rectState = NOT_SET;
		lblsState = NOT_SET;
		prLblsState = NOT_SET;
		gcapp.reset();
		brutalMode = 0;
		gcapp.brutalMode = 0;	
	}
	else
	{
		brutalDone = false;
		rect = QRect(0, 0, 0, 0);
		slave->rect = QRect(0, 0, 0, 0);
		gcapp.rect = Rect(0, 0, 0, 0);
		fgdPxls.clear();
		bgdPxls.clear();
		slave->fgdPxls.clear();
		slave->bgdPxls.clear();
		gcapp.reset();
		brutalMode = 1;
		gcapp.brutalMode = 1;
	}
	gcapp.showImage();
	update();
	slave->update();
}