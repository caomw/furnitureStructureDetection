#include <GMM2.h>

#include <iostream>
#include "opencv2/core/types_c.h"

#if MULTI_LABEL

using namespace cv;

GMM2::GMM2( cv::Mat& model, int cn )
    : channels_( cn )
{
    const int modelSize = channels_/*mean*/ + channels_*channels_/*covariance*/ + 1/*component weight*/;

    if ( model.empty() )
    {
        model.create( 1, modelSize*kComponentsCount, CV_64FC1 );
        model.setTo(Scalar(0));
    }
    else if (    (model.type() != CV_64FC1                  )
              || (model.rows   != 1                         )
              || (model.cols   != modelSize*kComponentsCount) )
    {
        CV_Error( CV_StsBadArg, "_model must have CV_64FC1 type, rows == 1 and cols == 13*componentsCount" );
    }

    model_   = model;

    p_coefs_ = model_.ptr<double>(0);
    p_means_ = p_coefs_ + kComponentsCount;
    p_cov_   = p_means_ + channels_ * kComponentsCount;


    for ( int ci = 0; ci < kComponentsCount; ++ci )
    {
        inverse_covs_[ci] = Eigen::MatrixXd( channels_, channels_ );
        prods_       [ci] = Eigen::MatrixXd( channels_, channels_ );
        sums_        [ci] = Eigen::VectorXd( channels_ );

        if ( p_coefs_[ci] > 0 )
            calcInverseCovAndDeterm( ci );
    }
}

double GMM2::operator()( const cv::Mat& color ) const
{
    double res = 0;
    for( int ci = 0; ci < kComponentsCount; ci++ )
        res += p_coefs_[ci] * (*this)(ci, color );
    return res;
}

double GMM2::operator()( int ci, const cv::Mat& color ) const
{
    double res = 0.;

    if ( !(p_coefs_[ci] > 0.) ) return res;

    if ( cov_determs_[ci] <= std::numeric_limits<double>::epsilon() )
        return res;
    //CV_Assert( cov_determs_[ci] > std::numeric_limits<double>::epsilon() );

    // prepare difference
    cv::Mat diff = color;
    // pointer to current component means
    double *p_current_mean = p_means_ + ci * channels_;

    // calculate diff
    {
        for ( int cn = 0; cn < channels_; ++cn )
            diff.at<t_color>(cn) -= p_current_mean[cn];
    }

    // multiply
    double mult = 0.;
    {
        for ( int cn = 0; cn < channels_; ++cn )
            for ( int cn2 = 0; cn2 < channels_; ++cn2 )
                mult += diff.at<t_color>(cn) * diff.at<t_color>(cn2) * inverse_covs_[ci](cn2, cn);
    }

//    double mult = diff[0]*(diff[0]*inverse_covs_[ci][0][0] + diff[1]*inverse_covs_[ci][1][0] + diff[2]*inverse_covs_[ci][2][0])
//            + diff[1]*(diff[0]*inverse_covs_[ci][0][1] + diff[1]*inverse_covs_[ci][1][1] + diff[2]*inverse_covs_[ci][2][1])
//            + diff[2]*(diff[0]*inverse_covs_[ci][0][2] + diff[1]*inverse_covs_[ci][1][2] + diff[2]*inverse_covs_[ci][2][2]);

    res = 1.0f / sqrt(cov_determs_[ci]) * exp(-0.5f*mult);
    return res;
}

int GMM2::whichComponent( const cv::Mat& color ) const
{
    int     ci_max  = 0;
    double  max_val = 0.;

    for ( int ci = 0; ci < kComponentsCount; ci++ )
    {
        double val = (*this)( ci, color );
        if ( val > max_val )
        {
            ci_max  = ci;
            max_val = val;
        }
    }

    return ci_max;
}

void GMM2::initLearning()
{
    for( int ci = 0; ci < kComponentsCount; ++ci )
    {
        for ( int cn = 0; cn < channels_; ++cn )
        {
            sums_[ci][cn] = 0.;
            for ( int cn2 = 0; cn2 < channels_; ++cn2 )
            {
                prods_[ci](cn,cn2) = 0.;
            }
        }
        sampleCounts_[ci] = 0;
    }
    totalSampleCount_ = 0;
}

void GMM2::addSample( int ci, const cv::Mat& color )
{
    for ( int cn = 0; cn < channels_; ++cn )
    {
        sums_[ci](cn) += color.at<t_color>(cn);
        for ( int cn2 = 0; cn2 < channels_; ++cn2 )
            prods_[ci](cn,cn2) += color.at<t_color>(cn) * color.at<t_color>(cn2);
    }
    //sums_[ci][0] += color[0]; sums_[ci][1] += color[1]; sums_[ci][2] += color[2];

    // prods_[ci][0][0] += color[0]*color[0]; prods_[ci][0][1] += color[0]*color[1]; prods_[ci][0][2] += color[0]*color[2];
    // prods_[ci][1][0] += color[1]*color[0]; prods_[ci][1][1] += color[1]*color[1]; prods_[ci][1][2] += color[1]*color[2];
    // prods_[ci][2][0] += color[2]*color[0]; prods_[ci][2][1] += color[2]*color[1]; prods_[ci][2][2] += color[2]*color[2];
    ++sampleCounts_[ci];
    ++totalSampleCount_;
}

