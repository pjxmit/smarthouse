#include <stdio.h>
#include <math.h>
#include "string.h"
#include "arch.h"
#include "math_opt.h"
#include "peaklevel.h"

#define EPS 1.0E-8


typedef struct {
	int enable;    ///16000
	int fs; //16000
	float AT;
	float RT;
	float AVT;
	float peak;
	float peakLinear;
	float attAlpha;// attack time 
	float relAlpha;// release time
	float avgAlpha;// average time;
}tPeakLevel;


void* peakLevel_init(int fs, tPeakLeavelParam* param)
{
	tPeakLevel* pkl = (tPeakLevel*)malloc(sizeof(tPeakLevel));
	if(pkl == NULL)
		return NULL;
	pkl->fs = fs;
	pkl->enable = param->enable;
	pkl->AT = param->attack_time;//0.0002;
	pkl->RT = param->release_time;//0.200;
	pkl->AVT = param->avg_time;
	pkl->attAlpha = 1 - exp(-2.2 / (fs * pkl->AT));
	pkl->relAlpha = 1 - exp(-2.2 / (fs * pkl->RT));
	pkl->avgAlpha = 1 - exp(-2.2 / (fs * pkl->AVT));
	pkl->peak = EPS;
	return pkl;
}

int peakLevel_set_param(void*handle, tPeakLeavelParam*param)
{
	tPeakLevel * pkl = (tPeakLevel *)handle;
	pkl->enable = param->enable;
	pkl->AT = param->attack_time;//0.0002;
	pkl->RT = param->release_time;//0.200;
	pkl->attAlpha = 1 - exp(-2.2 / (pkl->fs * pkl->AT));
	pkl->relAlpha = 1 - exp(-2.2 / (pkl->fs * pkl->RT));
	pkl->avgAlpha = 1 - exp(-2.2 / (pkl->fs * pkl->AVT));
	pkl->peak = EPS;
	return 0;
}

int peakLevel_set_param_v2(void*handle, tPeakLevelParamCalc* param)
{
	tPeakLevel * pkl = (tPeakLevel *)handle;
	pkl->enable = param->enable;
	pkl->attAlpha = param->attack_value;
	pkl->relAlpha = param->release_value;
	pkl->avgAlpha = param->average_value;
	pkl->peak = EPS;
	return 0;
}

float peakLevel_trans_time2value(int fs, float time)
{
	return 1 - exp(-2.2 / (fs * time));
}

float peakLevel_process(void * handle, float *in, int samples)
{
	float coeff = 0.0f;
	tPeakLevel * pkl = (tPeakLevel *)handle;
	float data, absData;
	float *pdata;
	int i;
	pdata = in;
	float peak = pkl->peak;
	float rms = 0;
	if (pkl->enable)
	{
		for (i=0; i< samples; i++)
		{
			data = *pdata++;
			
			#if 0//peak-level
			absData = my_fabs(data);
			
			if (absData > peak)
				coeff = pkl->attAlpha;
			else 
				coeff = pkl->relAlpha;
			peak = (1.0 - coeff) * peak + coeff * absData;
			#else //rms-level
			rms = data*data;
			coeff = pkl->avgAlpha;
			peak = (1.0 - coeff) * peak + coeff * rms;

			#endif
		}
	}
	else
		peak = EPS;
	pkl->peak = peak;
//	printf("%f-%f-%f\n", coeff, peak, rms);
	#if 0
	peak = 20*my_log10f(peak);
	#else
	peak = 10*my_log10f(peak);
	#endif

	return  peak;
}


