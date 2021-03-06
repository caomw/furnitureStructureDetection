#include "cvgrabcut.h"

//#include "gcgraph.hpp"
 /*************************** grabCut ******************************************
 
*/
/*
Calculate beta - parameter of GrabCut algorithm.
beta = 1/(2*avg(sqr(||color[i] - color[j]||)))
*/
static double calcBeta(const Mat& img)
{
	double beta = 0;
	for (int y = 0; y < img.rows; y++)
	{
		for (int x = 0; x < img.cols; x++)
		{
			Vec4d color = img.at<Vec4b>(y, x);
			if (x>0) // left
			{
				Vec4d diff = color - (Vec4d)img.at<Vec4b>(y, x - 1);
				beta += diff.dot(diff);
			}
			if (y>0 && x>0) // upleft
			{
				Vec4d diff = color - (Vec4d)img.at<Vec4b>(y - 1, x - 1);
				beta += diff.dot(diff);
			}
			if (y>0) // up
			{
				Vec4d diff = color - (Vec4d)img.at<Vec4b>(y - 1, x);
				beta += diff.dot(diff);
			}
			if (y>0 && x<img.cols - 1) // upright
			{
				Vec4d diff = color - (Vec4d)img.at<Vec4b>(y - 1, x + 1);
				beta += diff.dot(diff);
			}
		}
	}
	if (beta <= std::numeric_limits<double>::epsilon())
		beta = 0;
	else
		beta = 1.f / (2 * beta / (4 * img.cols*img.rows - 3 * img.cols - 3 * img.rows + 2));

	return beta;
}

/*
Calculate weights of noterminal vertices of graph.
beta and gamma - parameters of GrabCut algorithm.
*/
static void calcNWeights(const Mat& img, Mat& leftW, Mat& upleftW, Mat& upW, Mat& uprightW, double beta, double gamma)
{
	const double gammaDivSqrt2 = gamma / std::sqrt(2.0f);
	leftW.create(img.rows, img.cols, CV_64FC1);
	upleftW.create(img.rows, img.cols, CV_64FC1);
	upW.create(img.rows, img.cols, CV_64FC1);
	uprightW.create(img.rows, img.cols, CV_64FC1);
	for (int y = 0; y < img.rows; y++)
	{
		for (int x = 0; x < img.cols; x++)
		{
			Vec4d color = img.at<Vec4b>(y, x);
			if (x - 1 >= 0) // left
			{
				Vec4d diff = color - (Vec4d)img.at<Vec4b>(y, x - 1);
				leftW.at<double>(y, x) = gamma * exp(-beta*diff.dot(diff));
			}
			else
				leftW.at<double>(y, x) = 0;
			if (x - 1 >= 0 && y - 1 >= 0) // upleft
			{
				Vec4d diff = color - (Vec4d)img.at<Vec4b>(y - 1, x - 1);
				upleftW.at<double>(y, x) = gammaDivSqrt2 * exp(-beta*diff.dot(diff));
			}
			else
				upleftW.at<double>(y, x) = 0;
			if (y - 1 >= 0) // up
			{
				Vec4d diff = color - (Vec4d)img.at<Vec4b>(y - 1, x);
				upW.at<double>(y, x) = gamma * exp(-beta*diff.dot(diff));
			}
			else
				upW.at<double>(y, x) = 0;
			if (x + 1<img.cols && y - 1 >= 0) // upright
			{
				Vec4d diff = color - (Vec4d)img.at<Vec4b>(y - 1, x + 1);
				uprightW.at<double>(y, x) = gammaDivSqrt2 * exp(-beta*diff.dot(diff));
			}
			else
				uprightW.at<double>(y, x) = 0;
		}
	}
}

/*
Check size, type and element values of mask matrix.
*/
static void checkMask(const Mat& img, const Mat& mask)
{
	if (mask.empty())
		CV_Error(CV_StsBadArg, "mask is empty");
	if (mask.type() != CV_8UC1)
		CV_Error(CV_StsBadArg, "mask must have CV_8UC1 type");
	if (mask.cols != img.cols || mask.rows != img.rows)
		CV_Error(CV_StsBadArg, "mask must have as many rows and cols as img");
	for (int y = 0; y < mask.rows; y++)
	{
		for (int x = 0; x < mask.cols; x++)
		{
			uchar val = mask.at<uchar>(y, x);
			if (val != GC_BGD && val != GC_FGD && val != GC_PR_BGD && val != GC_PR_FGD)
				CV_Error(CV_StsBadArg, "mask element value must be equel"
				"GC_BGD or GC_FGD or GC_PR_BGD or GC_PR_FGD");
		}
	}
}

