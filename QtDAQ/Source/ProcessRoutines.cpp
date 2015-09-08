#include <string.h>
#include <algorithm>
#include "vector/vectorclass.h"
#include "ProcessRoutines.h"
//Applies a low pass filter to the data (inline), based in input alpha parameter
bool lowPassFilter(float* inputData, int inputNumSamples, float inputAlpha)
{
	float prev=inputData[0];
	for (int i=1;i<inputNumSamples;i++)
	{
		prev=inputAlpha*inputData[i]+(1-inputAlpha)*prev;
		inputData[i]=(float)prev;
	}
	return true;
}

//performs a trapezoidal filter with clamped bc
bool trapezoidalFilter(float* inputData, int inputNumSamples, int sampleLength, int gapLength, float offset)
{
	if (inputNumSamples<=sampleLength+gapLength)
		return false;


	//TODO: this currently results in garbage in the last section of the data. Need to correct
	float sum1=0, sum2=0;
	for (int i=0;i<sampleLength;i++)
	{
		sum1+=inputData[i];
		sum2+=inputData[i+gapLength];
	}
	float prevVal=(sum2-sum1)/sampleLength+offset;
	//loop to the end (<= because we're assigning  to [i-1])
	for (int i=1;i<=inputNumSamples;i++)
	{
		//remove previous first samples
		sum1-=inputData[i-1];
		//tiled bc
		
		sum2 -= inputData[std::min(i - 1 + gapLength, inputNumSamples - 1)];
		//add next sample
		sum1 += inputData[std::min(i + sampleLength, inputNumSamples - 1)];
		sum2 += inputData[std::min(i + gapLength + sampleLength, inputNumSamples - 1)];
		//assign new values
		inputData[i-1]=prevVal;
		prevVal=(sum2-sum1)/sampleLength+offset;
	}
	return true;
}

bool cfdSample(float* inputData, float inputBaseline, int inputNumSamples, float inputF, int inputL, int inputD, float inputOffset, float inputScale, float*& outputData)
{
	float F=inputF;
	int L=inputL;
	int D=inputD;
	int N=inputNumSamples;    
	//start from k-(L+D)=0 => k=L+D
	int k=L+D;
	if (k>inputNumSamples)
		return false;
	//zero the first part of the output    	
	for (int i=0;i<inputNumSamples;i++)
		outputData[i]=inputOffset;

	float baselineFix=inputBaseline*(1.0-F);
	for (k=L+D;k<inputNumSamples;k++)
	{		
		for (int i=1;i<=L;i++)
		{
            outputData[k]+=(F*(inputData[k-i])-(inputData[k-i-D])+baselineFix)*inputScale;
		}		
	}

	return true;
}

bool cfdSampleOptimized(float* inputData, float inputBaseline, int inputNumSamples, float inputF, int inputL, int inputD, float inputOffset, float inputScale, float*& outputData)
{
	float F=inputF;
	int L=inputL;
	int D=inputD;
	int N=inputNumSamples;    
	//start from k-(L+D)=0 => k=L+D
	int k=L+D;
	if (k>inputNumSamples)
		return false;
	//zero the first part of the output    	
	for (int i=0;i<=L+D;i++)
		outputData[i]=inputOffset;

	float baselineFix=inputBaseline*(1.0-F);

	for (int i=1;i<=L;i++)
	{
        outputData[k]+=(F*(inputData[k-i])-(inputData[k-i-D])+baselineFix)*inputScale;
	}		
	
	float* i_n = &(inputData[L + D]);
	float* i_nl = &(inputData[D]);
	float* i_nd = &(inputData[L]);
	float* i_nld = inputData;
	float* o_n = &(outputData[L + D]);
	for (int n=L+D;n<inputNumSamples-1;n++)
	{	
		outputData[n+1]=outputData[n]+(F*(inputData[n]-inputData[n-L])-inputData[n-D]+inputData[n-L-D])*inputScale;
	}	
	return true;
}

