#ifndef IGMM_H
#define IGMM_H

typedef double t_color;
namespace am
{
    class Labels
    {
        public:
            static			 unsigned char background()                      { return 0;     }
            static			 unsigned char probably_background()             { return 1;     }
            static           unsigned char label         ( unsigned char i ) { return i*2;   }
            static           unsigned char probably_label( unsigned char i ) { return i*2+1; }
    };
}

template <int cn>
class GMM;

class IGMM
{
    public:
        const GMM<3>* AsGMM3Pointer() const { return reinterpret_cast<const GMM<3>*>(this); };
        const GMM<1>* AsGMM1Pointer() const { return reinterpret_cast<const GMM<1>*>(this); };
        const GMM<4>* AsGMM4Pointer() const { return reinterpret_cast<const GMM<4>*>(this); };
#if MULTI_LABEL & 0
    public:
        static void create( int cn, std::shared_ptr<IGMM> p_gmm, cv::Mat &model )
        {
            switch (cn) {
                case 1:
                    p_gmm = std::make_shared<GMM<1>>( model );
                    break;
                case 3:
                    p_gmm = std::make_shared<GMM<3>>( model );
                    break;
                default:
                    std::cerr << "IGMM.create(): unknown number of GMM channels requested..." << std::endl;
                    break;
            }
        }
#endif
};

#endif // IGMM_H
