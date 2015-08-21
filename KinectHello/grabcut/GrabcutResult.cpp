#include "GrabcutResult.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <iostream>

namespace am
{

    void GrabcutResult::empty()
    {
        masks_for_labels_.clear();
        img_ = cv::Mat();
        dep_ = cv::Mat();
    }

    void GrabcutResult::add( cv::Mat const& mat )
    {
        masks_for_labels_.push_back( mat );
    };

    void
    GrabcutResult::addForLabel( cv::Mat const& mat, unsigned label )
    {
        if ( masks_for_labels_.size() <= label )
            masks_for_labels_.resize( label + 1 );

        masks_for_labels_[ label ] = mat.clone();
    }

     cv::Mat const&
     GrabcutResult::get_mask_for_label( unsigned label ) const
     {
         if ( masks_for_labels_.size() <= label )
         {
             std::cerr << "[" << __func__ << "]: " << "this shouldn't happen...\n";
             static const cv::Mat tmp;
             return tmp;
         }
         return masks_for_labels_[ label ];
     }

     int
     GrabcutResult::getBinaryMaskForLabel( cv::Mat &masked_depth, unsigned label ) const
     {
         if ( masks_for_labels_.size() <= label )
         {
             std::cout << "GrabcutResult::getBinaryMaskForLabel: masks_for_labels_.size() <= label...exiting" << std::endl;
             masked_depth = cv::Mat();
             return EXIT_FAILURE;
         }

         cv::Mat const& mask = get_mask_for_label( label );

         masked_depth.create( mask.size(), mask.type() );
         masked_depth.setTo(0);
         cv::Mat tmp_mask;

         cv::compare( mask, cv::GC_PR_FGD, tmp_mask, cv::CMP_EQ );
         mask.copyTo( masked_depth, tmp_mask );

         cv::compare( mask, cv::GC_FGD, tmp_mask, cv::CMP_EQ );
         mask.copyTo( masked_depth, tmp_mask );

         return EXIT_SUCCESS;
     }

    void GrabcutResult::set_image( cv::Mat const& img ) { img_ = img; }
    void GrabcutResult::set_depth( cv::Mat const& dep ) { dep_ = dep; }
    cv::Mat const& GrabcutResult::get_depth() const
    {
        return dep_;
    }
    cv::Mat const& GrabcutResult::get_rgb() const
    {
        return img_;
    }


} // ns am
