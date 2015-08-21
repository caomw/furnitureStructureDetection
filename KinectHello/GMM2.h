#ifndef GMM2_H
#define GMM2_H

#include "IGMM.h"
#include <opencv2/core/core.hpp>
#include <eigen3/Eigen/Dense>

//#if MULTI_LABEL

/*
 GMM2 - Gaussian Mixture Model
*/
class GMM2 //: public IGMM
{
    public:
        static const int kComponentsCount = 5;

        GMM2( cv::Mat& model, int cn );
        double operator()( const cv::Mat& color ) const;
        double operator()( int ci, const cv::Mat& color ) const;
        int whichComponent( const cv::Mat& color ) const;

        void initLearning();
        void addSample( int ci, const cv::Mat& color );
        void endLearning();

        int get_channels() const { return channels_; }
        const int get_components_count() const { return kComponentsCount; }

    private:
        void calcInverseCovAndDeterm( int ci );
        cv::Mat model_;
        double* p_coefs_;
        double* p_means_;
        double* p_cov_;
        const int channels_;

        Eigen::MatrixXd  inverse_covs_[kComponentsCount]; // [channels_][channels_];
        double  cov_determs_ [kComponentsCount];

        Eigen::VectorXd  sums_           [kComponentsCount]; // [channels_];
        Eigen::MatrixXd  prods_          [kComponentsCount]; // [channels_][channels_];
        int     sampleCounts_   [kComponentsCount];
        int     totalSampleCount_;


};
//#endif // if MULTI_LABEL

#endif // GMM2_H
