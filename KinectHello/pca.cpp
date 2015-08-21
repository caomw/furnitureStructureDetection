#include "pca.h"
#include "eigen3\eigen\Dense"
#include <vector>

namespace ldp
{
	void pca2D(int n, const float* data_xy, float axis_x[2], float axis_y[2])
	{
		axis_x[0] = 0.f;
		axis_x[1] = 0.f;
		axis_y[0] = 0.f;
		axis_y[1] = 0.f;

		// trivial case
		if (n == 0)
			return;

		// compute mean
		float mean_xy[2] = { 0.f, 0.f };
		for (int i = 0; i < n; i++)
		{
			mean_xy[0] += data_xy[i * 2];
			mean_xy[1] += data_xy[i * 2 + 1];
		}
		mean_xy[0] /= n;
		mean_xy[1] /= n;

		// compute covariance
		Eigen::Matrix2f C = Eigen::Matrix2f::Zero();
		for (int i = 0; i < n; i++)
		{
			float x = data_xy[i * 2] - mean_xy[0];
			float y = data_xy[i * 2 + 1] - mean_xy[1];
			C(0, 0) += x*x;
			C(1, 0) += x*y;
			C(0, 1) += y*x;
			C(1, 1) += y*y;
		}

		// compute eigen vectors
		Eigen::SelfAdjointEigenSolver<Eigen::Matrix2f> solver(C);
		Eigen::Matrix2f V = solver.eigenvectors();

		// output
		axis_x[0] = V(0, 1);
		axis_x[1] = V(1, 1);
		axis_y[0] = V(0, 0);
		axis_y[1] = V(1, 0);
	}
}