//   1D MEDIAN FILTER implementation
//     signal - input signal
//     result - output signal
//     N      - length of the signal
void medianfilter(const float* signal, float* result, int N)
{
	//   Move window through all elements of the signal
	for (int i = 2; i < N - 2; ++i)
	{
		//   Pick up window elements
		float window[5];
		for (int j = 0; j < 5; ++j)
			window[j] = signal[i - 2 + j];
		//   Order elements (only half of them)
		for (int j = 0; j < 3; ++j)
		{
			//   Find position of minimum element
			int min = j;
			for (int k = j + 1; k < 5; ++k)
				if (window[k] < window[min])
					min = k;
			//   Put found minimum element in its place
			const float temp = window[j];
			window[j] = window[min];
			window[min] = temp;
		}
		//   Get result - the middle element
		result[i - 2] = window[2];
	}
}


bool linearFit(float* inputData, int inputNumSamples, int startIndex, int endIndex, float& outputSlope, float& outputOffset)
{
    if (startIndex<0 || endIndex>=inputNumSamples || startIndex>=endIndex)
        return false;

    float s_x=0, s_y=0, s_x2=0, s_xy=0;
    int N=0;
    for (int i=startIndex;i<=endIndex;i++)
    {
        s_x+=i;
        s_y+=inputData[i];
        s_x2+=i*i;
        s_xy+=i*inputData[i];
        N++;
    }

    float av_x=s_x/N;
    float av_y=s_y/N;
    float av_x2=s_x2/N;
    float av_xy=s_xy/N;

    outputSlope=(av_xy-av_x*av_y)/(av_x2-av_x*av_x);
    outputOffset=av_y-outputSlope*av_x;
    return true;
}


bool findMaxValue(float* inputData, int inputNumSamples, int inputStartIndex, int inputEndIndex, float& outputMaxValue, int& outputPositionOfMax)
{
	float maxVal=inputData[inputStartIndex];
	int positionOfMax=inputStartIndex;
	for (int i=inputStartIndex+1;i<inputEndIndex;i++)
	{
		if (inputData[i]>maxVal)
		{
			maxVal=inputData[i];
			positionOfMax=i;
		}
	}
	outputMaxValue=maxVal;
	outputPositionOfMax=positionOfMax;
	return true;
}

bool findMinValue(float* inputData, int inputNumSamples, int inputStartIndex, int inputEndIndex, float& outputMinValue, int& outputPositionOfMin)
{
	float minVal=inputData[inputStartIndex];
	int positionOfMin=inputStartIndex;
	for (int i=inputStartIndex+1;i<inputEndIndex;i++)
	{
		if (inputData[i]<minVal)
		{
			minVal=inputData[i];
			positionOfMin=i;
		}
	}
	outputMinValue=minVal;
	outputPositionOfMin=positionOfMin;
	return true;
}


bool findMinMaxValue(float* inputData, int inputNumSamples, int inputStartIndex, int inputEndIndex, float& outputMinValue, int& outputPositionOfMin, float& outputMaxValue, int& outputPositionOfMax)
{
	float minVal=inputData[inputStartIndex];
	float maxVal=minVal;
	int positionOfMin=inputStartIndex;
	int positionOfMax=inputStartIndex;
	for (int i=inputStartIndex+1;i<inputEndIndex;i++)
	{
		if (inputData[i]<minVal)
		{
			minVal=inputData[i];
			positionOfMin=i;
		}
		else if (inputData[i]>maxVal)
		{
			maxVal=inputData[i];
			positionOfMax=i;
		}
	}
	outputMinValue=minVal;
	outputPositionOfMin=positionOfMin;
	outputMaxValue=maxVal;
	outputPositionOfMax=positionOfMax;
	return true;
}


