
#include "Smoke.h"
//#include "Fuzz.h"
#include "Fire.h"
#include "Plasma.h"
//#include "FlipBook.h"
#include "BumpMap.h"
//#include "AlphaFly.h"
#include "Particles.h"
#include "Water.h"
#include "ElectricFx.h"
#include "setformat.h"

typedef Procedural_Table * (*GetProceduralFunc)(void);

GetProceduralFunc GetProceduralFunctions[] = {
	Smoke_GetProcedural_Table,
	Fire_GetProcedural_Table,
	Plasma_GetProcedural_Table,
//	Fuzz_GetProcedural_Table,
//	FlipBook_GetProcedural_Table,
	BumpMap_GetProcedural_Table,
//	AlphaFly_GetProcedural_Table,
	Particles_GetProcedural_Table,
	Water_GetProcedural_Table,
	ElectricFx_GetProcedural_Table,
	SetFormat_GetProcedural_Table,
	nullptr
};

int NumGetProceduralFunctions = (sizeof(GetProceduralFunctions)/sizeof(GetProceduralFunc)) - 1;

