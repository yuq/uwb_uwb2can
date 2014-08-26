#include <math.h>
#include <string.h>

// solve:
//   (Xm - X1)^2 + (Ym - Y1)^2 + (Zm - Z1)^2 = D1^2  ---  (1)
//   (Xm - X2)^2 + (Ym - Y2)^2 + (Zm - Z2)^2 = D2^2  ---  (2)
//   (Xm - X3)^2 + (Ym - Y3)^2 + (Zm - Z3)^2 = D3^2  ---  (3)
//
// P1 != P2 != P3
static void solve(const double *p1, const double *p2, const double *p3, 
				  double d1, double d2, double d3, 
				  double *r1, double *r2)
{
	// STEP1
	// (1) - (2) \    / Xm = AZm + B  ---  (4)
	//            >--<
	// (1) - (3) /    \ Ym = CZm + D  ---  (5)
		
	// (1) - (2)
	// a1Xm + b1Ym + c1Zm = e1
	double a1 = 2 * (p2[0] - p1[0]);
	double b1 = 2 * (p2[1] - p1[1]);
	double c1 = 2 * (p2[2] - p1[2]);
	double e1 = d1 * d1 - d2 * d2 + p2[0] * p2[0] - 
		p1[0] * p1[0] + p2[1] * p2[1] - p1[1] * p1[1] + 
		p2[2] * p2[2] - p1[2] * p1[2];
		
	// (1) - (3)
	// a2Xm + b2Ym + c2Zm = e2
	double a2 = 2 * (p3[0] - p1[0]);
	double b2 = 2 * (p3[1] - p1[1]);
	double c2 = 2 * (p3[2] - p1[2]);
	double e2 = d1 * d1 - d3 * d3 + p3[0] * p3[0] - 
		p1[0] * p1[0] + p3[1] * p3[1] - p1[1] * p1[1] + 
		p3[2] * p3[2] - p1[2] * p1[2];
		
	// Xm = AZm + B
	double A = - (b1 * c2 - b2 * c1) / (b1 * a2 - b2 * a1);
	double B =   (b1 * e2 - b2 * e1) / (b1 * a2 - b2 * a1);
		
	// Ym = CZm + D
	double C = - (a1 * c2 - a2 * c1) / (a1 * b2 - a2 * b1);
	double D =   (a1 * e2 - a2 * e1) / (a1 * b2 - a2 * b1);
		
	// STEP2
	// (4) \
	//      >-> (3) -> Zm = R1/R2
	// (5) /
		
	// aZm^2 + bZm + c = 0
	double a = A * A + C * C + 1;
	double b = 2 * (A * (B - p3[0]) + C * (D - p3[1]) - p3[2]);
	double c = (B - p3[0]) * (B - p3[0]) + 
		(D - p3[1]) * (D - p3[1]) + p3[2] * p3[2] - d3 * d3;
		
	r1[2] = (-b + sqrt(b * b - 4 * a * c)) / (2 * a);
	r2[2] = (-b - sqrt(b * b - 4 * a * c)) / (2 * a);
		
	r1[0] = A * r1[2] + B;
	r2[0] = A * r2[2] + B;
		
	r1[1] = C * r1[2] + D;
	r2[1] = C * r2[2] + D;
}

// select from two answers according to a new point
static void select(const double *p, double d, const double *r1, const double *r2, double *r)
{
	double d1 = sqrt((r1[0] - p[0]) * (r1[0] - p[0]) + 
					 (r1[1] - p[1]) * (r1[1] - p[1]) + 
					 (r1[2] - p[2]) * (r1[2] - p[2]));
	double d2 = sqrt((r2[0] - p[0]) * (r2[0] - p[0]) + 
					 (r2[1] - p[1]) * (r2[1] - p[1]) + 
					 (r2[2] - p[2]) * (r2[2] - p[2]));
	if (fabs(d1 - d) < fabs(d2 - d))
		memcpy(r, r1, sizeof(double) * 3);
	else
		memcpy(r, r2, sizeof(double) * 3);
}

static void compute_position(const double *p1, const double *p2, const double *p3, 
							 const double *p4, double d1, double d2, 
							 double d3, double d4, double *r)
{
	double r1[3], r2[3];
	solve(p1, p2, p3, d1, d2, d3, r1, r2);
	select(p4, d4, r1, r2, r);
}

void get_position(double *p1, double *p2, double *p3, 
				  double *p4, double *p5, 
				  double d1, double d2, double d3, double d4, 
				  double d5, double *r)
{
	double *ps[] = {p1, p2, p3, p4, p5};
	double ds[] = {d1, d2, d3, d4, d5};
		
	for (int i = 3; i > 0; i--) {
		for (int j = 0; j < i; j++) {
			if (ds[j] > ds[j + 1]) {
				double td = ds[j];
				ds[j] = ds[j + 1];
				ds[j + 1] = td;
					
				double *tp = ps[j];
				ps[j] = ps[j + 1];
				ps[j + 1] = tp;
			}
		}
	}
		
	compute_position(ps[0], ps[1], p5, ps[2], ds[0], ds[1], d5, ds[2], r);
}
