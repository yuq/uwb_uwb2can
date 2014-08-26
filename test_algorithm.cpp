#include <iostream>
#include <cmath>

void get_position(double *p1, double *p2, double *p3, 
				  double *p4, double *p5, 
				  double d1, double d2, double d3, double d4, 
				  double d5, double *r);

static double get_distance(double *a, double *b)
{
	return sqrt((a[0] - b[0]) * (a[0] - b[0]) + 
				(a[1] - b[1]) * (a[1] - b[1]) + 
				(a[2] - b[2]) * (a[2] - b[2]));
}

int main(void)
{
	double p1[] = {0, 0, 0};
	double p2[] = {2, 0, 0};
	double p3[] = {0, 2, 0};
	double p4[] = {2, 2, 0};
	double p5[] = {1, 1, 1};
	double pm[] = {10, 23, 12};
	double r[3];
	get_position(p1, p2, p3, p4, p5, 
				 get_distance(p1, pm), get_distance(p2, pm), 
				 get_distance(p3, pm), get_distance(p4, pm), 
				 get_distance(p5, pm), r);
	std::cout << "x=" << r[0] << " y=" << r[1] << " z=" << r[2] << std::endl;
	return 0;
}