/*
Initialize mask using rectangular.
*/
static void initMaskWithRect(Mat& mask, Size imgSize, Rect rect)
{
	mask.create(imgSize, CV_8UC1);
	mask.setTo(GC_BGD);

	rect.x = std::max(0, rect.x);
	rect.y = std::max(0, rect.y);
	rect.width = std::min(rect.width, imgSize.width - rect.x);
	rect.height = std::min(rect.height, imgSize.height - rect.y);

	(mask(rect)).setTo(Scalar(GC_PR_FGD));
}

/*
Initialize GMM background and foreground models using kmeans algorithm.
*/
static void initGMMs(const Mat& img, const Mat& mask, GMM2& bgdGMM, GMM2& fgdGMM)
{
	const int kMeansItCount = 10;
	const int kMeansType = KMEANS_PP_CENTERS;

	Mat bgdLabels, fgdLabels;
	std::vector<Vec4f> bgdSamples, fgdSamples;
	Point p;
	for (p.y = 0; p.y < img.rows; p.y++)
	{
		for (p.x = 0; p.x < img.cols; p.x++)
		{
			if (mask.at<uchar>(p) == GC_BGD || mask.at<uchar>(p) == GC_PR_BGD)
				bgdSamples.push_back((Vec4f)img.at<Vec4b>(p));
			else // GC_FGD | GC_PR_FGD
				fgdSamples.push_back((Vec4f)img.at<Vec4b>(p));
		}
	}
	CV_Assert(!bgdSamples.empty() && !fgdSamples.empty());
	Mat _bgdSamples((int)bgdSamples.size(), 3, CV_32FC1, &bgdSamples[0][0]);
	kmeans(_bgdSamples, GMM2::kComponentsCount, bgdLabels,
		TermCriteria(CV_TERMCRIT_ITER, kMeansItCount, 0.0), 0, kMeansType);
	Mat _fgdSamples((int)fgdSamples.size(), 3, CV_32FC1, &fgdSamples[0][0]);
	kmeans(_fgdSamples, GMM2::kComponentsCount, fgdLabels,
		TermCriteria(CV_TERMCRIT_ITER, kMeansItCount, 0.0), 0, kMeansType);

	bgdGMM.initLearning();
	for (int i = 0; i < (int)bgdSamples.size(); i++)
		bgdGMM.addSample(bgdLabels.at<int>(i, 0), bgdSamples[i]);
	bgdGMM.endLearning();

	fgdGMM.initLearning();
	for (int i = 0; i < (int)fgdSamples.size(); i++)
		fgdGMM.addSample(fgdLabels.at<int>(i, 0), fgdSamples[i]);
	fgdGMM.endLearning();
}

/*
Assign GMMs components for each pixel.
*/
static void assignGMMsComponents(const Mat& img, const Mat& mask, const GMM2& bgdGMM, const GMM2& fgdGMM, Mat& compIdxs)
{
	Point p;
	for (p.y = 0; p.y < img.rows; p.y++)
	{
		for (p.x = 0; p.x < img.cols; p.x++)
		{
			Vec4d color = img.at<Vec4b>(p);
			compIdxs.at<int>(p) = mask.at<uchar>(p) == GC_BGD || mask.at<uchar>(p) == GC_PR_BGD ?
				bgdGMM.whichComponent(color) : fgdGMM.whichComponent(color);
		}
	}
}

/*
Learn GMMs parameters.
*/
static void learnGMMs(const Mat& img, const Mat& mask, const Mat& compIdxs, GMM2& bgdGMM, GMM2& fgdGMM)
{
	bgdGMM.initLearning();
	fgdGMM.initLearning();
	Point p;
	for (int ci = 0; ci < GMM2::kComponentsCount; ci++)
	{
		for (p.y = 0; p.y < img.rows; p.y++)
		{
			for (p.x = 0; p.x < img.cols; p.x++)
			{
				if (compIdxs.at<int>(p) == ci)
				{
					if (mask.at<uchar>(p) == GC_BGD || mask.at<uchar>(p) == GC_PR_BGD)
						bgdGMM.addSample(ci, img.at<Vec4b>(p));
					else
						fgdGMM.addSample(ci, img.at<Vec4b>(p));
				}
			}
		}
	}
	bgdGMM.endLearning();
	fgdGMM.endLearning();
}

