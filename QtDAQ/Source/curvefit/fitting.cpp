#include "fitting.h"


bool fitGaussian(int numVals, double* valsY, double initA, double initSigma, double initX_C, double* params, double* paramErrors, double& norm)
{
	double p[]={initA, initSigma, initX_C};
	int status;
#ifdef USE_LMFIT
	double* valsX=new double[numVals];
	for (int i=0;i<numVals;i++)
		valsX[i]=i;

	lm_control_struct control = lm_control_double;
    lm_status_struct stat;
    control.verbosity = 0;
	lmcurve(3, p, numVals, valsX, valsY, gaussfunc, &control, &stat);
	norm=stat.fnorm;
	delete[] valsX;
#else
	mp_config config;
	memset(&config, 0, sizeof(config));
	config.maxiter = 100;

	mp_result result;
	memset(&result,0,sizeof(result));       /* Zero results structure */
	result.xerror = paramErrors;

	mp_par    pars[3];
	memset(&pars[0], 0, sizeof(pars));
	pars[0].limited[0] = 1;    /* limited[0] indicates lower bound */
	pars[0].limits[0]  = 0.0; /* Actual constraint p[0]>= 0 */
	pars[1].limited[0] = 1;    /* limited[0] indicates lower bound */
	pars[1].limits[0]  = 0.0; /* Actual constraint p[0]>= 0 */

	status = mpfit(gaussfunc, numVals, 3, p, pars, &config, (void *)valsY, &result);
#endif
	params[0]=p[0];
	params[1]=p[1];
	params[2]=p[2];

	return true;
}

bool fitDoubleGaussian(int numVals, double* valsY, double initA1, double initSigma1, double initX_C1, double initA2, double initSigma2, double initX_C2, double* params, double& norm)
{
	double p[]={initA1, initSigma1, initX_C1, initA2, initSigma2, initX_C2};
	int status;
	bool retVal=true;
#ifdef USE_LMFIT
	double* valsX=new double[numVals];
	for (int i=0;i<numVals;i++)
		valsX[i]=i;

	lm_control_struct control = lm_control_double;
    lm_status_struct stat;
    control.verbosity = 0;
	lmcurve(6, p, numVals, valsX, valsY, doublegaussfunc, &control, &stat);
	norm=stat.fnorm;
	retVal=(stat.outcome>=1 && stat.outcome<=3);
	delete[] valsX;
#else	
	mp_config config;
	memset(&config, 0, sizeof(config));
	config.maxiter=100;
	mp_result result;
	memset(&result,0,sizeof(result));       //Zero results structure

	mp_par    pars[6];
	memset(&pars[0], 0, sizeof(pars));
	pars[0].limited[0] = 1;    // limited[0] indicates lower bound 
	pars[0].limits[0]  = 0.0; // Actual constraint p[0]>= 0 
	pars[1].limited[0] = 1;    // limited[0] indicates lower bound 
	pars[1].limits[0]  = 0.0; // Actual constraint p[0]>= 0 
	pars[3].limited[0] = 1;    // limited[0] indicates lower bound 
	pars[3].limits[0]  = 0.0; // Actual constraint p[0]>= 0 
	pars[4].limited[0] = 1;    // limited[0] indicates lower bound 
	pars[4].limits[0]  = 0.0; // Actual constraint p[0]>= 0 	

	status = mpfit(doublegaussfunc, numVals, 6, p, pars, &config, (void *)valsY, &result);
#endif
	params[0]=p[0];
	params[1]=p[1];
	params[2]=p[2];
	params[3]=p[3];
	params[4]=p[4];
	params[5]=p[5];
	return retVal;
}
#ifdef USE_LMFIT
double gaussfunc(double t, const double* p)
{
	double A1, sigma1, x_c1;
	A1=p[0];
	sigma1=p[1];
	x_c1=p[2];
	
	double f1 = abs(A1)*exp(-0.5*pow((t-x_c1)/sigma1, 2));

	return f1;

}
#else
int gaussfunc(int m, int n, double *p, double *dy, double **dvec, void *vars)
{
	int i;
	double A, sigma, x_c;
	A=p[0];	
	sigma=p[1];
	x_c=p[2];
	double* y=(double*)vars;

	for (i=0; i<m; i++)
	{
		double f = A*exp(-0.5*pow((i-x_c)/sigma, 2));
		dy[i]=(y[i]-f);
	}

	return 0;
}
#endif

#ifdef USE_LMFIT
double doublegaussfunc(double t, const double* p)
{
	double A1, sigma1, x_c1;
	double A2, sigma2, x_c2;
	A1=p[0];
	sigma1=p[1];
	x_c1=p[2];

	A2=p[3];
	sigma2=p[4];
	x_c2=p[5];

	double f1 = abs(A1)*exp(-0.5*pow((t-x_c1)/sigma1, 2));
	double f2 = abs(A2)*exp(-0.5*pow((t-x_c2)/sigma2, 2));

	return f1+f2;

}
#else
int doublegaussfunc(int m, int n, double *p, double *dy, double **dvec, void *vars)
{
	int i;
	double A1, sigma1, x_c1;
	double A2, sigma2, x_c2;
	A1=p[0];
	sigma1=p[1];
	x_c1=p[2];

	A2=p[3];
	sigma2=p[4];
	x_c2=p[5];

	double* y=(double*)vars;

	for (i=0; i<m; i++)
	{
		double f1 = A1*exp(-0.5*pow((i-x_c1)/sigma1, 2));
		double f2 = A2*exp(-0.5*pow((i-x_c2)/sigma2, 2));
		dy[i]=(y[i]-(f1+f2));
	}

	return 0;
}
#endif