//find baseline, by using a moving average of size sampleSize, searching for the region (within sampleRange) with the lowest variance
bool findBaseline(float* inputData, int inputSampleStart, int inputSampleRange, int inputSampleSize, float& outputBaseline)
{

	if (inputSampleRange<0 || inputSampleRange<inputSampleSize)
		return false;

	float sumX=0;
	float sumX2=0;
	//moving variance check for best baseline
	//initial sigma_x, sigma_x2
	for (int i=inputSampleStart;i<inputSampleStart+inputSampleSize;i++)
	{
		sumX+=inputData[i];
		sumX2+=inputData[i]*inputData[i];
	}
	float meanX=sumX/(float)inputSampleSize;
	float meanX2=sumX2/(float)inputSampleSize;
	float variance=meanX2-meanX*meanX;

	float minVariance=variance;
	float meanAtMinVariance=meanX;
	//consider moving average for range (from [0:sampleSize] -> [sampleRange-sampleSize:sampleRange])

	int maxOffset=inputSampleRange-inputSampleSize;
	if (maxOffset<0)
		return false;
	for (int i=inputSampleStart;i<inputSampleStart+maxOffset;i++)
	{
		//remove the first element
		sumX-=inputData[i];
		sumX2-=inputData[i]*inputData[i];
		//add the next
		sumX+=inputData[i+inputSampleSize];
		sumX2+=inputData[i+inputSampleSize]*inputData[i+inputSampleSize];

        float tempMeanX=sumX/(float)inputSampleSize;
        float tempMeanX2=sumX2/(float)inputSampleSize;
		float tempVariance=tempMeanX-tempMeanX2*tempMeanX2;
		if (tempVariance<minVariance)
		{
			minVariance=tempVariance;
			meanAtMinVariance=tempMeanX;
		}
	}
	outputBaseline=meanAtMinVariance;
	return true;
}

bool applyGain(float* inputData, int inputNumSamples, float inputGain, float inputBaseline)
{
    float newBaseline=(inputGain<0)?(1024-inputBaseline):inputBaseline;
    for (int i=inputNumSamples-1;i>=0;i--)
        inputData[i]=newBaseline+(inputData[i]-inputBaseline)*inputGain;
    return true;
}

//find the half-rise point. In order to do this, we need to search backward from the min value
bool findHalfRise(float* inputData, int inputNumSamples, int inputPositionOfMin, float inputBaseline, int& outputPositionOfHalfRise)
{
	//check bounds
	if (inputPositionOfMin<0 || inputPositionOfMin>=inputNumSamples)
		return false;

	//calculate pulse height (difference between min and baseline)
	float pulseHeight=(inputData[inputPositionOfMin]-inputBaseline);
	//height of half-rise will be baseline + height/2
	float halfRise=inputBaseline+pulseHeight/2.0f;	
	//search back for half rise
	for (int i=inputPositionOfMin;i>=0;i--)
	{
		//check to see if we've switched over
		if ((inputData[i]-halfRise)*(inputData[i+1]-halfRise)<0)
		{
			//store the position in output and return
			outputPositionOfHalfRise=i;
			return true;
		}
	}
	//didn't find half-rise, something bad happened
	return false;
}

//calculates the charge integrals using the input data and gates.
bool calculateIntegrals(float* inputData, int inputNumSamples, float inputBaseline, int inputStartGate, int inputShortGateEnd, int inputLongGateEnd, float& outputShortIntegral, float& outputLongIntegral)
{

	//check bounds
	if (inputStartGate<0 || inputShortGateEnd<inputStartGate || inputShortGateEnd>inputLongGateEnd || inputLongGateEnd>=inputNumSamples)
		return false;

	float shortIntegralUncorrected=0;
	float longIntegralUncorrected=0;

	for (int i=inputStartGate;i<inputShortGateEnd;i++)
	{
		//within short gate, both integrals incremented
		shortIntegralUncorrected+=inputData[i];
	}
	longIntegralUncorrected=shortIntegralUncorrected;
	//after short gate end, only increment long gate
	for (int i=inputShortGateEnd;i<inputLongGateEnd;i++)
		longIntegralUncorrected+=inputData[i];

	//correct for baseline
	outputShortIntegral=inputBaseline*(inputShortGateEnd-inputStartGate)-shortIntegralUncorrected;
	outputLongIntegral=inputBaseline*(inputLongGateEnd-inputStartGate)-longIntegralUncorrected;

    return true;
}