/*
Construct GCGraph
*/
static void constructGCGraph(const Mat& img, const Mat& mask, const GMM2& bgdGMM, const GMM2& fgdGMM, double lambda,
	const Mat& leftW, const Mat& upleftW, const Mat& upW, const Mat& uprightW,
	GCGraph<double>& graph)
{
	int vtxCount = img.cols*img.rows,
		edgeCount = 2 * (4 * img.cols*img.rows - 3 * (img.cols + img.rows) + 2);
	graph.create(vtxCount, edgeCount);
	Point p;
	for (p.y = 0; p.y < img.rows; p.y++)
	{
		for (p.x = 0; p.x < img.cols; p.x++)
		{
			// add node
			int vtxIdx = graph.addVtx();
			Vec4b color = img.at<Vec4b>(p);

			// set t-weights
			double fromSource, toSink;
			if (mask.at<uchar>(p) == GC_PR_BGD || mask.at<uchar>(p) == GC_PR_FGD)
			{
				fromSource = -log(bgdGMM(color));
				toSink = -log(fgdGMM(color));
			}
			else if (mask.at<uchar>(p) == GC_BGD)
			{
				fromSource = 0;
				toSink = lambda;
			}
			else // GC_FGD
			{
				fromSource = lambda;
				toSink = 0;
			}
			graph.addTermWeights(vtxIdx, fromSource, toSink);

			// set n-weights
			if (p.x>0)
			{
				double w = leftW.at<double>(p);
				graph.addEdges(vtxIdx, vtxIdx - 1, w, w);
			}
			if (p.x>0 && p.y>0)
			{
				double w = upleftW.at<double>(p);
				graph.addEdges(vtxIdx, vtxIdx - img.cols - 1, w, w);
			}
			if (p.y>0)
			{
				double w = upW.at<double>(p);
				graph.addEdges(vtxIdx, vtxIdx - img.cols, w, w);
			}
			if (p.x<img.cols - 1 && p.y>0)
			{
				double w = uprightW.at<double>(p);
				graph.addEdges(vtxIdx, vtxIdx - img.cols + 1, w, w);
			}
		}
	}
}

/*
Estimate segmentation using MaxFlow algorithm
*/
static void estimateSegmentation(GCGraph<double>& graph, Mat& mask)
{
	graph.maxFlow();
	Point p;
	for (p.y = 0; p.y < mask.rows; p.y++)
	{
		for (p.x = 0; p.x < mask.cols; p.x++)
		{
			if (mask.at<uchar>(p) == GC_PR_BGD || mask.at<uchar>(p) == GC_PR_FGD)
			{
				if (graph.inSourceSegment(p.y*mask.cols + p.x /*vertex index*/))
					mask.at<uchar>(p) = GC_PR_FGD;
				else
					mask.at<uchar>(p) = GC_PR_BGD;
			}
		}
	}
}

void cv::grabCut(InputArray _img, InputOutputArray _mask, Rect rect,
	InputOutputArray _bgdModel, InputOutputArray _fgdModel,
	int iterCount, int mode)
{
	Mat img = _img.getMat();
	Mat& mask = _mask.getMatRef();
	Mat& bgdModel = _bgdModel.getMatRef();
	Mat& fgdModel = _fgdModel.getMatRef();

	if (img.empty())
		CV_Error(CV_StsBadArg, "image is empty");
	if (img.type() != CV_8UC3)
		CV_Error(CV_StsBadArg, "image mush have CV_8UC3 type");

	GMM bgdGMM(bgdModel), fgdGMM(fgdModel);
	Mat compIdxs(img.size(), CV_32SC1);

	if (mode == GC_INIT_WITH_RECT || mode == GC_INIT_WITH_MASK)
	{
		if (mode == GC_INIT_WITH_RECT)
			initMaskWithRect(mask, img.size(), rect);
		else // flag == GC_INIT_WITH_MASK
			checkMask(img, mask);
		initGMMs(img, mask, bgdGMM, fgdGMM);
	}

	if (iterCount <= 0)
		return;

	if (mode == GC_EVAL)
		checkMask(img, mask);

	const double gamma = 50;
	const double lambda = 9 * gamma;
	const double beta = calcBeta(img);

	Mat leftW, upleftW, upW, uprightW;
	calcNWeights(img, leftW, upleftW, upW, uprightW, beta, gamma);

	for (int i = 0; i < iterCount; i++)
	{
		GCGraph<double> graph;
		assignGMMsComponents(img, mask, bgdGMM, fgdGMM, compIdxs);
		learnGMMs(img, mask, compIdxs, bgdGMM, fgdGMM);
		constructGCGraph(img, mask, bgdGMM, fgdGMM, lambda, leftW, upleftW, upW, uprightW, graph);
		estimateSegmentation(graph, mask);
	}
}