void initialGuesses(int numBins, double* bins, double parameterMin, double parameterMax, double& A1, double& A2, double& mean1, double& mean2, double& sigma1, double& sigma2)
{
	if (numBins)
	{
		double maxValue=0;
		double FWHM;
		int totalEntries=0;
		double entrySum=0;
		double entryVal=parameterMin;
		double interval=(double)((parameterMax-parameterMin))/numBins;
		int indexMax=numBins-1;
		for (int i=0;i<numBins;i++)
		{
			if (bins[i]>maxValue)
			{
				indexMax=i;
				maxValue=bins[i];
			}
			totalEntries+=bins[i];
			entrySum+=bins[i]*entryVal;
			entryVal+=interval;
		}
		
		//find fwhm points  for first peak
		double halfMax=maxValue/2.0;
		//right
		double posHalfMaxR;
		bool foundHalfMaxR=false;
		for (int i=indexMax;i<numBins-1;i++)
		{
			//found a match
			if ((bins[i]-halfMax)*(bins[i+1]-halfMax)<0)
			{
				double x1=interval*i+parameterMin;
				double x2=interval*(i+1)+parameterMin;
				double p1=bins[i];
				double p2=bins[i+1];

				posHalfMaxR=x1+(x2-x1)*(p1-halfMax)/(p1-p2);
				foundHalfMaxR=true;
				break;
			}
		}
		//left
		double posHalfMaxL;
		bool foundHalfMaxL=false;
		for (int i=indexMax;i>=0;i--)
		{
			//found a match
			if ((bins[i]-halfMax)*(bins[i+1]-halfMax)<0)
			{
				double x1=interval*i+parameterMin;
				double x2=interval*(i+1)+parameterMin;
				double p1=bins[i];
				double p2=bins[i+1];

				posHalfMaxL=x1+(x2-x1)*(p1-halfMax)/(p1-p2);
				foundHalfMaxL=true;
				break;
			}
		}
		mean1=indexMax;	
		A1=maxValue;
		if (foundHalfMaxL && foundHalfMaxR)
		{
			FWHM=abs(posHalfMaxR-posHalfMaxL);
			sigma1=FWHM/(2.35482*interval);
		}
		else
			return;

		//find second peak
		indexMax=(((posHalfMaxR+posHalfMaxL)/2.0)-parameterMin)/interval;
		//search right
		int startIndexR=(int)(indexMax+2*sigma1);
		int indexMaxR=startIndexR;
		double maxValR=0;
		for (int i=startIndexR;i<numBins-1;i++)
		{
			if (bins[i]>maxValR)
			{
				indexMaxR=i;
				maxValR=bins[i];
			}
		}
		//search left
		int startIndexL=(int)(indexMax-2*sigma1);
		int indexMaxL=startIndexL;
		double maxValL=0;
		for (int i=startIndexL;i>0;i--)
		{
			if (bins[i]>maxValL)
			{
				indexMaxL=i;
				maxValL=bins[i];
			}
		}
		if (maxValL>maxValR)
			mean2=indexMaxL;
		else
			mean2=indexMaxR;

		//find fwhm points  for second peak
		maxValue=max(maxValL, maxValR);
		A2=maxValue;
		halfMax=maxValue/2.0;
		//right
		foundHalfMaxR=false;
		for (int i=mean2;i<numBins-1;i++)
		{
			//found a match
			if ((bins[i]-halfMax)*(bins[i+1]-halfMax)<0)
			{
				double x1=interval*i+parameterMin;
				double x2=interval*(i+1)+parameterMin;
				double p1=bins[i];
				double p2=bins[i+1];

				posHalfMaxR=x1+(x2-x1)*(p1-halfMax)/(p1-p2);
				foundHalfMaxR=true;
				break;
			}
		}
		//left
		foundHalfMaxL=false;
		for (int i=mean2;i>=0;i--)
		{
			//found a match
			if ((bins[i]-halfMax)*(bins[i+1]-halfMax)<0)
			{
				double x1=interval*i+parameterMin;
				double x2=interval*(i+1)+parameterMin;
				double p1=bins[i];
				double p2=bins[i+1];

				posHalfMaxL=x1+(x2-x1)*(p1-halfMax)/(p1-p2);
				foundHalfMaxL=true;
				break;
			}
		}
		if (foundHalfMaxL && foundHalfMaxR)
		{
			FWHM=abs(posHalfMaxR-posHalfMaxL);
			sigma2=FWHM/(2.35482*interval);
		}		
	}
}