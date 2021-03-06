#include "MultiLabelGrabCut.h"

#include "gcgraph.hpp"
#include <limits>
//#include "GMM.h"

using namespace cv;

namespace am
{
#if 0
    /*
  Check size, type and element values of mask matrix.
 */
    void checkMask( const Mat& img, const Mat& mask )
    {
        if( mask.empty()           )  CV_Error( CV_StsBadArg, "mask is empty" );
        if( mask.type() != CV_8UC1 )  CV_Error( CV_StsBadArg, "mask must have CV_8UC1 type" );
        if(    (mask.cols != img.cols)
            || (mask.rows != img.rows) )  CV_Error( CV_StsBadArg, "mask must have as many rows and cols as img" );

        for( int y = 0; y < mask.rows; y++ )
        {
            for( int x = 0; x < mask.cols; x++ )
            {
                uchar val = mask.at<uchar>(y,x);
                if( val!=GC_BGD && val!=GC_FGD && val!=GC_PR_BGD && val!=GC_PR_FGD )
                    CV_Error( CV_StsBadArg, "mask element value must be equal"
                              "GC_BGD or GC_FGD or GC_PR_BGD or GC_PR_FGD" );
            }
        }
    }

    /*
  Initialize mask using rectangular.
*/
    void initMaskWithRect( Mat& mask, Size imgSize, Rect rect )
    {
        mask.create( imgSize, CV_8UC1 );
        mask.setTo( GC_BGD );

        rect.x = std::max(0, rect.x);
        rect.y = std::max(0, rect.y);
        rect.width = std::min(rect.width, imgSize.width-rect.x);
        rect.height = std::min(rect.height, imgSize.height-rect.y);

        (mask(rect)).setTo( Scalar(GC_PR_FGD) );
    }

    /*
  Estimate segmentation using MaxFlow algorithm
*/
    void estimateSegmentation( GCGraph<double>& graph, Mat& mask )
    {
        graph.maxFlow();
        Point p;
        for( p.y = 0; p.y < mask.rows; p.y++ )
        {
            for( p.x = 0; p.x < mask.cols; p.x++ )
            {
                if( mask.at<uchar>(p) == GC_PR_BGD || mask.at<uchar>(p) == GC_PR_FGD )
                {
                    if( graph.inSourceSegment( p.y*mask.cols+p.x /*vertex index*/ ) )
                        mask.at<uchar>(p) = GC_PR_FGD;
                    else
                        mask.at<uchar>(p) = GC_PR_BGD;
                }
            }
        }
    }
#endif

} // ns am
