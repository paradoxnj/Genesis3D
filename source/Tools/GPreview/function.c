#define	WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma warning ( disable : 4201 4214 )
#include <mmsystem.h>
#pragma warning ( default : 4201 4214 )
#include	<math.h>
#include	<stdlib.h>
#include	<assert.h>
#include	"function.h"
#pragma warning ( disable : 4514 )

typedef	struct	Function_TriangleOscillator
{
	long		toStartTime;
	long		toPeriod;
	long		toHalfPeriod;
}	Function_TriangleOscillator;

typedef	struct	Function_Ramp
{
	long		rStartTime;
	long		rRange;
}	Function_Ramp;

#define	NUMSAMPLEDPOINTS	100

typedef	struct	Function_Sampled
{
	float	sfDomainMin;
	float	sfDomainMax;
	float	sfRangeMin;
	float	sfRangeMax;
	float *	sfSampledValues;
	int		sfSampledValueCount;
}	SampFunc;

typedef	enum
{
	RTFUNC_TRIANGLE_OSCILLATOR,
	RTFUNC_LINEAR_RAMP,
	RTFUNC_SAMPLED,
}	RTFunctionKind;

typedef	struct	Function_RTFunction
{
	RTFunctionKind	rtfKind;
	union	{
		Function_TriangleOscillator *	rtfTriangleOscillator;
		Function_Ramp *					rtfLinearRamp;
		Function_Sampled *				rtfSampled;
	}	f;
	long			rtfStartTime;
}	Function_RTFunction;

Function_RTFunction *	Function_RTFunctionCreateTriangleOscillator(
	float	Period,
	float	RangeMin,
	float	RangeMax
	)
{
	Function_RTFunction *	Func;

#pragma todo ("Add range support on TriangleOscillator")
if	(RangeMin != 0.0f)
	return NULL;
if	(RangeMax != 1.0f)
	return NULL;

	Func = malloc(sizeof(*Func));
	if	(!Func)
		return Func;

	Func->rtfKind = RTFUNC_TRIANGLE_OSCILLATOR;

	Func->f.rtfTriangleOscillator = Function_TriangleOscillatorCreate();
	if	(!Func->f.rtfTriangleOscillator)
	{
		free(Func);
		return NULL;
	}

	Function_TriangleOscillatorSetPeriod(Func->f.rtfTriangleOscillator, (long)(Period * 1000.0f));

	Function_RTFunctionReset(Func);

	return Func;
}

Function_RTFunction *	Function_RTFunctionCreateLinearRamp(
	float	Duration,
	float	RangeMin,
	float	RangeMax
	)
{
	Function_RTFunction *	Func;

#pragma todo ("Add range support on LinearRamp")
if	(RangeMin != 0.0f)
	return NULL;
if	(RangeMax != 1.0f)
	return NULL;

	Func = malloc(sizeof(*Func));
	if	(!Func)
		return Func;

	Func->rtfKind = RTFUNC_LINEAR_RAMP;

	Func->f.rtfLinearRamp = Function_RampCreate();
	if	(!Func->f.rtfLinearRamp)
	{
		free(Func);
		return NULL;
	}

	Function_RampSetRange(Func->f.rtfLinearRamp, (long)(Duration * 1000.0f));

	Function_RTFunctionReset(Func);

	return Func;
}

Function_RTFunction *	Function_RTFunctionCreateSampled(
	float *	SampledValues,
	int		SampledValueCount,
	float	Duration,
	float	RangeMin,
	float	RangeMax
	)
{
	Function_RTFunction *	Func;

#pragma todo ("Add range support on Sampled")
if	(RangeMin != 0.0f)
	return NULL;
if	(RangeMax != 1.0f)
	return NULL;

	Func = malloc(sizeof(*Func));
	if	(!Func)
		return Func;

	Func->rtfKind = RTFUNC_SAMPLED;

	Func->f.rtfSampled = Function_SampledCreate(SampledValues, SampledValueCount);
	if	(!Func->f.rtfSampled)
	{
		free(Func);
		return NULL;
	}

	Function_SampledSetDomain(Func->f.rtfSampled, 0.0f, Duration);
	Function_SampledSetRange( Func->f.rtfSampled, RangeMin,  RangeMax);

	Function_RTFunctionReset(Func);

	return Func;
}

void	Function_RTFunctionDestroy(Function_RTFunction *Func)
{
	switch	(Func->rtfKind)
	{
	case	RTFUNC_TRIANGLE_OSCILLATOR:
		Function_TriangleOscillatorDestroy(Func->f.rtfTriangleOscillator);
		break;

	case	RTFUNC_LINEAR_RAMP:
		Function_RampDestroy(Func->f.rtfLinearRamp);
		break;

	case	RTFUNC_SAMPLED:
		Function_SampledDestroy(Func->f.rtfSampled);
		break;

	default:
		assert(!"Illegal function kind");
		break;
	}

	free(Func);
}

int		Function_RTFunctionHasDecayed(Function_RTFunction *Func)
{
	switch	(Func->rtfKind)
	{
	long	time;

	case	RTFUNC_TRIANGLE_OSCILLATOR:
		return 0;

	case	RTFUNC_LINEAR_RAMP:
		return Function_RampIsClamped(Func->f.rtfLinearRamp);

	case	RTFUNC_SAMPLED:
		time = timeGetTime();
		return Function_SampledInDomain(Func->f.rtfSampled,
										 (float)(time - Func->rtfStartTime) / 1000.0f);

	default:
		assert(!"Illegal function kind");
		return 1;
	}
}