//calculates charge integrals after determining a linear baseline to subtract
bool calculateIntegralsLinearBaseline(float* inputData, int inputNumSamples, int inputNumBaselineSamples, int inputStartGate, int inputShortGateEnd, int inputLongGateEnd, float& outputShortIntegral, float& outputLongIntegral)
{
	//check bounds
	if (inputNumBaselineSamples==0 || inputStartGate<inputNumBaselineSamples || inputShortGateEnd<inputStartGate || inputShortGateEnd>inputLongGateEnd || inputLongGateEnd >= inputNumSamples - inputNumBaselineSamples)
		return false;

	//calculate baseline
	int hackOffset = 30;
	int i_A = inputNumBaselineSamples / 2;
	int i_B = inputNumSamples - hackOffset - inputNumBaselineSamples / 2;

	float V_A = 0, V_B = 0;

	for (int i = 0; i < inputNumBaselineSamples; i++)
	{
		V_A += inputData[i];
		V_B += inputData[inputNumSamples - hackOffset - inputNumBaselineSamples + i];
	}
	V_A /= inputNumBaselineSamples;
	V_B /= inputNumBaselineSamples;

	float m = (V_B - V_A) / (i_B - i_A);
	float C = V_A - m * i_A;
	

	float shortIntegralUncorrected = 0;
	float longIntegralUncorrected = 0;

	for (int i = inputStartGate; i<inputShortGateEnd; i++)
	{
		//within short gate, both integrals incremented
		shortIntegralUncorrected += (m*i + C) - inputData[i];
	}
	longIntegralUncorrected = shortIntegralUncorrected;
	//after short gate end, only increment long gate
	for (int i = inputShortGateEnd; i < inputLongGateEnd; i++)
		longIntegralUncorrected += (m*i + C) - inputData[i];

	shortIntegralUncorrected -= (m*inputStartGate + C) - (inputStartGate - (int)inputStartGate)*inputData[(int)inputStartGate];
	longIntegralUncorrected -= (m*inputStartGate + C) - (inputStartGate - (int)inputStartGate)*inputData[(int)inputStartGate];
	shortIntegralUncorrected += (m*inputShortGateEnd + C) - (inputShortGateEnd - (int)inputShortGateEnd)*inputData[(int)inputShortGateEnd];
	longIntegralUncorrected += (m*inputLongGateEnd + C) - (inputLongGateEnd - (int)inputLongGateEnd)*inputData[(int)inputLongGateEnd];
	//correct for baseline
	outputShortIntegral = shortIntegralUncorrected;
	outputLongIntegral = longIntegralUncorrected;

	return true;
}


//calculates the charge integrals using the input data and gates.
bool calculateIntegralsCorrected(float* inputData, int inputNumSamples, float inputBaseline, float inputStartGate, float inputShortGateEnd, float inputLongGateEnd, float& outputShortIntegral, float& outputLongIntegral)
{

	//check bounds
	if (inputStartGate<0 || inputShortGateEnd<inputStartGate || inputShortGateEnd>inputLongGateEnd || inputLongGateEnd>=inputNumSamples)
		return false;

	float shortIntegralUncorrected=0;
	float longIntegralUncorrected=0;

	for (int i=inputStartGate;i<inputShortGateEnd;i++)
	{
		//within short gate, both integrals incremented
		shortIntegralUncorrected+=(inputBaseline-inputData[i]);
	} 
	longIntegralUncorrected=shortIntegralUncorrected;
	//after short gate end, only increment long gate
	for (int i=inputShortGateEnd;i<inputLongGateEnd;i++)
		longIntegralUncorrected += (inputBaseline - inputData[i]);

	shortIntegralUncorrected-=inputBaseline-(inputStartGate-(int)inputStartGate)*inputData[(int)inputStartGate];
	longIntegralUncorrected-=inputBaseline-(inputStartGate-(int)inputStartGate)*inputData[(int)inputStartGate];
	shortIntegralUncorrected+=inputBaseline-(inputShortGateEnd-(int)inputShortGateEnd)*inputData[(int)inputShortGateEnd];
	longIntegralUncorrected+=inputBaseline-(inputLongGateEnd-(int)inputLongGateEnd)*inputData[(int)inputLongGateEnd];
	//correct for baseline
	outputShortIntegral=shortIntegralUncorrected;
	outputLongIntegral=longIntegralUncorrected;

    return true;
}