/*
 ******************************************************************************/

void GCApplication::reset()
{
	if (!mask.empty())
		mask.setTo(Scalar::all(GC_BGD));
	bgdPxls.clear(); fgdPxls.clear();
	prBgdPxls.clear();  prFgdPxls.clear();

	isInitialized = false;
	rectState = NOT_SET;
	lblsState = NOT_SET;
	prLblsState = NOT_SET;
	iterCount = 0;
}

void GCApplication::setImageAndWinName(const Mat* _image, const string& _winName)
{
	//if (_image.empty() || _winName.empty())
	//if (_image.empty())
	//	return;
	image = _image;
	image->copyTo(res);
	winName = &_winName;
	mask.create(image->size(), CV_8UC1);
	reset();
}

//void GCApplication::showImage()
//{
//	//if (image->empty() || winName->empty())
//	if (image->empty())
//		return;
//
//	//Mat* res = &(this->res);
//	Mat binMask;
//	if (!isInitialized)
//		image->copyTo(res);
//	else
//	{
//		getBinMask(mask, binMask);
//		image->copyTo(res, binMask);
//	}
//
//	vector<Point>::const_iterator it;
//	for (it = bgdPxls.begin(); it != bgdPxls.end(); ++it)
//		circle(res, *it, radius, BLUE, thickness);
//	for (it = fgdPxls.begin(); it != fgdPxls.end(); ++it)
//		circle(res, *it, radius, RED, thickness);
//	for (it = prBgdPxls.begin(); it != prBgdPxls.end(); ++it)
//		circle(res, *it, radius, LIGHTBLUE, thickness);
//	for (it = prFgdPxls.begin(); it != prFgdPxls.end(); ++it)
//		circle(res, *it, radius, PINK, thickness);
//
//	if (rectState == IN_PROCESS || rectState == SET)
//		rectangle(res, Point(rect.x, rect.y), Point(rect.x + rect.width, rect.y + rect.height), GREEN, 2);
//	//imshow("haha", res);
//	//this->res = res;
//	//res.copyTo(this->res);
//	//cv::imwrite("res.jpg", res);
//	//if (qRes)
//	//	qRes->~QImage();
//	//mat2qimagep(res, &qRes);
//
//}

void GCApplication::showImage()
{
	if (image->empty() || winName->empty())
		return;

	Mat res;
	Mat binMask;
	if (!isInitialized)
		image->copyTo(res);
	else
	{
		getBinMask(mask, binMask);
		image->copyTo(res, binMask);
	}

	vector<Point>::const_iterator it;
	for (it = bgdPxls.begin(); it != bgdPxls.end(); ++it)
		circle(res, *it, radius, BLUE, thickness);
	for (it = fgdPxls.begin(); it != fgdPxls.end(); ++it)
		circle(res, *it, radius, RED, thickness);
	for (it = prBgdPxls.begin(); it != prBgdPxls.end(); ++it)
		circle(res, *it, radius, LIGHTBLUE, thickness);
	for (it = prFgdPxls.begin(); it != prFgdPxls.end(); ++it)
		circle(res, *it, radius, PINK, thickness);

	if (rectState == IN_PROCESS || rectState == SET)
		rectangle(res, Point(rect.x, rect.y), Point(rect.x + rect.width, rect.y + rect.height), GREEN, 2);

	res.copyTo(this->res);
}

