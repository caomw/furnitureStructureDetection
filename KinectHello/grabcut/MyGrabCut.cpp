/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

//#include "opencv2/imgproc/precomp.hpp"
#include "opencv2/core/types_c.h"
#include "gcgraph.hpp"
#include <limits>
#include "GMM.h"

using namespace cv;

namespace am
{

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


} // ns am