//calculates the charge integrals using the input data and gates.
bool calculateIntegralsTrapezoidal(float* inputData, int inputNumSamples, float inputBaseline, float inputStartGate, float inputShortGateEnd, float inputLongGateEnd, float& outputShortIntegral, float& outputLongIntegral)
{

	//check bounds
	if (inputStartGate<0 || inputShortGateEnd<inputStartGate || inputShortGateEnd>inputLongGateEnd || inputLongGateEnd>=inputNumSamples)
		return false;

	float shortIntegralUncorrected=0;
	float longIntegralUncorrected=0;

	/*
	//short integral
	float start=inputStartGate;
	float end=inputShortGateEnd;
	int first=ceil(inputStartGate);
	int pre=floor(inputStartGate);
	int last=floor(inputShortGateEnd);
	int post=ceil(inputShortGateEnd);

	float V_pre=inputData[pre]+(start-pre)*(inputData[first]-inputData[pre]);
	float A_pre=V_pre*(first-start)+0.5f*(first-start)*(inputData[first]-V_pre);
	float V_post=inputData[last]+(end-last)*inputData[post];
	float A_post=V_post*(end-last)+0.5f*(end-last)*(inputData[last]-V_post);
	float A_mid=inputData[first];
	for (int i=first+1;i<last;i++)
		A_mid+=2*inputData[i];
	A_mid+=inputData[last];
	A_mid/=2;
	shortIntegralUncorrected=A_pre+A_mid+A_post;
	//long integral
	end=inputLongGateEnd;
	last=floor(inputLongGateEnd);
	post=ceil(inputLongGateEnd);
	V_post=inputData[last]+(end-last)*inputData[post];
	A_post=V_post*(end-last)+0.5f*(end-last)*(inputData[last]-V_post);
	A_mid=inputData[first];
	for (int i=first+1;i<last;i++)
		A_mid+=2*inputData[i];
	A_mid+=inputData[last];
	A_mid/=2;
	longIntegralUncorrected=A_pre+A_mid+A_post;
	//correct for baseline
	outputShortIntegral=inputBaseline*(inputShortGateEnd-inputStartGate)-shortIntegralUncorrected;
	outputLongIntegral=inputBaseline*(inputLongGateEnd-inputStartGate)-longIntegralUncorrected;
	*/


	//simple trapezoidal: 

	int first=(int)inputStartGate;
	int shortEnd=(int)inputShortGateEnd;
	int longEnd=(int)inputLongGateEnd;

	shortIntegralUncorrected=inputData[first];
	for (int i=first+1;i<shortEnd;i++)
		shortIntegralUncorrected+=2*inputData[i];
	shortIntegralUncorrected+=inputData[shortEnd];
	shortIntegralUncorrected/=2;

	longIntegralUncorrected=inputData[first];
	for (int i=first+1;i<longEnd;i++)
		longIntegralUncorrected+=2*inputData[i];
	longIntegralUncorrected+=inputData[longEnd];
	longIntegralUncorrected/=2;

	outputShortIntegral=inputBaseline*(inputShortGateEnd-inputStartGate)-shortIntegralUncorrected;
	outputLongIntegral=inputBaseline*(inputLongGateEnd-inputStartGate)-longIntegralUncorrected;

	return true;
}

