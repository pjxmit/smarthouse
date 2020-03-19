#ifndef _PEAKLEVEL_H
#define _PEAKLEVEL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	int enable;
	float attack_time; //by seconds 0.0002 default
	float release_time;// 0.2 default
	float avg_time;//average time, for rms-level
}tPeakLeavelParam;

typedef struct
{
	int enable;
	float attack_value;
	float release_value;
	float average_value;
}tPeakLevelParamCalc;

void* peakLevel_init(int fs, tPeakLeavelParam* param);

int peakLevel_set_param(void*handle, tPeakLeavelParam*param);
int peakLevel_set_param_v2(void*handle, tPeakLevelParamCalc* param);
float peakLevel_trans_time2value(int fs, float time); //change attack_time/release_time to attack_value/ release_value

float peakLevel_process(void * handle, float *in, int samples); //return peaklevel by dB


#ifdef __cplusplus
}
#endif
#endif