void	Function_RTFunctionReset(Function_RTFunction *Func)
{
	Func->rtfStartTime = timeGetTime();
}

float	Function_RTFunctionValue(Function_RTFunction *Func)
{
	switch	(Func->rtfKind)
	{
	case	RTFUNC_TRIANGLE_OSCILLATOR:
		return Function_TriangleOscillatorValue(Func->f.rtfTriangleOscillator);

	case	RTFUNC_LINEAR_RAMP:
		return Function_RampValue(Func->f.rtfLinearRamp);

	case	RTFUNC_SAMPLED:
		return Function_SampledValue(Func->f.rtfSampled,
								     (float)(timeGetTime() - Func->rtfStartTime) / 1000.0f);

	default:
		assert(!"Illegal function kind");
		return 0.0f;
	}
}

Function_Ramp *	Function_RampCreate(void)
{
	Function_Ramp *	ramp;

	ramp = malloc(sizeof(*ramp));
	if	(!ramp)
		return ramp;
	Function_RampSetRange(ramp, 1000);
	Function_RampReset(ramp);

	return ramp;
}

void	Function_RampDestroy(Function_Ramp *ramp)
{
	free(ramp);
}

void	Function_RampSetRange(Function_Ramp *ramp, long range)
{
	ramp->rRange = range;
}

void	Function_RampReset(Function_Ramp *ramp)
{
	ramp->rStartTime = timeGetTime();
}

int		Function_RampIsClamped(Function_Ramp *ramp)
{
	long	ticks;

	ticks = timeGetTime() - ramp->rStartTime;
	if	(ticks > ramp->rRange)
		return 1;

	return 0;
}

float	Function_RampValue(Function_Ramp *ramp)
{
	long	ticks;

	ticks = timeGetTime() - ramp->rStartTime;
	if	(ticks > ramp->rRange)
		return 0.0f;

	return (float)ticks / (float)ramp->rRange;
}

Function_TriangleOscillator *	Function_TriangleOscillatorCreate(void)
{
	Function_TriangleOscillator *	osc;

	osc = malloc(sizeof(*osc));
	if	(!osc)
		return osc;

	Function_TriangleOscillatorSetPeriod(osc, 1000);
	Function_TriangleOscillatorReset(osc);

	return osc;
}

void Function_TriangleOscillatorDestroy(Function_TriangleOscillator *osc)
{
	free(osc);
}

float	Function_TriangleOscillatorValue(Function_TriangleOscillator *osc)
{
	long	ticks;

	ticks = (timeGetTime() - osc->toStartTime) % osc->toPeriod;
	if	(ticks > osc->toHalfPeriod)
	{
		return 1.0f - (float)(ticks - osc->toHalfPeriod) / (float)osc->toHalfPeriod;
	}
	else
	{
		return (float)ticks / (float)osc->toHalfPeriod;
	}
}

void	Function_TriangleOscillatorSetPeriod(Function_TriangleOscillator *osc, long milliseconds)
{
	osc->toPeriod = milliseconds;
	osc->toHalfPeriod = milliseconds / 2;
}

void	Function_TriangleOscillatorReset(Function_TriangleOscillator *osc)
{
	osc->toStartTime = timeGetTime();
}

Function_Sampled *	Function_SampledCreate(
	float *	SampledValues,
	int 	SampledValueCount)
{
	Function_Sampled *	func;

	func = malloc(sizeof(*func));
	if	(!func)
		return func;

	func->sfSampledValues = SampledValues;
	func->sfSampledValueCount = SampledValueCount;
	func->sfDomainMin = 0.0f;
	func->sfDomainMax = 1.0f;
	func->sfRangeMin = 0.0f;
	func->sfRangeMax = 1.0f;

	return func;
}

void Function_SampledDestroy(Function_Sampled *Func)
{
	free(Func);
}

void Function_SampledSetDomain(Function_Sampled *Func, float DomainMin, float DomainMax)
{
	Func->sfDomainMin = DomainMin;
	Func->sfDomainMax = DomainMax;
}

void Function_SampledSetRange(Function_Sampled *Func, float RangeMin, float RangeMax)
{
	Func->sfRangeMin = RangeMin;
	Func->sfRangeMax = RangeMax;
}

int	Function_SampledInDomain(Function_Sampled *sf, float X)
{
	if	(X < sf->sfDomainMin)
		return 1;
	if	(X > sf->sfDomainMax)
		return 1;

	return 0;
}

float Function_SampledValue(Function_Sampled *sf, float X)
{
	int		idx;

//	assert(xValue >= sf->sfDomainMin);
//	assert(xValue <= sf->sfDomainMax);
#pragma todo("Clean up domain/range support on Sampled functions")
	if	(X < sf->sfDomainMin)
		return sf->sfRangeMin;
	if	(X > sf->sfDomainMax)
		return sf->sfRangeMin;

	idx = (int)(fabs(X / (sf->sfDomainMax - sf->sfDomainMin)) * (float)(sf->sfSampledValueCount - 1));
	assert(idx >= 0);
	assert(idx < sf->sfSampledValueCount);

	return sf->sfRangeMin + sf->sfSampledValues[idx] * (sf->sfRangeMax - sf->sfRangeMin);
}