bool calculateIntegralsSimpson(float* inputData, int inputNumSamples, float inputBaseline, float inputStartGate, float inputShortGateEnd, float inputLongGateEnd, float& outputShortIntegral, float& outputLongIntegral)
{
	//check bounds
	if (inputStartGate<0 || inputShortGateEnd<inputStartGate || inputShortGateEnd>inputLongGateEnd || inputLongGateEnd>=inputNumSamples)
		return false;

	float shortIntegralUncorrected=0;
	float longIntegralUncorrected=0;

	int first=(int)inputStartGate;
	int shortEnd=(int)inputShortGateEnd;
	int longEnd=(int)inputLongGateEnd;

	int mod3=(shortEnd-first)%3;
	if (mod3!=0)	
		first-=(3-mod3);
	mod3=(shortEnd-first)%3;
	mod3=(longEnd-first)%3;
	if (mod3!=0)
		longEnd+=(3-mod3);
	mod3=(longEnd-first)%3;
	shortIntegralUncorrected=inputData[first];
	for (int i=first+1, j=1;i<shortEnd;i++, j++)
	{
		int coeff=(j%3==0)?2:3;
		shortIntegralUncorrected+=coeff*inputData[i];
	}
	shortIntegralUncorrected+=inputData[shortEnd];
	shortIntegralUncorrected*=3.0/8.0;

	longIntegralUncorrected=inputData[first];
	for (int i=first+1, j=1;i<longEnd;i++, j++)
	{
		int coeff=(j%3==0)?2:3;
		longIntegralUncorrected+=coeff*inputData[i];
	}
	longIntegralUncorrected+=inputData[longEnd];
	longIntegralUncorrected*=3.0/8.0;
	



	outputShortIntegral=inputBaseline*(shortEnd-first)-shortIntegralUncorrected;
	outputLongIntegral=inputBaseline*(longEnd-first)-longIntegralUncorrected;

	return true;
}


bool calculateZeroCrossing(float* inputData, int inputNumSamples, float inputBaseline, int inputStartGate, int inputLongGateEnd, float inputLongIntegral, float inputZCFStart, float inputZCFStop, float& outputZeroCrossingInterval)
{
	if (inputStartGate<0 || inputLongGateEnd<=inputStartGate || inputLongGateEnd>= inputNumSamples || inputZCFStop>=1.0 || inputZCFStart >= inputZCFStop)
        return false;

    //performing the search forwards will be quicker, since most of the
    float sumLinearCutoffStart=(inputZCFStart)*inputLongIntegral;
    float sumLinearCutoffStop=(inputZCFStop)*inputLongIntegral;
    float tempSum=0;
	float prevSum=tempSum;
	float startTime=0;
	float stopTime=0;
	bool foundStartTime=false;
    for (int i=inputStartGate;i<inputLongGateEnd;i++)
    {
        tempSum+=(inputBaseline-inputData[i]);
        if (!foundStartTime && tempSum>=sumLinearCutoffStart)
        {
			int i1=i-1, i2=i;
			float f1=prevSum, f2=tempSum;
			float f=sumLinearCutoffStart;
            startTime=i1+(f-f1)/(f2-f1)-inputStartGate;
            foundStartTime = true;
        }
		if (foundStartTime && tempSum>=sumLinearCutoffStop)
        {
			int i1=i-1, i2=i;
			float f1=prevSum, f2=tempSum;
			float f=sumLinearCutoffStop;
            stopTime=i1+(f-f1)/(f2-f1)-inputStartGate;
			outputZeroCrossingInterval=stopTime-startTime;
            return true;
        }
		prevSum=tempSum;
    }
    //something wrong with the crossing
	outputZeroCrossingInterval=0;
    return false;
}

//calculates PSD factor using the input integrals
bool calculatePSD(float inputShortIntegral, float inputLongIntegral, float& outputPSD)
{
	//can only get a PSD if both integrals are positive and long integral is longer than short
	if (inputShortIntegral >0 && inputLongIntegral > inputShortIntegral)
	{
		//late f.brooks def of psd val
		outputPSD=100*inputShortIntegral/inputLongIntegral;
		return true;
	}
	else
	{
		outputPSD=100*inputShortIntegral/inputLongIntegral;
		return false;
	}
}