void GCApplication::setRectInMask()
{
	assert(!mask.empty());
	mask.setTo(GC_BGD);
	rect.x = max(0, rect.x);
	rect.y = max(0, rect.y);
	rect.width = min(rect.width, image->cols - rect.x);
	rect.height = min(rect.height, image->rows - rect.y);
	(mask(rect)).setTo(Scalar(GC_PR_FGD));
}

void GCApplication::setLblsInMask(int flags, Point p, bool isPr)
{
	vector<Point> *bpxls, *fpxls;
	uchar bvalue, fvalue;
	if (!isPr)
	{
		bpxls = &bgdPxls;
		fpxls = &fgdPxls;
		bvalue = GC_BGD;
		fvalue = GC_FGD;
	}
	else
	{
		bpxls = &prBgdPxls;
		fpxls = &prFgdPxls;
		bvalue = GC_PR_BGD;
		fvalue = GC_PR_FGD;
	}
	//if (flags & BGD_KEY)
	if (flags & Qt::ControlModifier)
	{
		bpxls->push_back(p);
		circle(mask, p, radius, bvalue, thickness);
	}
	if (flags & Qt::ShiftModifier)
	{
		fpxls->push_back(p);
		circle(mask, p, radius, fvalue, thickness);
	}
}

void GCApplication::QMouseClick(int event, int x, int y, int flags)
{
	// TODO add bad args check
	switch (event)
	{
	case CV_EVENT_LBUTTONDOWN: // set rect or GC_BGD(GC_FGD) labels
	{
		bool isb = (flags & Qt::ControlModifier),
			isf = (flags & Qt::ShiftModifier);
		if (rectState == NOT_SET && !isb && !isf)
		{
			rectState = IN_PROCESS;
			rect = Rect(x, y, 1, 1);
		}
		if ((isb || isf) && rectState == SET)
			lblsState = IN_PROCESS;
	}
	break;
	case CV_EVENT_RBUTTONDOWN: // set GC_PR_BGD(GC_PR_FGD) labels
	{
		bool isb = (flags & BGD_KEY) != 0,
			isf = (flags & FGD_KEY) != 0;
		if ((isb || isf) && rectState == SET)
			prLblsState = IN_PROCESS;
	}
	break;
	case CV_EVENT_LBUTTONUP:
		if (rectState == IN_PROCESS)
		{
			rect = Rect(Point(rect.x, rect.y), Point(x, y));
			rectState = SET;
			setRectInMask();
			assert(bgdPxls.empty() && fgdPxls.empty() && prBgdPxls.empty() && prFgdPxls.empty());
			showImage();
		}
		if (lblsState == IN_PROCESS)
		{
			setLblsInMask(flags, Point(x, y), false);
			lblsState = SET;
			showImage();
		}
		break;
	case CV_EVENT_RBUTTONUP:
		if (prLblsState == IN_PROCESS)
		{
			setLblsInMask(flags, Point(x, y), true);
			prLblsState = SET;
			showImage();
		}
		break;
	case CV_EVENT_MOUSEMOVE:
		if (rectState == IN_PROCESS)
		{
			rect = Rect(Point(rect.x, rect.y), Point(x, y));
			assert(bgdPxls.empty() && fgdPxls.empty() && prBgdPxls.empty() && prFgdPxls.empty());
			showImage();
		}
		else if (lblsState == IN_PROCESS)
		{
			setLblsInMask(flags, Point(x, y), false);
			showImage();
		}
		else if (prLblsState == IN_PROCESS)
		{
			setLblsInMask(flags, Point(x, y), true);
			showImage();
		}
		break;
	}
}

int GCApplication::nextIter()
{
	if (isInitialized)
		grabCut(*image, mask, rect, bgdModel, fgdModel, 1);
	else
	{
		if (rectState != SET)
			return iterCount;

		if (lblsState == SET || prLblsState == SET)
			grabCut(*image, mask, rect, bgdModel, fgdModel, 1, GC_INIT_WITH_MASK);
		else
			grabCut(*image, mask, rect, bgdModel, fgdModel, 1, GC_INIT_WITH_RECT);

		isInitialized = true;
	}
	iterCount++;

	bgdPxls.clear(); fgdPxls.clear();
	prBgdPxls.clear(); prFgdPxls.clear();

	return iterCount;
}