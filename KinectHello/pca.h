#pragma once


namespace ldp
{
	/**
	* Estimate the principle axes of a given 2D dataset.
	* @n:[input]
	*	number of data points.
	* @data_xy: [input]
	*	x0, y0, x1, y1, ..., xn, yn
	* @axis_x: [output]
	*	the first principle axis.
	* @axis_y: [output]
	*	the second principle axis
	* */
	void pca2D(int n, const float* data_xy, float axis_x[2], float axis_y[2]);
}