//clones input data into output data
bool fork(float* inputData, int inputNumSamples, float*& outputData)
{
	float* data=new float[inputNumSamples];
	memcpy(data, inputData, sizeof(float)*inputNumSamples);
	outputData=data;
	return true;
}

bool clone(float* inputData, int inputNumSamples, float* outputData)
{
	if (inputNumSamples%4==0)
	{
		int numIterations=inputNumSamples/4;
		Vec4f vInput;
		for (int i=0;i<inputNumSamples/4;i++, inputData+=4, outputData+=4)
		{
		vInput.load(inputData);
		vInput.store(outputData);
		}
	}
	else
		memcpy(outputData, inputData, sizeof(float)*inputNumSamples);
	return true;
}

bool clone_a(float* inputData, int inputNumSamples, float* outputData)
{
	if (inputNumSamples%4==0)
	{
		int numIterations=inputNumSamples/4;
		Vec4f vInput;
		for (int i=0;i<inputNumSamples/4;i++, inputData+=4, outputData+=4)
		{
		vInput.load_a(inputData);
		vInput.store_a(outputData);
		}
	}
	else
		memcpy(outputData, inputData, sizeof(float)*inputNumSamples);
	return true;
}

//removes input data and zeros pointer
bool end(float* inputData)
{
	if (!inputData)
		return false;
	else
	{
		delete [] inputData;
		inputData=0;
		return true;
	}
}


bool delta(float* inputData, int inputNumSamples, float inputGain, float inputOffset)
{
	for (int i=inputNumSamples-1;i>0;i--)
		inputData[i]=inputOffset+inputGain*(inputData[i]-inputData[i-1]);
	inputData[0]=inputOffset;
	return true;
}

//bool findIntersection(float* inputData, int inputNumSamples, int inputStartIndex, int inputEndIndex, float inputThreshold, int& outputPossitionOfCrossing);



bool findIntersection(float* inputData, int inputNumSamples, int inputSearchStart, int inputSearchEnd, int inputCrossingMinLength, float inputThreshold, bool inputNegative, int& outputCrossingPosition)
{
    if (inputSearchStart<0 || inputSearchStart+inputCrossingMinLength>=inputNumSamples)
	{
        outputCrossingPosition=-1;
		return false;
	}
	//TODO: optimize this!
	int polarity=(inputNegative?1:-1);
    for (int i=inputSearchStart;i<inputSearchEnd-inputCrossingMinLength;i++)
	{
        if (polarity*inputData[i]<polarity*inputThreshold)
		{
			bool singleHit=false;
			//possible trigger. Now check to make sure it's not a single hit
            for (int j=1;j<inputCrossingMinLength;j++)
			{
                if (polarity*inputData[i+j]>polarity*inputThreshold)
				{
					singleHit=true;
					//skip ahead, nothing to see here, folks
					i+=j;
					break;
				}
			}
			//we have a trigger!
			if (!singleHit)
			{
                outputCrossingPosition=i;
				return true;
			}
		}
	}
    outputCrossingPosition=-1;
	//true just means no errors
	return true;
}

bool linearFilter(float* inputData, int inputNumSamples, float inputBaseline, float inputStart, float* inputFilter, int inputFilterLength, float& outputSum)
{
	if (inputStart<1 || inputStart+inputFilterLength>=inputNumSamples-1)
	{
        outputSum=-1000;
		return false;
	}
	outputSum=0;
	float t=inputStart-floor(inputStart);
	int j=(int)floor(inputStart);
	for (int i=0, j=inputStart;i<inputFilterLength;i++,j++)
	{		
		float inputVal=lerp(inputData[j], inputData[j+1], t);
		outputSum+=(inputBaseline-inputVal)*inputFilter[i];
		//outputSum+=(inputBaseline-inputData[j])*inputFilter[i];

	}
	return true;
}


