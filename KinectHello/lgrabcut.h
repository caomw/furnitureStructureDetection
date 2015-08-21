#ifndef GRABCUT_H
#define GRABCUT_H

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
using namespace cv;

class GCApplication
{
public:
	enum{ NOT_SET = 0, IN_PROCESS = 1, SET = 2 };
	static const int radius = 2;
	static const int thickness = -1;

	void reset();
	void setImageAndWinName(const Mat* _image);
	void showImage() const;
	void mouseClick(int event, int x, int y, int flags, void* param);
	int nextIter();
	int getIterCount() const { return iterCount; }
	vector<Point> fgdPxls, bgdPxls, prFgdPxls, prBgdPxls;
	const Mat* image;
	Mat mask;
	uchar rectState, lblsState, prLblsState;
	Rect rect;
private:
	void setRectInMask();
	void setLblsInMask(int flags, Point p, bool isPr);

	const string* winName;
	Mat bgdModel, fgdModel;
	bool isInitialized;

	int iterCount;
};

#endif