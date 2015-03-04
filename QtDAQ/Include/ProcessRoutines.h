#pragma once
#include "DRS4Acquisition.h"
#include "vector/vectorclass.h"
#include <string>
#include <algorithm>


//helpers

inline float lerp(float v0, float v1, float t) 
{
  return (1-t)*v0 + t*v1;
}
//all input and output should be passed as pointers (or float pointers in the case of arrays) and parameters should be named accordingly. should all return bool (success)

//performs a low pass (RC) filter with a given alpha parameter
bool lowPassFilter(float* inputData, int inputNumSamples, float inputAlpha);

//performs a trapezoidal filter with tiled boundary conditions
bool trapezoidalFilter(float* inputData, int inputNumSamples, int sampleLength, int gapLength, float offset);

//searches for max / min between start and end index
bool findMaxValue(float* inputData, int inputNumSamples, int inputStartIndex, int inputEndIndex, float& outputMaxValue, int& outputPositionOfMax);
bool findMinValue(float* inputData, int inputNumSamples, int inputStartIndex, int inputEndIndex, float& outputMinValue, int& outputPositionOfMin);
bool findMinMaxValue(float* inputData, int inputNumSamples, int inputStartIndex, int inputEndIndex, float& outputMinValue, int& outputPositionOfMin, float& outputMaxValue, int& outputPositionOfMax);

//find baseline, by using a moving average of size sampleSize, searching for the region (within sampleRange) with the lowest variance
bool findBaseline(float* inputData, int inputSampleStart, int inputSampleRange, int inputSampleSize, float& outputBaseline);

bool applyGain(float* inputData, int inputNumSamples, float inputGain, float inputBaseline);

//find the half-rise point. In order to do this, we need to search backward from the min value
bool findHalfRise(float* inputData, int inputNumSamples, int inputPositionOfMin, float inputBaseline, int& outputPositionOfHalfRise);

//calculates the charge integrals using the input data and gates.
bool calculateIntegrals(float* inputData, int inputNumSamples, float inputBaseline, int inputStartGate, int inputShortGateEnd, int inputLongGateEnd, float& outputShortIntegral, float& outputLongIntegral);
//calculates the charge integrals using the input data and gates.
bool calculateIntegralsLinearBaseline(float* inputData, int inputNumSamples, int inputNumBaselineSamples, int inputStartGate, int inputShortGateEnd, int inputLongGateEnd, float& outputShortIntegral, float& outputLongIntegral);
//with partial corrections (requires float arguments)
bool calculateIntegralsCorrected(float* inputData, int inputNumSamples, float inputBaseline, float inputStartGate, float inputShortGateEnd, float inputLongGateEnd, float& outputShortIntegral, float& outputLongIntegral);

//calculates the charge integrals using the input data and gates. (trapezoidal integration)
bool calculateIntegralsTrapezoidal(float* inputData, int inputNumSamples, float inputBaseline, float inputStartGate, float inputShortGateEnd, float inputLongGateEnd, float& outputShortIntegral, float& outputLongIntegral);

//calculates the charge integrals using the input data and gates. (Simpson cubic integration)
bool calculateIntegralsSimpson(float* inputData, int inputNumSamples, float inputBaseline, float inputStartGate, float inputShortGateEnd, float inputLongGateEnd, float& outputShortIntegral, float& outputLongIntegral);


//calculates the time to pass a certain fraction of the total integral.
bool calculateZeroCrossing(float* inputData, int inputNumSamples, float inputBaseline, int inputStartGate, int inputLongGateEnd, float inputLongIntegral, float inputZCFStart, float inputZCFStop, float& outputZeroCrossingInterval);

//calculates PSD factor using the input integrals
bool calculatePSD(float inputShortIntegral, float inputLongIntegral, float& outputPSD);

//clones input data into newly created output data
bool fork(float* inputData, int inputNumSamples, float*& outputData);

//clones input data into existing data array
bool clone(float* inputData, int inputNumSamples, float* outputData);

//clones input data into existing data array (aligned)
bool clone_a(float* inputData, int inputNumSamples, float* outputData);

//removes input data and zeros pointer
bool end(float* inputData);

//outputs numerically differentiated sample
bool delta(float* inputData, int inputNumSamples, float inputGain, float inputOffset);

//cfd sample
bool cfdSample(float* inputData, float inputBaseline, int inputNumSamples, float inputF, int inputL, int inputD, float inputOffset, float inputScale, float*& outputData);
bool cfdSampleOptimized(float* inputData, float inputBaseline, int inputNumSamples, float inputF, int inputL, int inputD, float inputOffset, float inputScale, float*& outputData);

//linear fit of a section of data
bool linearFit(float* inputData, int inputNumSamples, int startIndex, int endIndex, float& outputSlope, float& outputOffset);

//search for trigger 
bool findIntersection(float* inputData, int inputNumSamples, int inputSearchStart, int inputSearchEnd, int inputCrossingMinLength, float inputThreshold, bool inputNegative, int& outputCrossingPosition);

bool linearFilter(float* inputData, int inputNumSamples, float inputBaseline, float inputStart, float* inputFilter, int inputFilterLength, float& outputSum);

void medianfilter(const float* signal, float* result, int N);