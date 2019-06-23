#ifndef	FUNCTION_H
#define	FUNCTION_H

typedef	struct	Function_TriangleOscillator	Function_TriangleOscillator;
typedef	struct	Function_Ramp				Function_Ramp;
typedef struct	Function_Sampled			Function_Sampled;

typedef struct	Function_RTFunction			Function_RTFunction;

Function_RTFunction *	Function_RTFunctionCreateTriangleOscillator(
	float	Period,
	float	RangeMin,
	float	RangeMax
	);

Function_RTFunction *	Function_RTFunctionCreateLinearRamp(
	float	Duration,
	float	RangeMin,
	float	RangeMax
	);

Function_RTFunction *	Function_RTFunctionCreateSampled(
	float *	SampledValues,
	int		SampledValueCount,
	float	Duration,
	float	RangeMin,
	float	RangeMax
	);

void	Function_RTFunctionDestroy(Function_RTFunction *Func);

int		Function_RTFunctionHasDecayed(Function_RTFunction *Func);

void	Function_RTFunctionReset(Function_RTFunction *Func);

float	Function_RTFunctionValue(Function_RTFunction *Func);

Function_Ramp *	Function_RampCreate(void);

void	Function_RampDestroy(Function_Ramp *ramp);

void	Function_RampSetRange(Function_Ramp *ramp, long range);

void	Function_RampReset(Function_Ramp *ramp);

float	Function_RampValue(Function_Ramp *ramp);

int		Function_RampIsClamped(Function_Ramp *ramp);

Function_TriangleOscillator *Function_TriangleOscillatorCreate(void);

void Function_TriangleOscillatorDestroy(Function_TriangleOscillator *osc);

float Function_TriangleOscillatorValue(Function_TriangleOscillator *osc);

void Function_TriangleOscillatorSetPeriod(
	Function_TriangleOscillator *	osc,
	long 							milliseconds);

void Function_TriangleOscillatorReset(Function_TriangleOscillator *osc);

Function_Sampled *	Function_SampledCreate(
	float *	SampledValues,
	int 	SampledValueCount);

void Function_SampledDestroy(Function_Sampled *Func);

void Function_SampledSetDomain(Function_Sampled *Func, float DomainMin, float DomainMax);
void Function_SampledSetRange(Function_Sampled *Func, float RangeMin, float RangeMax);
float Function_SampledValue(Function_Sampled *Func, float X);
int	Function_SampledInDomain(Function_Sampled *sf, float X);

#endif

