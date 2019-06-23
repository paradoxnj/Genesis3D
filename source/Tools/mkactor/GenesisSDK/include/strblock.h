// geStrBlock
#ifndef GE_STRBLOCK_H
#define GE_STRBLOCK_H

#include "basetype.h"	// geBoolean
#include "vfile.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct geStrBlock geStrBlock;

geStrBlock *GENESISCC geStrBlock_Create(void);
void GENESISCC geStrBlock_Destroy(geStrBlock **SB);

geBoolean GENESISCC geStrBlock_Append(geStrBlock **ppSB,const char *String);

void GENESISCC geStrBlock_Delete(geStrBlock **ppSB,int Nth);

const char *GENESISCC geStrBlock_GetString(const geStrBlock *SB, int Index);

//geBoolean GENESISCC geStrBlock_SetString(geStrBlock **ppSB, int Index, const char *String);
//geBoolean GENESISCC geStrBlock_Insert(geStrBlock **ppSB,int InsertAfterIndex,const char *String);

geBoolean GENESISCC geStrBlock_FindString(const geStrBlock* pSB, const char* String, int* pIndex);

int GENESISCC geStrBlock_GetCount(const geStrBlock *SB);
int GENESISCC geStrBlock_GetChecksum(const geStrBlock *SB);

geStrBlock* GENESISCC geStrBlock_CreateFromFile(geVFile* pFile);
geBoolean GENESISCC geStrBlock_WriteToFile(const geStrBlock *SB, geVFile *pFile);
geBoolean GENESISCC geStrBlock_WriteToBinaryFile(const geStrBlock *SB,geVFile *pFile);

#ifdef __cplusplus
}
#endif

#endif