void GMM2::endLearning()
{
    const double variance = 0.01;
    for ( int ci = 0; ci < kComponentsCount; ++ci )
    {
        int n = sampleCounts_[ ci ];
        if ( n == 0 )
            p_coefs_[ci] = 0;
        else
        {
            p_coefs_[ci] = (double)n / totalSampleCount_;

            double *p_current_mean = p_means_ + ci * channels_;
            for ( int cn = 0; cn < channels_; ++cn )
            {
                p_current_mean[cn] = sums_[ci][cn] / n; // p_current_mean[1] = sums_[ci][1]/n; p_current_mean[2] = sums_[ci][2]/n;
            }

            double *p_current_cov = p_cov_ + ci * channels_ * channels_;
            for ( int cn = 0; cn < channels_; ++cn )
                for ( int cn2 = 0; cn2 < channels_; ++cn2 )
                    p_current_cov[cn*channels_+cn2] = prods_[ci](cn,cn2) / n - p_current_mean[cn] * p_current_mean[cn2];

            // p_current_cov[0] = prods_[ci][0][0]/n - p_current_mean[0]*p_current_mean[0];
            // p_current_cov[1] = prods_[ci][0][1]/n - p_current_mean[0]*p_current_mean[1];
            // p_current_cov[2] = prods_[ci][0][2]/n - p_current_mean[0]*p_current_mean[2];
            // p_current_cov[3] = prods_[ci][1][0]/n - p_current_mean[1]*p_current_mean[0]; p_current_cov[4] = prods_[ci][1][1]/n - p_current_mean[1]*p_current_mean[1]; p_current_cov[5] = prods_[ci][1][2]/n - p_current_mean[1]*p_current_mean[2];
            // p_current_cov[6] = prods_[ci][2][0]/n - p_current_mean[2]*p_current_mean[0]; p_current_cov[7] = prods_[ci][2][1]/n - p_current_mean[2]*p_current_mean[1]; p_current_cov[8] = prods_[ci][2][2]/n - p_current_mean[2]*p_current_mean[2];
            if ( channels_ == 3 )
            {
                double dtrm = p_current_cov[0]*(p_current_cov[4]*p_current_cov[8]-p_current_cov[5]*p_current_cov[7])
                            - p_current_cov[1]*(p_current_cov[3]*p_current_cov[8]-p_current_cov[5]*p_current_cov[6])
                            + p_current_cov[2]*(p_current_cov[3]*p_current_cov[7]-p_current_cov[4]*p_current_cov[6]);
                std::cout << "dtrm: " << dtrm << std::endl;
                if ( dtrm <= std::numeric_limits<double>::epsilon() )
                {
                    // Adds the white noise to avoid singular covariance matrix.
                    p_current_cov[0] += variance;
                    p_current_cov[4] += variance;
                    p_current_cov[8] += variance;
                }
                calcInverseCovAndDeterm( ci );
            }
            else if ( channels_ == 1 )
            {
                calcInverseCovAndDeterm( ci );
            }
            else
            {
                std::cerr << "endLearning not implemented for cn != {1,3} !!!" << std::endl;
            }
        }
    }
}

void GMM2::calcInverseCovAndDeterm( int ci )
{
    if ( (channels_ != 1) && (channels_ != 3) )
        std::cerr << "calcInverseCovAndDeterm: channels_ ==  1 or 3 please!" << std::endl;

    if ( p_coefs_[ci] > 0 )
    {
        double *p_current_cov = p_cov_ + ci * channels_ * channels_;
        double dtrm = 0.;
        if ( channels_ == 3 )
        {
            dtrm = cov_determs_[ci] = p_current_cov[0] * (p_current_cov[4]*p_current_cov[8]-p_current_cov[5]*p_current_cov[7])
                                    - p_current_cov[1] * (p_current_cov[3]*p_current_cov[8]-p_current_cov[5]*p_current_cov[6])
                                    + p_current_cov[2] * (p_current_cov[3]*p_current_cov[7]-p_current_cov[4]*p_current_cov[6]);

            CV_Assert( dtrm > std::numeric_limits<double>::epsilon() );
            inverse_covs_[ci](0,0) =  (p_current_cov[4]*p_current_cov[8] - p_current_cov[5]*p_current_cov[7]) / dtrm;
            inverse_covs_[ci](1,0) = -(p_current_cov[3]*p_current_cov[8] - p_current_cov[5]*p_current_cov[6]) / dtrm;
            inverse_covs_[ci](2,0) =  (p_current_cov[3]*p_current_cov[7] - p_current_cov[4]*p_current_cov[6]) / dtrm;
            inverse_covs_[ci](0,1) = -(p_current_cov[1]*p_current_cov[8] - p_current_cov[2]*p_current_cov[7]) / dtrm;
            inverse_covs_[ci](1,1) =  (p_current_cov[0]*p_current_cov[8] - p_current_cov[2]*p_current_cov[6]) / dtrm;
            inverse_covs_[ci](2,1) = -(p_current_cov[0]*p_current_cov[7] - p_current_cov[1]*p_current_cov[6]) / dtrm;
            inverse_covs_[ci](0,2) =  (p_current_cov[1]*p_current_cov[5] - p_current_cov[2]*p_current_cov[4]) / dtrm;
            inverse_covs_[ci](1,2) = -(p_current_cov[0]*p_current_cov[5] - p_current_cov[2]*p_current_cov[3]) / dtrm;
            inverse_covs_[ci](2,2) =  (p_current_cov[0]*p_current_cov[4] - p_current_cov[1]*p_current_cov[3]) / dtrm;
        }
        else if ( channels_ == 1 )
        {
            dtrm = cov_determs_[ci] = p_current_cov[0];
            inverse_covs_[ci](0,0) = 1. / dtrm;
        }
        else
        {
             std::cerr << "calcInverseCovAndDeterm: channels_ ==  1 or 3 please!" << std::endl;
        }
    }
}

#endif
