#pragma once
#define USE_LMFIT

#ifdef USE_LMFIT
#include "lmcurve.h"
#else
#include "mpfit.h"
#endif

#include <math.h>
#include <string.h>
#include "globals.h"
using namespace std;

bool fitGaussian(int numVals, double* valsY, double initA, double initSigma, double initX_C, double* params, double* paramErrors, double& norm);
bool fitDoubleGaussian(int numVals, double* valsY, double initA1, double initSigma1, double initX_C1, double initA2, double initSigma2, double initX_C2, double* params, double& norm);
#ifdef USE_LMFIT
double doublegaussfunc(double t, const double* p);
double gaussfunc(double t, const double* p);
#else
int gaussfunc(int m, int n, double *p, double *dy, double **dvec, void *vars);
int doublegaussfunc(int m, int n, double *p, double *dy, double **dvec, void *vars);
#endif

void initialGuesses(int numBins, double* bins, double parameterMin, double parameterMax, double& A1, double& A2, double& mean1, double& mean2, double& sigma1, double& sigma2);