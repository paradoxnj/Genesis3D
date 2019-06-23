/****************************************************************************************/
/*  MKBODY.C	                                                                        */
/*                                                                                      */
/*  Author: Stephen Balkum	                                                            */
/*  Description: Body construction from MAX export and textures.						*/
/*                                                                                      */
/*  The contents of this file are subject to the Genesis3D Public License               */
/*  Version 1.01 (the "License"); you may not use this file except in                   */
/*  compliance with the License. You may obtain a copy of the License at                */
/*  http://www.genesis3d.com                                                            */
/*                                                                                      */
/*  Software distributed under the License is distributed on an "AS IS"                 */
/*  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See                */
/*  the License for the specific language governing rights and limitations              */
/*  under the License.                                                                  */
/*                                                                                      */
/*  The Original Code is Genesis3D, released March 25, 1999.                            */
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include <assert.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "body.h"
#include "quatern.h"
#include "ram.h"
#include "strblock.h"
#include "vph.h"
#include "mkbody.h"
#include "bitmap.h"

#include "maxmath.h"

float VERT_EQUALITY_TOLERANCE = 0.005f; // obtained empirically
int fCount =0;		// face count for display

#define MK_PI 3.141592654f

#define LINE_LENGTH 1000 // maximum characters read in a line of a text file

typedef struct MkBody_Options
{
	char TexturePath[_MAX_PATH];
	char NFOFile[_MAX_PATH];
	char VPHFile[_MAX_PATH];
	MK_Boolean Capitalize;
	MK_Boolean WriteTextVPH;
	MK_Boolean RotationInBody;
	char BodyFile[_MAX_PATH];
	geVec3d EulerAngles;
	geStrBlock *ExtraMaterials; 
} MkBody_Options;


// Use this to initialize the default settings
const static MkBody_Options DefaultOptions = 
{
	"",
	"",
	"",
	MK_FALSE,
	MK_FALSE,
	MK_FALSE,
	"",
	{ 0.0f, 0.0f, 0.0f},
	NULL
};

#define NAME_LENGTH 256

typedef struct
{
	char Name[NAME_LENGTH];
	int Index;
	int ParentID;
	geXForm3d Matrix;
} BoneDetail;

typedef struct
{
	char Name[NAME_LENGTH];
	geXForm3d NodeTM;
	geQuaternion Q;
	geVec3d S;
	geVec3d T;
} NodeDetail;

typedef struct
{
	geVec3d v;
	geVec3d offsetv;
	int bone;
} V2VertexDetail;

typedef struct
{
	float tu, tv;
	int NAN;
} V2TVertexDetail;

typedef struct
{
	int material;
	int smoothingGroup;
	int v[3];
	int tv[3];
} V2FaceDetail;

typedef struct 
{
	geVec3d v;						// location
	geVec3d n;						// normal
	float tu, tv;					// texture coords
	int bone;						// bone index
} VertexDetail;

typedef struct 
{
	int material;					// material index
	VertexDetail verts[3];			// vertex info
} FaceDetail;

typedef struct 
{
	int bone;						// bone for this vertex
	geVec3d BSPoint;					// offset in bone space
	geVec3d WSPoint;					// location in world space
} VPHVertex;

typedef struct
{
	geXForm3d Matrix;
	char Name[NAME_LENGTH];
} VPHLink;

typedef struct
{
	int NumLinks;
	VPHLink* pLinks;
	int NumObjects;
	geStrBlock* pSBObjectNames;
	int* NumVerts;
	VPHVertex** ppVerts;
} VPHData;

MkUtil_Printf Printf;

MK_Boolean FileStringSeek(char* pDest, const char* pSearchStr, FILE* fp)
{
	char* p;

	do
	{
		p = fgets(pDest, LINE_LENGTH, fp);
	}
	while( (strstr(pDest, pSearchStr) == NULL) && (p == pDest) );

	return (p == pDest) ? MK_TRUE : MK_FALSE;
}

void StripNewLine(char* pString)
{
	while(*pString != 0)
	{
		if(*pString == '\n')
			*pString = 0;
		pString++;
	}
}

void NameCopyNoNewline(char* pDest, const char* pSrc)
{
	strncpy(pDest, pSrc, NAME_LENGTH);

	StripNewLine(pDest);
}

void FreeVPHData(VPHData** ppVPHData)
{
	int j;
	VPHData* pVPHData;
	
	assert(ppVPHData != NULL);
	pVPHData = *ppVPHData;

	if(pVPHData != NULL)
	{
		geStrBlock_Destroy(&pVPHData->pSBObjectNames);
		for(j=0;j<pVPHData->NumObjects;j++)
		{
			geRam_Free(pVPHData->ppVerts[j]);
		}
		geRam_Free(pVPHData->NumVerts);
		geRam_Free(pVPHData->ppVerts);
		geRam_Free(pVPHData->pLinks);

		geRam_Free(*ppVPHData);
	}
}

ReturnCode ReadVPHData(MkBody_Options* options, FILE* fp, VPHData* pVPHData)
{
	ReturnCode retValue = RETURN_SUCCESS;
	VPHVertex* pVPHVerts = NULL;
	VPHVertex** ppVerts = NULL;
	VPHLink* pVPHLinks = NULL;
	char line[LINE_LENGTH];
	int NumLinks, NumObjects;
	geStrBlock* pSBNames;
	int* pNumVerts = NULL;
	int i, j;
	char* ptext;

	assert(pVPHData != NULL);

	if(FileStringSeek(line, "Number of links", fp) == MK_FALSE)
	{
		Printf("ERROR: Could not find links in VPH file\n");
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		return retValue;
	}

	j = sscanf(line, "Number of links = %d\n", &NumLinks);
	if (j != 1)
		{
			Printf("ERROR: Could not read number of links from VPH file\n");
			Printf("Line from VPH file:\n%s\n",line);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			return retValue;
		}
		
	pVPHLinks = GE_RAM_ALLOCATE_ARRAY(VPHLink, NumLinks);
	if(pVPHLinks == NULL)
	{
		Printf("ERROR: Could not allocate VPH links\n");
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		return retValue;
	}

	for(i=0;i<NumLinks;i++)
	{
		if(FileStringSeek(line, "NAME =", fp) == MK_FALSE)
		{
			Printf("ERROR: Could not find link %d in VPH file\n", i);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			break;
		}

		NameCopyNoNewline(pVPHLinks[i].Name, strchr(line, '=') + 2);
		if(options->Capitalize != MK_FALSE)
			strupr(pVPHLinks[i].Name);

		if(FileStringSeek(line, "Matrix ZM", fp) == MK_FALSE)
		{
			Printf("ERROR: Could not find link %d's matrix in VPH file\n", i);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			break;
		}

		j = fscanf(fp, "%f %f %f\n",	&pVPHLinks[i].Matrix.AX,
										&pVPHLinks[i].Matrix.AY,
										&pVPHLinks[i].Matrix.AZ);
		if(j != 3)
		{
			Printf("ERROR: Could not read link %d's matrix in VPH file\n", i);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			break;
		}
		j = fscanf(fp, "%f %f %f\n",	&pVPHLinks[i].Matrix.BX,
										&pVPHLinks[i].Matrix.BY,
										&pVPHLinks[i].Matrix.BZ);
		if(j != 3)
		{
			Printf("ERROR: Could not read link %d's matrix in VPH file\n", i);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			break;
		}
		j = fscanf(fp, "%f %f %f\n",	&pVPHLinks[i].Matrix.CX,
										&pVPHLinks[i].Matrix.CY,
										&pVPHLinks[i].Matrix.CZ);
		if(j != 3)
		{
			Printf("ERROR: Could not read link %d's matrix in VPH file\n", i);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			break;
		}
		j = fscanf(fp, "%f %f %f\n",	&pVPHLinks[i].Matrix.Translation.X,
										&pVPHLinks[i].Matrix.Translation.Y,
										&pVPHLinks[i].Matrix.Translation.Z);
		if(j != 3)
		{
			Printf("ERROR: Could not read link %d's matrix in VPH file\n", i);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			break;
		}
	}
	if(i != NumLinks)
	{
		// there must have been an error
		geRam_Free(pVPHLinks);
		return retValue;
	}

	// setup for reading the verts
	if(FileStringSeek(line, "num_objects = ", fp) == MK_FALSE)
	{
		Printf("ERROR: Could not find links in VPH file\n");
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		geRam_Free(pVPHLinks);
		return retValue;
	}

	if(sscanf(line, "num_objects = %d\n", &NumObjects) != 1)
	{
		Printf("ERROR: Could not read number of objects in VPH file\n");
		Printf("Line from VPH file:\n%s\n",line);
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		geRam_Free(pVPHLinks);
		return retValue;
	}

	// setup for object names
	pSBNames = geStrBlock_Create();
	if(pSBNames == NULL)
	{
		Printf("ERROR: Could not allocate VPH string block\n");
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		geRam_Free(pVPHLinks);
		return retValue;
	}

	// Setup for parent link indexes
	pNumVerts = GE_RAM_ALLOCATE_ARRAY(int, NumObjects);
	if(pNumVerts == NULL)
	{
		Printf("ERROR: Could not allocate vert count data\n");
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		geRam_Free(pVPHLinks);
		return retValue;
	}
	ppVerts = GE_RAM_ALLOCATE_ARRAY(VPHVertex*, NumObjects);
	if(ppVerts == NULL)
	{
		Printf("ERROR: Could not allocate vert list array\n");
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		geRam_Free(pVPHLinks);
		geRam_Free(pNumVerts);
		return retValue;
	}

	for(i=0;i<NumObjects;i++)
	{
		if(FileStringSeek(line, "NAME =", fp) == MK_FALSE)
		{
			Printf("ERROR: Could not find object %d in VPH file\n", i);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			break;
		}

		StripNewLine(line);
		if(options->Capitalize != MK_FALSE)
			strupr(line);
		if(geStrBlock_Append(&pSBNames, line + strlen("NAME = ")) == GE_FALSE)
		{
			Printf("ERROR: Could not append vph object %d string\n", i);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			break;
		}

		if(FileStringSeek(line, "Number of vertices = ", fp) == MK_FALSE)
		{
			Printf("ERROR: Could not find object %d verts in VPH file\n", i);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			break;
		}

		if(sscanf(line, "Number of vertices = %d", &pNumVerts[i]) != 1)
		{
			Printf("ERROR: Could not read number of verts in VPH file\n");
			Printf("Line from VPH file:\n%s\n",line);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			break;
		}

		pVPHVerts = GE_RAM_ALLOCATE_ARRAY(VPHVertex, pNumVerts[i]);
		if(pVPHVerts == NULL)
		{
			Printf("ERROR: Could not allocate vert data\n");
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			break;
		}
		ppVerts[i] = pVPHVerts;

		for(j=0;j<pNumVerts[i];j++)
		{
			if(FileStringSeek(line, "rigid (", fp) == MK_FALSE)
			{
				Printf("ERROR: Could not read rigid vert %d\n", j);
				Printf("Line from VPH file:\n%s\n",line);
				MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
				break;
			}

			if(sscanf(line, "  rigid (%d", &pVPHVerts[j].bone) != 1)
			{
				Printf("ERROR: Could not read rigid vert %d link\n", j);
				Printf("Line from VPH file:\n%s\n",line);
				MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
				break;
			}

#pragma message ("need to test for rigid connections by looking at deform_link")
			/*
			// non-positive bone is a rigid bone and a good thing
			if(pVPHVerts[j].bone > 0)
			{
				Printf("ERROR: All links should be root or rigid\n");
				MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
				break;
			}
			pVPHVerts[j].bone = -pVPHVerts[j].bone;
			*/

			ptext = strrchr(line, ')');
			if(ptext == NULL)
			{
				Printf("ERROR: Could not find rigid vert %d xyz\n", j);
				MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
				break;
			}
			ptext++;

			if(sscanf(ptext, "%f %f %f", &pVPHVerts[j].BSPoint.X, &pVPHVerts[j].BSPoint.Y, &pVPHVerts[j].BSPoint.Z) != 3)
			{
				Printf("ERROR: Could not read rigid vert %d xyz\n", j);
				Printf("Line from VPH file:\n%s\n",line);
				MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
				break;
			}
		}
		if(j != pNumVerts[i])
		{
			// there must have been an error
			break;
		}

	}
	if(i != NumObjects)
	{
		// there must have been an error
		geStrBlock_Destroy(&pSBNames);
		for(j=0;j<i;j++)
		{
			geRam_Free(ppVerts[j]);
		}
		geRam_Free(pNumVerts);
		geRam_Free(ppVerts);
		geRam_Free(pVPHLinks);
		return retValue;
	}

	pVPHData->NumLinks = NumLinks;
	pVPHData->pLinks = pVPHLinks;
	pVPHData->NumObjects = NumObjects;
	pVPHData->pSBObjectNames = pSBNames;
	pVPHData->NumVerts = pNumVerts;
	pVPHData->ppVerts = ppVerts;

	return retValue;
}

MK_Boolean AddBones(MkBody_Options* options, geBody* pBody, const int NumBones, BoneDetail* pBoneIDs, const int NumNodes, const NodeDetail* pNodes)
{
	int j, k;
	geXForm3d matrix, parentmatrix;
	geQuaternion gQ;
	geVec3d v;

	for(k=0;k<NumBones;k++)
	{
		for(j=0;j<NumNodes;j++)
		{
			if(strcmp(pBoneIDs[k].Name, pNodes[j].Name) == 0)
				break;
		}
		if(j == NumNodes)
		{
			Printf("ERROR: Could not find '%s' node\n", pBoneIDs[k].Name);
			return MK_FALSE;
		}

		// want to strip the scale component
		geQuaternion_ToMatrix(&pNodes[j].Q, &matrix);
		matrix.Translation = pNodes[j].T;

		if(pBoneIDs[k].ParentID != -1)
		{
			parentmatrix = pBoneIDs[pBoneIDs[k].ParentID].Matrix;
		}
		else
		{
			geXForm3d_SetIdentity(&parentmatrix);

			// Root is identity by definition
			//geXForm3d_SetIdentity(&matrix);
		}

		// store world space matrix for later use
		pBoneIDs[k].Matrix = matrix;

		// divide out the parent
		MaxMath_InverseMultiply(&matrix, &parentmatrix, &matrix);

		// flip the rotation
		geQuaternion_FromMatrix(&matrix, &gQ);
		gQ.W = -gQ.W;
		v = matrix.Translation;
		geQuaternion_ToMatrix(&gQ, &matrix);
		matrix.Translation = v;
		geXForm3d_Orthonormalize(&matrix);

		// To support bvh format, the body must not have a rotational
		// attachment.  This can be pushed into the motion.  Thus, the
		// attachment is just the local-space translation.
		if(options->RotationInBody == MK_FALSE)
			geXForm3d_SetTranslation(&matrix, v.X, v.Y, v.Z);

		// get parent's index
		j = pBoneIDs[k].ParentID;
		if(j == -1)
			j = GE_BODY_NO_PARENT_BONE;
		else
			j = pBoneIDs[j].Index;

		// attach the bone
		if(geBody_AddBone(pBody, j, pBoneIDs[k].Name, &matrix, &pBoneIDs[k].Index) == GE_FALSE)
		{
			Printf("ERROR: Bone '%s' not added\n", pBoneIDs[k].Name);
			return MK_FALSE;
		}
	}

	return MK_TRUE;
}

MK_Boolean CalculateWSVPHVerts(VPHData* pVPHData, const int NumNodes, const NodeDetail* pNodes)
{
	int i, j, k;
	VPHVertex* pVerts;
	geXForm3d matrix, nodematrix;

	for(k=0;k<pVPHData->NumObjects;k++)
	{
		pVerts = pVPHData->ppVerts[k];

		for(j=0;j<pVPHData->NumVerts[k];j++)
		{
			if(pVerts[j].bone >= pVPHData->NumLinks)
			{
				Printf("ERROR: VPH link index out of range\n");
				return MK_FALSE;
			}

			matrix = pVPHData->pLinks[pVerts[j].bone].Matrix;

			for(i=0;i<NumNodes;i++)
			{
				if(strcmp(pNodes[i].Name, pVPHData->pLinks[pVerts[j].bone].Name) == 0)
				{
					geQuaternion_ToMatrix(&pNodes[i].Q, &nodematrix);
					nodematrix.Translation = pNodes[i].T;
					break;
				}
			}
			if(i == NumNodes)
			{
				Printf("ERROR: Could not match link with node\n");
				return MK_FALSE;
			}

			geXForm3d_Multiply(&matrix, &nodematrix, &matrix);

			geXForm3d_Transform(&matrix, &pVerts[j].BSPoint, &pVerts[j].WSPoint);
		}
	}

	return MK_TRUE;
}

ReturnCode CalculateVertexNormal(int Index, int Group, int NumFaces, 
		V2FaceDetail* pFaces, V2VertexDetail* pVerts, geVec3d* pNormal)
{
	ReturnCode retCode = RETURN_SUCCESS;
	int j;
	int nNumNormals;
	int AnyNormals = 0;

	//-------------------------------------------------------------------
	// Calculate Vertex normals

	geVec3d Normal;
	geVec3d A, B;
	geVec3d v={0.0f,0.0f,0.0f};
	geVec3d ThisNormal={0.0f,0.0f,0.0f};

	// Average the normals for all common faces in the same smoothing group
	nNumNormals = 0;
	
	geVec3d_Clear(&Normal);
	Normal.X = 1.0f;
	*pNormal = Normal;

	for(j=0;j<NumFaces;j++)
	{
		if (MkUtil_Interrupt())
			{
				Printf("Interrupted\n");
				MkUtil_AdjustReturnCode(&retCode, RETURN_ERROR);
				return(RETURN_ERROR);
			}

				
		if(pFaces[j].smoothingGroup == Group)
		{
			if( (pFaces[j].v[0] == Index) || 
				(pFaces[j].v[1] == Index) || 
				(pFaces[j].v[2] == Index) )
			{
				AnyNormals = 1;
				A = pVerts[pFaces[j].v[2]].v;
				B = pVerts[pFaces[j].v[0]].v;
				v = pVerts[pFaces[j].v[1]].v;
				geVec3d_Subtract(&A, &v, &A);
				geVec3d_Subtract(&B, &v, &B);
				geVec3d_CrossProduct(&A, &B, &ThisNormal);

				#if 0
				if(geVec3d_Length(&ThisNormal) < VERT_EQUALITY_TOLERANCE)
					{
						MkUtil_AdjustReturnCode(&retCode, RETURN_WARNING);
						Printf("WARNING: degenerate face: %f %f %f\n", v.X, v.Y, v.Z);
						geVec3d_Clear(&ThisNormal);
						ThisNormal.X = 1.0f;
					}
				else
				#endif
					{
						geVec3d_Add(&Normal, &ThisNormal, &Normal);
						nNumNormals++;
					}
			}
		}
	}

	if(nNumNormals == 0) 
	{
		Printf("ERROR: There is a vertex not connected to a face.\n");
		MkUtil_AdjustReturnCode(&retCode, RETURN_ERROR);
		return(RETURN_ERROR);
	}

	if (geVec3d_Normalize(&Normal)==0.0f)
		{
			// It is possible for a sum of normals to be zero.  For example, the corner
			// of a piece of paper.  Just pick the last found normal and spit out a
			// warning.
			MkUtil_AdjustReturnCode(&retCode, RETURN_WARNING);
			Printf("WARNING: Vertex normal calculated to zero: %f %f %f\n", v.X, v.Y, v.Z);
			Normal = ThisNormal;
		}

	if (geVec3d_Normalize(&Normal)==0.0f)
		{
			MkUtil_AdjustReturnCode(&retCode, RETURN_WARNING);
			Printf("WARNING: Tried to find a non-zero normal from local geometry, but failed.\n");
			geVec3d_Clear(&Normal);
			Normal.X = 1.0f;
		}

	*pNormal = Normal;

	return(retCode);
}

ReturnCode V2ReadVertices(MkBody_Options* options, FILE* fp, geBody* pBody, char* name, int NumVertices, V2VertexDetail* pVerts, VPHData* pvphdata, NodeDetail* pNodes, int NumNodes)
{
	char line[LINE_LENGTH];
	char* ptext;
	int i, j, k;
	int n;
	geXForm3d dummyMatrix;
	geXForm3d xmatrix;
	geXForm3d euler;

	assert(pVerts != NULL);

	geXForm3d_SetEulerAngles(&euler, &options->EulerAngles);

	for(k=0;k<NumVertices;k++)
	{
		if (MkUtil_Interrupt())
			{
				Printf("Interrupted\n");
				return(RETURN_ERROR);
			}

		if(fgets(line, LINE_LENGTH, fp) == NULL)
		{
			Printf("ERROR: Could not read from '%s' NFO file\n", options->NFOFile);
			goto ReadVertexFailure;
		}

		i = sscanf(line, "%f %f %f %f %f %f",	&pVerts[k].v.X,
												&pVerts[k].v.Y,
												&pVerts[k].v.Z,
												&pVerts[k].offsetv.X,
												&pVerts[k].offsetv.Y,
												&pVerts[k].offsetv.Z);
		if(i != 6)
		{
			Printf("ERROR: Could not read vertex for mesh '%s' in '%s' NFO file\n", name, options->NFOFile);
			Printf("Line from VPH file:\n%s\n",line);
			goto ReadVertexFailure;
		}

		// apply rotation to all data at read time
		geXForm3d_Transform(&euler, &pVerts[k].v, &pVerts[k].v);
#pragma message ("vertex offsets not used now, but when they are used, they need to be rotated")

		// to get to the bone name, search for the 6th space
		ptext = line;
		for(i=0;i<6;i++)
		{
			ptext = strchr(ptext, ' ');
			if(ptext == NULL)
			{
				Printf("ERROR: Could not read bone vertex for mesh '%s' in '%s' NFO file\n", name, options->NFOFile);
				goto ReadVertexFailure;
			}
			ptext++;
		}
		StripNewLine(ptext);

		if(options->Capitalize != MK_FALSE)
			strupr(ptext);

		if(strcmp(ptext, "-1") == 0)
		{
			VPHVertex* pVPHVerts;
			if (pvphdata==NULL)
				{
					Printf("ERROR: VPH data needed (and not loaded) for mesh '%s'\n", name);
					Printf("--- possibly this mesh has not been physiqued:   '%s'\n", name);
					goto ReadVertexFailure;
				}
				
			if(geStrBlock_FindString(pvphdata->pSBObjectNames, name, &j) == GE_FALSE)
			{
				Printf("ERROR: Could not find vph data for mesh '%s'\n", name);
				goto ReadVertexFailure;
			}

			pVPHVerts = pvphdata->ppVerts[j];
			j = pvphdata->NumVerts[j] - 1;
			while(j >= 0)
			{
				if(geVec3d_Compare(&pVPHVerts[j].WSPoint, &pVerts[k].v, VERT_EQUALITY_TOLERANCE) != GE_FALSE)
					break; // found it!

				j--;
			}
			if(j < 0)
			{
				// didn't find vert
				Printf("ERROR: Could not find matching vertex\n");
				goto ReadVertexFailure;
			}

			for(n=0;n<NumNodes;n++)
			{
				if(strcmp(pNodes[n].Name, pvphdata->pLinks[pVPHVerts[j].bone].Name) == 0)
					break; // found it!
			}
			if(n == NumNodes)
			{
				// didn't find vert
				Printf("ERROR: (1) Could not find node '%s' for vertex\n", pvphdata->pLinks[pVPHVerts[j].bone].Name);
				goto ReadVertexFailure;
			}

			// copy bone name
			strcpy(ptext, pNodes[n].Name);
		}
		else
		{
			for(n=0;n<NumNodes;n++)
			{
				if(strcmp(pNodes[n].Name, ptext) == 0)
					break; // found it!
			}
			if(n == NumNodes)
			{
				// didn't find vert
				Printf("ERROR: (2) Could not find node '%s' for vertex\n", ptext);
				goto ReadVertexFailure;
			}
		}

		// gotta have a node at this point
		assert( (n >= 0) && (n < NumNodes) );

		if(geBody_GetBoneByName(pBody, ptext, &i, &dummyMatrix, &j) == GE_FALSE)
		{
			Printf("ERROR: Could not find bone '%s' for mesh '%s' in '%s' NFO file\n", ptext, name, options->NFOFile);
			goto ReadVertexFailure;
		}
		pVerts[k].bone = i;

		// Need to do a NoScale
		geQuaternion_ToMatrix(&pNodes[n].Q, &xmatrix);
		xmatrix.Translation = pNodes[n].T;

		MaxMath_GetInverse(&xmatrix, &xmatrix);

		// fixup the vert
		MaxMath_Transform(&xmatrix, &pVerts[k].v, &pVerts[k].offsetv);
	}

	return(RETURN_SUCCESS);

ReadVertexFailure:

	return(RETURN_ERROR);
}

ReturnCode V2ReadTextureVertices(MkBody_Options* options, FILE* fp, char* name, int NumTVertices, V2TVertexDetail* pTVerts)
{
	char line[LINE_LENGTH];
	int i, k;

	assert(pTVerts != NULL);

	for(k=0;k<NumTVertices;k++)
	{
		if(fgets(line, LINE_LENGTH, fp) == NULL)
		{
			Printf("ERROR: Could not read from '%s' NFO file\n", options->NFOFile);
			goto ReadTextureVertexFailure;
		}

		i = sscanf(line, "%f %f",	&pTVerts[k].tu,
									&pTVerts[k].tv);
		pTVerts[k].NAN = GE_FALSE;
		if(i != 2)
		{
			if (strstr(line,"NAN") != NULL)
				{
					pTVerts[k].tu = pTVerts[k].tv = 0.0f;
					pTVerts[k].NAN = GE_TRUE;
				}
			else
				{
					Printf("ERROR: Could not read texture uv for mesh '%s' in '%s' NFO file\n", name, options->NFOFile);
					Printf("       (expected %d total uv pairs.  failed to read uv pair for vertex %d\n)",NumTVertices,k);
					Printf("Line from VPH file:\n%s\n",line);
					goto ReadTextureVertexFailure;
				}
		}

		// Need to flip the v's for some reason
		pTVerts[k].tv = 1 - pTVerts[k].tv;
	}

	return(RETURN_SUCCESS);

ReadTextureVertexFailure:

	return(RETURN_ERROR);
}

ReturnCode V2ReadFaces(MkBody_Options* options, FILE* fp, char* name, int NumFaces, V2FaceDetail* pFaces)
{
	char line[LINE_LENGTH];
	int i, k;

	assert(pFaces != NULL);

	for(k=0;k<NumFaces;k++)
	{
		if (MkUtil_Interrupt())
			{
				Printf("Interrupted\n");
				return(RETURN_ERROR);
			}
		if(fgets(line, LINE_LENGTH, fp) == NULL)
		{
			Printf("ERROR: Could not read from '%s' NFO file\n", options->NFOFile);
			goto ReadFacesFailure;
		}

		if(sscanf(line, "Face %d", &i) != 1)
		{
			Printf("ERROR: Could not read face number for '%s' mesh from '%s' NFO file\n", name, options->NFOFile);
			Printf("Line from VPH file:\n%s\n",line);
			goto ReadFacesFailure;
		}

		if(fscanf(fp, "    %d\n", &pFaces->material) != 1)
		{
			Printf("ERROR: Could not read face material for '%s' mesh from '%s' NFO file\n", name, options->NFOFile);
			goto ReadFacesFailure;
		}

		if(fscanf(fp, "    %d\n", &pFaces->smoothingGroup) != 1)
		{
			Printf("ERROR: Could not read face group for '%s' mesh from '%s' NFO file\n", name, options->NFOFile);
			goto ReadFacesFailure;
		}

		for(i=0;i<3;i++)
		{
			if(fscanf(fp, "    %d %d\n", &pFaces->v[i], &pFaces->tv[i]) != 2)
			{
				Printf("ERROR: Could not read face indices for '%s' mesh from '%s' NFO file\n", name, options->NFOFile);
				goto ReadFacesFailure;
			}
		}

		pFaces++;
	}

	return(RETURN_SUCCESS);

ReadFacesFailure:

	return(RETURN_ERROR);
}

ReturnCode V2ReadAndAddMesh(MkBody_Options* options, FILE* fp, geBody* pBody, VPHData* pvphdata, NodeDetail* pNodes, int NumNodes)
{
	ReturnCode retValue = RETURN_SUCCESS;
	ReturnCode thisRetValue;
	char line[LINE_LENGTH];
	char name[NAME_LENGTH];
	char* ptext;
	const char* bonename;
	int NumVertices;
	int NumTVertices;
	int NumFaces;
	int i, j, k;
	V2VertexDetail* pVerts = NULL;
	V2TVertexDetail* pTVerts = NULL;
	V2FaceDetail* pFaces = NULL;
	const geVec3d* vertices[3];
	const V2TVertexDetail* tvertices[3];
	int bones[3];
	geVec3d v;
	geVec3d normals[3];
	geXForm3d dummyMatrix;
	geXForm3d xmatrix;

#define V2RAAM_SAFE_FGETS													\
if(fgets(line, LINE_LENGTH, fp) == NULL)									\
{																			\
	Printf("ERROR: Could not read from '%s' NFO file\n", options->NFOFile);	\
	MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);								\
	goto ReadMeshClean;														\
}

	// name of mesh
	V2RAAM_SAFE_FGETS;
	StripNewLine(line);

	ptext = strstr(line, ": ");
	if(ptext != NULL)
		ptext += 2; // length of ": "
	if( (ptext == NULL) || (strlen(ptext) < 1) )
	{
		// there should be some name-like text
		Printf("ERROR: Found no node name: '%s'\n", line);
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		goto ReadMeshClean;
	}
	strncpy(name, ptext, NAME_LENGTH);

	// Vertex List

	V2RAAM_SAFE_FGETS;
	if(strcmp(line, "Vertex List\n") != 0)
	{
		Printf("ERROR: No vertex list for '%s' mesh in '%s' NFO file\n", name, options->NFOFile);
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		goto ReadMeshClean;
	}

	V2RAAM_SAFE_FGETS;
	if(sscanf(line, "Number of Vertices = %d", &NumVertices) != 1)
	{
		Printf("ERROR: Could not read number of verticies for mesh '%s' in '%s' NFO file\n", name, options->NFOFile);
		Printf("Line from VPH file:\n%s\n",line);
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		goto ReadMeshClean;
	}

	pVerts = GE_RAM_ALLOCATE_ARRAY(V2VertexDetail, NumVertices);
	if(pVerts == NULL)
	{
		Printf("ERROR: Could not allocate vertex array for mesh '%s' in '%s' NFO file\n", name, options->NFOFile);
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		goto ReadMeshClean;
	}

	thisRetValue = V2ReadVertices(options, fp, pBody, name, NumVertices, pVerts, pvphdata, pNodes, NumNodes);
	MkUtil_AdjustReturnCode(&retValue, thisRetValue);
	if(thisRetValue == RETURN_ERROR)
	{
		goto ReadMeshClean;
	}

	// Texture Vertex List

	V2RAAM_SAFE_FGETS;
	if(strcmp(line, "Texture Vertex List\n") != 0)
	{
		Printf("ERROR: No texture vertex list for '%s' mesh in '%s' NFO file\n", name, options->NFOFile);
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		goto ReadMeshClean;
	}

	V2RAAM_SAFE_FGETS;
	if(sscanf(line, "Number of Texture Vertices = %d", &NumTVertices) != 1)
	{
		Printf("ERROR: Could not read number of texture verticies for mesh '%s' in '%s' NFO file\n", name, options->NFOFile);
		Printf("Line from VPH file:\n%s\n",line);
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		goto ReadMeshClean;
	}

	pTVerts = GE_RAM_ALLOCATE_ARRAY(V2TVertexDetail, NumTVertices);
	if(pTVerts == NULL)
	{
		Printf("ERROR: Could not allocate texture vertex array for mesh '%s' in '%s' NFO file\n", name, options->NFOFile);
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		goto ReadMeshClean;
	}

	thisRetValue = V2ReadTextureVertices(options, fp, name, NumTVertices, pTVerts);
	MkUtil_AdjustReturnCode(&retValue, thisRetValue);
	if(thisRetValue == RETURN_ERROR)
	{
		goto ReadMeshClean;
	}

	// Face List

	V2RAAM_SAFE_FGETS;
	if(strcmp(line, "Face List\n") != 0)
	{
		Printf("ERROR: No face list for '%s' mesh in '%s' NFO file\n", name, options->NFOFile);
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		goto ReadMeshClean;
	}

	V2RAAM_SAFE_FGETS;
	if(sscanf(line, "Number of Faces = %d", &NumFaces) != 1)
	{
		Printf("ERROR: Could not read number of faces for mesh '%s' in '%s' NFO file\n", name, options->NFOFile);
		Printf("Line from VPH file:\n%s\n",line);
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		goto ReadMeshClean;
	}

	pFaces = GE_RAM_ALLOCATE_ARRAY(V2FaceDetail, NumFaces);
	if(pFaces == NULL)
	{
		Printf("ERROR: Could not allocate face array for mesh '%s' in '%s' NFO file\n", name, options->NFOFile);
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		goto ReadMeshClean;
	}

	retValue = V2ReadFaces(options, fp, name, NumFaces, pFaces);
	if(retValue == RETURN_ERROR)
	{
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		goto ReadMeshClean;
	}

	// Add the faces

	for(i=0;i<NumFaces;i++)
	{
		for(j=0;j<3;j++)
		{
			if (MkUtil_Interrupt())
				{
					Printf("Interrupted\n");
					MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
					goto ReadMeshClean;
				}
			vertices[j] = &pVerts[pFaces[i].v[j]].offsetv;
			bones[j] = pVerts[pFaces[i].v[j]].bone;
			tvertices[j] = &pTVerts[pFaces[i].tv[j]];

			if (tvertices[j]->NAN == GE_TRUE)
				{	
					geVec3d v;
					v = pVerts[pFaces[i].v[j]].v;
					Printf("Error: Bad uv coordinates for mesh '%s' in '%s' NFO file\n", name,options->NFOFile);
					Printf("       (uv coordinates used in vertex at coordinates (%f,%f,%f))\n",v.X,v.Y,v.Z);
					MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
					goto ReadMeshClean;
				}

			thisRetValue = CalculateVertexNormal(	pFaces[i].v[j], 
													pFaces[i].smoothingGroup, 
													NumFaces, pFaces, 
													pVerts, 
													&v);
			MkUtil_AdjustReturnCode(&retValue, thisRetValue);
			if(thisRetValue == RETURN_ERROR)
			{
				Printf("ERROR: Could not calculate normal for mesh '%s' in '%s' NFO file\n", name, options->NFOFile);
				goto ReadMeshClean;
			}

			geBody_GetBone(pBody, bones[j], &bonename, &dummyMatrix, &k);
			for(k=0;k<NumNodes;k++)
			{
				if(strcmp(pNodes[k].Name, bonename) == 0)
					break; // found it!
			}
			if(k == NumNodes)
			{
				// didn't find vert
				Printf("ERROR: (3) Could not find node '%s' for vertex\n", bonename);
				MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
				goto ReadMeshClean;
			}

			// gotta have a node at this point
			assert( (k >= 0) && (k < NumNodes) );

			// Need to do a NoScale
			geQuaternion_ToMatrix(&pNodes[k].Q, &xmatrix);
			xmatrix.Translation = pNodes[k].T;

			MaxMath_GetInverse(&xmatrix, &xmatrix);

			// fixup the normal
			geVec3d_Clear(&xmatrix.Translation);
			MaxMath_Transform(&xmatrix, &v, &normals[j]);
		}

		if (pFaces[i].material<0)
			{
				MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
				Printf("ERROR: no material assigned to face %d, on mesh '%s' in '%s' NFO file\n", 
						i,name, options->NFOFile);
				goto ReadMeshClean;
			}		
		if (pFaces[i].material >= geBody_GetMaterialCount(pBody))
			{
				MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
				Printf("ERROR: bad material index for face %d, on mesh '%s' in '%s' NFO file\n",
						i,name, options->NFOFile);
				Printf("       (index was %d, and there are only %d materials)\n", 
						pFaces[i].material,geBody_GetMaterialCount(pBody));	
				goto ReadMeshClean;
			}
		fCount++;
		if (fCount%25==0)
			Printf("\t%d Faces Processed\n",fCount);


		if(geBody_AddFace(pBody,	vertices[0], &normals[0],
										tvertices[0]->tu, tvertices[0]->tv, 
										bones[0],
									vertices[1], &normals[1],
										tvertices[1]->tu, tvertices[1]->tv, 
										bones[1],
									vertices[2], &normals[2],
										tvertices[2]->tu, tvertices[2]->tv, 
										bones[2],
									pFaces[i].material) == GE_FALSE)
		{
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			Printf("ERROR: Could not add face for mesh '%s' in '%s' NFO file\n", name, options->NFOFile);
			goto ReadMeshClean;
		}
	}

ReadMeshClean:
	if(pFaces != NULL)
		geRam_Free(pFaces);
	if(pTVerts != NULL)
		geRam_Free(pTVerts);
	if(pVerts != NULL)
		geRam_Free(pVerts);

	return(retValue);
}


MkBody_Material(MkBody_Options *options, const char *line, geBody *pBody, int i)
{
	char* ptext;
	char name[NAME_LENGTH];
	int Index;
				
	assert( line != NULL );
	assert( pBody != NULL );

	if (strchr(line,':')==NULL)
		{
			Printf("ERROR: line not formatted as expected: '%s'\n",line);
			return GE_FALSE;
		}
	if(strncmp(line, "(MAP)", 5) == 0)
	{
		ptext = strstr(line, ": ");
		if(ptext != NULL)
			ptext += 2; // length of ": "
		if( (ptext == NULL) || (strlen(ptext) < 1) )
		{
			// there should be some filename-like text
			Printf("ERROR: Found no texture map: '%s'\n", line);
			return GE_FALSE;
		}

		// prefix the texture path
		strcpy(name, options->TexturePath);
		strcat(name, ptext);
		{
			geVFile *VF;
			geBitmap *Bmp;
			if (stricmp(name+strlen(name)-4,".BMP") != 0)
				{
					Printf("ERROR: Material %d does not reference a BMP.  '%s' \n",i,ptext);
					return GE_FALSE;
				}
			VF = geVFile_OpenNewSystem(NULL,GE_VFILE_TYPE_DOS,name,NULL,GE_VFILE_OPEN_READONLY);
			if (VF == NULL)
				{
					Printf("ERROR: Material %d, unable to locate bitmap '%s'\n",i,name);
					return GE_FALSE;
				}
			Bmp = geBitmap_CreateFromFile(VF);
			geVFile_Close(VF);
			if (Bmp == NULL)
				{
					Printf("ERROR: Material %d, unable to read bitmap '%s'\n",i,name);
					return GE_FALSE;
				}
			if (geBitmap_SetColorKey(Bmp,GE_TRUE,255,GE_TRUE)==GE_FALSE)
				{
					Printf("ERROR: Material %d, unable to set color keying (transparent color) to 255 for bitmap '%s'\n",i,name);
					return GE_FALSE;
				}
			{
				int W,H,TwoPower;
				W = geBitmap_Width ( Bmp );
				H = geBitmap_Height( Bmp );
				if (W != H)
					{
				#pragma message ("change this so that we can 'treat warnings as errors'")
						Printf("WARNING: bitmap for material %d is not square. '%s'\n",i,name);
						//return GE_FALSE;
					}
				#pragma message ("remove this when these limitations are removed from engine")
				for (TwoPower=1; TwoPower<=256; TwoPower*=2)
					{
						if (TwoPower==W)
						break;
					}
				if (TwoPower > 256)
					{
						Printf("Error: bitmap for material must be a power of 2 width and height.\n");
						Printf("       The size of material %d ('%s') is width=%d, height=%d\n",i,name,W,H);
						return GE_FALSE;
					}
			}
          	
	
			strcpy(name,line+6);		// after the "(MAP) "
			*strrchr(name, ':')=0;
			if(geBody_AddMaterial(pBody, name, Bmp, 255.0f, 255.0f, 255.0f, &Index) == GE_FALSE)
				{
					Printf("ERROR: Could not add material %d ('%s') to body. \n",i, name);
					return GE_FALSE;
				}
		}
	}
	else if(strncmp(line, "(RGB)", 5) == 0)
	{
		float r, g, b;
		int j;
		r=g=b=0.0f;
		ptext = strrchr(line, ':');
		if(ptext != NULL)
		{
			ptext++;
			j = sscanf(ptext, "%f %f %f", &r, &g, &b);
		} 
		else 
			j=0;
		if( (ptext == NULL) || (j != 3) )
		{
			// there should be some rgb values
			Printf("ERROR: Found no texture color: '%s'\n", line);
			return GE_FALSE;
		}
		strcpy(name,line+6);      //  after the "(RGB) "
		*strrchr(name, ':')=0;
		if(geBody_AddMaterial(pBody, name, NULL, r, g, b, &Index) == GE_FALSE)
		{
			Printf("ERROR: Could not add material to body: '%s'\n", line);
			return GE_FALSE;
		}
	}
	else
	{
		Printf("ERROR: Could not identify material type: '%s'\n", line);
		return GE_FALSE;
	}
	return GE_TRUE;
}


ReturnCode MkBody_DoMake(MkBody_Options* options,MkUtil_Printf PrintfCallback)
{
	Printf = PrintfCallback;
	fCount =0;

	int nVersion = 0;
	ReturnCode retValue = RETURN_SUCCESS;
	ReturnCode newValue;
	FILE* fp;
	geVFile *VF;
	geBody* pBody = NULL;
	char line[LINE_LENGTH];
	char vlgfile[_MAX_PATH];
	VPHData* pvphdata = NULL;
	int i, j, k,Count;
	int Index;
	char* ptext;
	int NumBones;
	BoneDetail* pBoneIDs;
	int NumNodes;
	NodeDetail* pNodes;
	VertexDetail* pfacevert;
	char name[NAME_LENGTH];
	FaceDetail face;
	int NumMeshes;
	VPHVertex* pVPHVerts;
	geXForm3d euler;
	geQuaternion eulerq;

	// body and NFO filenames must be specified
	if(options->BodyFile[0] == 0)
	{
		Printf("ERROR: Must specify a body file\n");
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		remove(vlgfile);
		return retValue;
	}
	if(options->NFOFile[0] == 0)
	{
		Printf("ERROR: Must specify an NFO file\n");
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		remove(vlgfile);
		return retValue;
	}

	// Process the VPH file if there is one
	if(options->VPHFile[0] != 0)
	{
		strcpy(vlgfile, options->VPHFile);
		strcat(vlgfile, ".VLG");
		i = vphmain(options->VPHFile, vlgfile);
		if(options->WriteTextVPH != MK_FALSE)
		{
			strcpy(line, options->VPHFile);
			ptext = strchr(line, '.');
			if(ptext != NULL)
				ptext[1] = 0;
			strcat(line, "vlg");
			remove(line);
			rename(vlgfile, line);
			// now use the new file
			strcpy(options->VPHFile, line);
		}
		else
		{
			// now use the new file
			strcpy(options->VPHFile, vlgfile);
		}

		if(i != 0)
		{
			Printf("ERROR: Could not read the VPH file\n");
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			remove(vlgfile);
			return retValue;
		}
	}

	// who knows how many times this will get used in here
	geXForm3d_SetEulerAngles(&euler, &options->EulerAngles);
	geQuaternion_FromMatrix(&euler, &eulerq);

	pBody = geBody_Create();

	if(pBody != NULL)
	{
		if(options->VPHFile[0] != 0)
		{
			// Read vph data
			fp = fopen(options->VPHFile, "rt");
			if(fp == NULL)
			{
				Printf("ERROR: Could not open '%s' NFO file\n", options->NFOFile);
				MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
				geBody_Destroy(&pBody);
				remove(vlgfile);
				return retValue;
			}

			pvphdata = GE_RAM_ALLOCATE_STRUCT(VPHData);
			if(pvphdata == NULL)
			{
				Printf("ERROR: Could not allocate VPHData\n");
				MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
				geBody_Destroy(&pBody);
				remove(vlgfile);
				return retValue;
			}
			newValue = ReadVPHData(options, fp, pvphdata);
			fclose(fp);
			remove(vlgfile); // done with it now
			MkUtil_AdjustReturnCode(&retValue, newValue);
			if(retValue == RETURN_ERROR)
			{
				Printf("ERROR: Could not read '%s' VPH data\n", options->VPHFile);
				geBody_Destroy(&pBody);
				geRam_Free(pvphdata);
				return retValue;
			}
		}

		// Read nfo data
		fp = fopen(options->NFOFile, "rt");
		if(fp == NULL)
		{
			Printf("ERROR: Could not open '%s' NFO file\n", options->NFOFile);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			geBody_Destroy(&pBody);
			FreeVPHData(&pvphdata);
			return retValue;
		}

#define SET_ERROR_CLOSE_RETURN					\
	MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);	\
	fclose(fp);									\
	fp = NULL;									\
	geBody_Destroy(&pBody);						\
	FreeVPHData(&pvphdata);						\
	return retValue

#define FGETS_LINE_OR_CLOSE_AND_RETURN(s)										\
	if(fgets(s, LINE_LENGTH, fp) == NULL)										\
	{																			\
		Printf("ERROR: Could not read from '%s' NFO file\n", options->NFOFile);	\
		SET_ERROR_CLOSE_RETURN;													\
	}

		// Read and check version
		FGETS_LINE_OR_CLOSE_AND_RETURN(line);
		if(strncmp(line, "NFO ", 4) != 0)
		{
			Printf("ERROR: '%s' is not an NFO file\n", options->NFOFile);
			SET_ERROR_CLOSE_RETURN;
		}

		// check version numbers
		StripNewLine(line);
		if(strcmp(line + 4, "1.0") == 0)
		{
			nVersion = 0x0100;
		}
		else if(strcmp(line + 4, "2.0") == 0)
		{
			nVersion = 0x0200;
		}
		else
		{
			Printf("ERROR: '%s' NFO file version '%s' is not supported\n", options->NFOFile, line + 4);
			SET_ERROR_CLOSE_RETURN;
		}
		assert(nVersion != 0);

		// Start with the materials
		FGETS_LINE_OR_CLOSE_AND_RETURN(line);
		if(strcmp(line, "Material List\n") != 0)
		{
			Printf("ERROR: No material list in '%s' NFO file\n", options->NFOFile);
			SET_ERROR_CLOSE_RETURN;
		}

		FGETS_LINE_OR_CLOSE_AND_RETURN(line);
		if(sscanf(line, "Number of Materials = %d", &Count) != 1)
		{
			Printf("ERROR: Could not read number of materials in '%s' NFO file\n", options->NFOFile);
			Printf("Line from VPH file:\n%s\n",line);
			SET_ERROR_CLOSE_RETURN;
		}

		// Add materials
		for (i=0; i<Count; i++)
		{
			if (MkUtil_Interrupt())
				{
					Printf("Interrupted\n");
					SET_ERROR_CLOSE_RETURN;
				}
			FGETS_LINE_OR_CLOSE_AND_RETURN(line);
			StripNewLine(line);
			if (MkBody_Material(options,line,pBody,i)==GE_FALSE)
				{
					Printf("ERROR: Unable to add material %d ('%s')\n",i,line);
					SET_ERROR_CLOSE_RETURN;
				}
		}
		

		// EXTRA materials
		{
			int Last = Count + geStrBlock_GetCount(options->ExtraMaterials); 
			int i;
			for ( i=Count; i<Last; i++)	
				{
					const char* MatLine;
					MatLine = geStrBlock_GetString(options->ExtraMaterials,i-Count);
					if (MkBody_Material( options, MatLine, pBody,i)==0)
						{
							Printf("ERROR: Unable to add extra material %d ('%s')\n",i-Count,MatLine);
							SET_ERROR_CLOSE_RETURN;
						}
				}
		}

		// Bone List
		FGETS_LINE_OR_CLOSE_AND_RETURN(line);
		if(strcmp(line, "Bone List\n") != 0)
		{
			Printf("ERROR: No bone list in '%s' NFO file\n", options->NFOFile);
			SET_ERROR_CLOSE_RETURN;
		}

		FGETS_LINE_OR_CLOSE_AND_RETURN(line);
		if(sscanf(line, "Number of Bones = %d", &NumBones) != 1)
		{
			Printf("ERROR: Could not read number of bones in '%s' NFO file\n", options->NFOFile);
			Printf("Line from VPH file:\n%s\n",line);
			SET_ERROR_CLOSE_RETURN;
		}

		pBoneIDs = GE_RAM_ALLOCATE_ARRAY(BoneDetail, NumBones);
		if(pBoneIDs == NULL)
		{
			Printf("ERROR: Could not allocate bone array\n", options->NFOFile);
			SET_ERROR_CLOSE_RETURN;
		}

		// Have a new allocation, so need a new clean up macro

#undef SET_ERROR_CLOSE_RETURN

#define SET_ERROR_CLOSE_RETURN					\
	MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);	\
	fclose(fp);									\
	fp = NULL;									\
	geBody_Destroy(&pBody);						\
	FreeVPHData(&pvphdata);						\
	geRam_Free(pBoneIDs);						\
	return retValue

		for(i=0;i<NumBones;i++)
		{
			if (MkUtil_Interrupt())
				{
					Printf("Interrupted\n");
					break;
				}
			if(fgets(line, LINE_LENGTH, fp) == NULL)
			{
				Printf("ERROR: Could not read from '%s' NFO file\n", options->NFOFile);
				break;
			}
			StripNewLine(line);

			j = strlen("Bone: ");
			if((int)strlen(line) < j)
			{
				// there should be some name-like text
				Printf("ERROR: Found no bone name: '%s'\n", line);
				break;
			}
			strncpy(pBoneIDs[i].Name, line + j, NAME_LENGTH);
			if(options->Capitalize != MK_FALSE)
				strupr(pBoneIDs[i].Name);

			if(fgets(line, LINE_LENGTH, fp) == NULL)
			{
				Printf("ERROR: Could not read from '%s' NFO file\n", options->NFOFile);
				break;
			}
			StripNewLine(line);

			j = strlen("Parent: ");
			if((int)strlen(line) < j)
			{
				// there should be a number
				Printf("ERROR: Found no bone index: '%s'\n", line);
				break;
			}
			if(sscanf(line + j, "%d", &pBoneIDs[i].ParentID) != 1)
			{
				Printf("ERROR: Read no bone index: '%s'\n", line);
				break;
			}
		}
		if(i != NumBones)
		{
			// must have been an error
			SET_ERROR_CLOSE_RETURN;
		}

		// Node List
		FGETS_LINE_OR_CLOSE_AND_RETURN(line);
		if(strcmp(line, "Node Transform Matrix List\n") != 0)
		{
			Printf("ERROR: No node list in '%s' NFO file\n", options->NFOFile);
			SET_ERROR_CLOSE_RETURN;
		}

		FGETS_LINE_OR_CLOSE_AND_RETURN(line);
		if(sscanf(line, "Number of Nodes = %d", &NumNodes) != 1)
		{
			Printf("ERROR: Could not read number of nodes in '%s' NFO file\n", options->NFOFile);
			Printf("Line from VPH file:\n%s\n",line);
			SET_ERROR_CLOSE_RETURN;
		}

		pNodes = GE_RAM_ALLOCATE_ARRAY(NodeDetail, NumNodes);
		if(pNodes == NULL)
		{
			Printf("ERROR: Could not allocate node array\n", options->NFOFile);
			SET_ERROR_CLOSE_RETURN;
		}

		// Have a new allocation, so need a new clean up macro

#undef SET_ERROR_CLOSE_RETURN

#define SET_ERROR_CLOSE_RETURN					\
	MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);	\
	fclose(fp);									\
	fp = NULL;									\
	geBody_Destroy(&pBody);						\
	FreeVPHData(&pvphdata);						\
	geRam_Free(pBoneIDs);						\
	geRam_Free(pNodes);							\
	return retValue

		for(i=0;i<NumNodes;i++)
		{
			if (MkUtil_Interrupt())
				{
					Printf("Interrupted\n");
					break;
				}
			if(fgets(line, LINE_LENGTH, fp) == NULL)
			{
				Printf("ERROR: Could not read from '%s' NFO file\n", options->NFOFile);
				break;
			}
			StripNewLine(line);

			ptext = strstr(line, ": ");
			if(ptext != NULL)
				ptext += 2; // length of ": "
			if( (ptext == NULL) || (strlen(ptext) < 1) )
			{
				// there should be some name-like text
				Printf("ERROR: Found no node name: '%s'\n", line);
				break;
			}
			strncpy(pNodes[i].Name, ptext, NAME_LENGTH);
			if(options->Capitalize != MK_FALSE)
				strupr(pNodes[i].Name);

			// Read TM Matrix
			j = fscanf(fp, "%f, %f, %f,\n",	&pNodes[i].NodeTM.AX,
											&pNodes[i].NodeTM.AY,
											&pNodes[i].NodeTM.AZ);
			if(j != 3)
			{
				Printf("ERROR: Could not read node %d's matrix\n", i);
				break;
			}
			j = fscanf(fp, "%f, %f, %f,\n",	&pNodes[i].NodeTM.BX,
											&pNodes[i].NodeTM.BY,
											&pNodes[i].NodeTM.BZ);
			if(j != 3)
			{
				Printf("ERROR: Could not read node %d's matrix\n", i);
				break;
			}
			j = fscanf(fp, "%f, %f, %f,\n",	&pNodes[i].NodeTM.CX,
											&pNodes[i].NodeTM.CY,
											&pNodes[i].NodeTM.CZ);
			if(j != 3)
			{
				Printf("ERROR: Could not read node %d's matrix\n", i);
				break;
			}
			j = fscanf(fp, "%f, %f, %f,\n",	&pNodes[i].NodeTM.Translation.X,
											&pNodes[i].NodeTM.Translation.Y,
											&pNodes[i].NodeTM.Translation.Z);
			if(j != 3)
			{
				Printf("ERROR: Could not read node %d's matrix\n", i);
				break;
			}

			// geQuaternion
			j = fscanf(fp, "Q(w,x,y,z): %f %f %f %f\n",	&pNodes[i].Q.W,
														&pNodes[i].Q.X,
														&pNodes[i].Q.Y,
														&pNodes[i].Q.Z);
			if(j != 4)
			{
				Printf("ERROR: Could not read node %d's quaternion\n", i);
				break;
			}
			geQuaternion_Normalize(&pNodes[i].Q); // can't believe I have to do this!

			// Scale
			j = fscanf(fp, "S(x,y,z): %f %f %f\n",	&pNodes[i].S.X,
													&pNodes[i].S.Y,
													&pNodes[i].S.Z);
			if(j != 3)
			{
				Printf("ERROR: Could not read node %d's scale\n", i);
				break;
			}

			// Translation
			j = fscanf(fp, "T(x,y,z): %f %f %f\n",	&pNodes[i].T.X,
													&pNodes[i].T.Y,
													&pNodes[i].T.Z);
			if(j != 3)
			{
				Printf("ERROR: Could not read node %d's translation\n", i);
				break;
			}

			// apply rotation to all data at read time
			geXForm3d_Orthonormalize(&pNodes[i].NodeTM);
			geXForm3d_Multiply(&euler, &pNodes[i].NodeTM, &pNodes[i].NodeTM);
			geQuaternion_Multiply(&eulerq, &pNodes[i].Q, &pNodes[i].Q);
			geXForm3d_Transform(&euler, &pNodes[i].S, &pNodes[i].S);
			geXForm3d_Transform(&euler, &pNodes[i].T, &pNodes[i].T);
		}
		if(i != NumNodes)
		{
			// must have been an error
			SET_ERROR_CLOSE_RETURN;
		}

		// Add bones to body
		if(AddBones(options, pBody, NumBones, pBoneIDs, NumNodes, pNodes) == MK_FALSE)
		{
			// must have been an error
			SET_ERROR_CLOSE_RETURN;
		}

		// Fix up the VPH verts
		if(pvphdata != NULL)
		{
			if(CalculateWSVPHVerts(pvphdata, NumNodes, pNodes) == MK_FALSE)
			{
				// must have been an error
				SET_ERROR_CLOSE_RETURN;
			}
		}

		// Look for Bounding Box, if not found, move on
#define BBOX_STRING "Bounding Box: "
		FGETS_LINE_OR_CLOSE_AND_RETURN(line);
		while(strstr(line, BBOX_STRING) != NULL)
		{
			// Bounding Box: name
			// minx, miny, minz
			// maxx, maxy, maxz
			// tm
			// q
			// s
			// t
#define NUM_CUBE_POINTS 8
			geVec3d points[NUM_CUBE_POINTS];
			geVec3d min, max, t;
			geXForm3d matrix, parentmatrix;
			geQuaternion q;

			// read bone
			ptext = line + strlen(BBOX_STRING);
			StripNewLine(ptext);
			if(options->Capitalize != MK_FALSE)
				strupr(ptext);

			for(j=0;j<NumBones;j++)
			{
				if(strcmp(pBoneIDs[j].Name, ptext) == 0)
				{
					break;
				}
			}
			if(j == NumBones)
			{
				// didn't find bone for this box, attach it to the root
				Index = GE_BODY_ROOT;
				geXForm3d_SetIdentity(&parentmatrix);
			}
			else
			{
				if(geBody_GetBoneByName(pBody, ptext, &Index, &matrix, &i) == GE_FALSE)
				{
					Printf("ERROR: bone '%s' not in body\n", ptext);
					SET_ERROR_CLOSE_RETURN;
				}

				parentmatrix = pBoneIDs[j].Matrix;
			}

			// read min
			j = fscanf(fp, "%f %f %f\n",	&min.X,
											&min.Y,
											&min.Z);
			if(j != 3)
			{
				Printf("ERROR: Could not read bounding box's min\n");
				SET_ERROR_CLOSE_RETURN;
			}

			// read max
			j = fscanf(fp, "%f %f %f\n",	&max.X,
											&max.Y,
											&max.Z);
			if(j != 3)
			{
				Printf("ERROR: Could not read bounding box's max\n");
				SET_ERROR_CLOSE_RETURN;
			}

			// populate cube points
			for(j=0;j<NUM_CUBE_POINTS;j++)
			{
				// j's bits determine where to get each component
				// x from bit 2, y bit 1, z bit 0
				if(j & 4)
					points[j].X = min.X;
				else
					points[j].X = max.X;
				if(j & 2)
					points[j].Y = min.Y;
				else
					points[j].Y = max.Y;
				if(j & 1)
					points[j].Z = min.Z;
				else
					points[j].Z = max.Z;
			}

			// read matrix
			// we have been ignoring the max matrix and creating one
			// from the quat and translation

			// WS Matrix
			FGETS_LINE_OR_CLOSE_AND_RETURN(line); // matrix row 1
			FGETS_LINE_OR_CLOSE_AND_RETURN(line); // matrix row 2
			FGETS_LINE_OR_CLOSE_AND_RETURN(line); // matrix row 3
			FGETS_LINE_OR_CLOSE_AND_RETURN(line); // matrix row 4

			// Quaternion
			j = fscanf(fp, "Q(w,x,y,z): %f %f %f %f\n",	&q.W,
														&q.X,
														&q.Y,
														&q.Z);
			if(j != 4)
			{
				Printf("ERROR: Could not read box's quaternion\n");
				SET_ERROR_CLOSE_RETURN;
			}

			// Scale
			FGETS_LINE_OR_CLOSE_AND_RETURN(line);

			// Translation
			j = fscanf(fp, "T(x,y,z): %f %f %f\n",	&t.X,
													&t.Y,
													&t.Z);
			if(j != 3)
			{
				Printf("ERROR: Could not read box's translation\n", i);
				break;
			}

			// compute matrix
			geQuaternion_Normalize(&q);
			geQuaternion_ToMatrix(&q, &matrix);
			matrix.Translation = t;

			// apply rotation to all data at read time
			geXForm3d_Multiply(&euler, &matrix, &matrix);

			// move to bone's coord space
			MaxMath_InverseMultiply(&matrix, &parentmatrix, &matrix);

			// invert matrix to transform points into bone space
			geXForm3d_GetTranspose(&parentmatrix, &parentmatrix);

			// transform the points
			geXForm3d_TransformArray(&parentmatrix, points, points, NUM_CUBE_POINTS);

			// find the min's and max's
			min = points[0];
			max = points[0];
			for(j=1;j<NUM_CUBE_POINTS;j++)
			{
				if(points[j].X < min.X)
					min.X = points[j].X;
				if(points[j].Y < min.Y)
					min.Y = points[j].Y;
				if(points[j].Z < min.Z)
					min.Z = points[j].Z;
				if(points[j].X > max.X)
					max.X = points[j].X;
				if(points[j].Y > max.Y)
					max.Y = points[j].Y;
				if(points[j].Z > max.Z)
					max.Z = points[j].Z;
			}

			// now set the bounding box
			geBody_SetBoundingBox(pBody, Index, &min, &max);

			// read next line for expectant mesh list or another bounding box
			FGETS_LINE_OR_CLOSE_AND_RETURN(line);
		}

		// Mesh List
		if(strcmp(line, "Mesh List\n") != 0)
		{
			Printf("ERROR: No mesh list in '%s' NFO file\n", options->NFOFile);
			SET_ERROR_CLOSE_RETURN;
		}

		FGETS_LINE_OR_CLOSE_AND_RETURN(line);
		if(sscanf(line, "Number of meshes = %d", &NumMeshes) != 1)
		{
			Printf("ERROR: Could not read number of meshes in '%s' NFO file\n", options->NFOFile);
			Printf("Line from VPH file:\n%s\n",line);
			SET_ERROR_CLOSE_RETURN;
		}

		if(nVersion == 0x0200)
		{
			while(NumMeshes > 0)
			{
				ReturnCode rval = V2ReadAndAddMesh(options, fp, pBody, pvphdata, pNodes, NumNodes);
				if (rval == RETURN_ERROR)
				{
					SET_ERROR_CLOSE_RETURN;
				}
				MkUtil_AdjustReturnCode(&retValue, rval);
				NumMeshes--;
			}
			Printf("\t%d Faces Processed\n",fCount);

		}
		else if(nVersion == 0x0100)
		{
			while(NumMeshes > 0)
			{
				if (MkUtil_Interrupt())
					{
						Printf("Interrupted\n");
						break;
					}
		
				// name of mesh
				FGETS_LINE_OR_CLOSE_AND_RETURN(line);
				StripNewLine(line);

				ptext = strstr(line, ": ");
				if(ptext != NULL)
					ptext += 2; // length of ": "
				if( (ptext == NULL) || (strlen(ptext) < 1) )
				{
					// there should be some name-like text
					Printf("ERROR: Found no node name: '%s'\n", line);
					break;
				}
				strncpy(name, ptext, NAME_LENGTH);

				// Face List
				FGETS_LINE_OR_CLOSE_AND_RETURN(line);
				if(strcmp(line, "Face List\n") != 0)
				{
					Printf("ERROR: No face list in '%s' NFO file\n", options->NFOFile);
					SET_ERROR_CLOSE_RETURN;
				}

				FGETS_LINE_OR_CLOSE_AND_RETURN(line);
				if(sscanf(line, "Number of Faces = %d", &k) != 1)
				{
					Printf("ERROR: Could not read number of faces in '%s' NFO file\n", options->NFOFile);
					Printf("Line from VPH file:\n%s\n",line);
					SET_ERROR_CLOSE_RETURN;
				}

				while(k > 0)
				{
					// ignore face number
					FGETS_LINE_OR_CLOSE_AND_RETURN(line);

					if(fscanf(fp, "    %d\n", &face.material) != 1)
					{
						Printf("ERROR: Could not read material for face '%s'\n", line);
						SET_ERROR_CLOSE_RETURN;
					}

					for(i=0;i<3;i++)
					{
						pfacevert = face.verts + i;
						j = fscanf(fp, "    %f %f %f %f %f %f %f %f %d\n",	&pfacevert->v.X,
																			&pfacevert->v.Y,
																			&pfacevert->v.Z,
																			&pfacevert->n.X,
																			&pfacevert->n.Y,
																			&pfacevert->n.Z,
																			&pfacevert->tu,
																			&pfacevert->tv,
																			&pfacevert->bone);
						if(j != 9)
						{
							Printf("ERROR: Could not read coords for face '%s'\n", line);
							SET_ERROR_CLOSE_RETURN;
						}

						// apply rotation to all data at read time
						geXForm3d_Transform(&euler, &pfacevert->v, &pfacevert->v);
						geXForm3d_Transform(&euler, &pfacevert->n, &pfacevert->n);

						// Textures need to be flipped from 3dsmax
						pfacevert->tv = 1 - pfacevert->tv;

						// If this vert is not assigned to a bone, find one.
						if(pfacevert->bone == -1)
						{
							int n;
							geXForm3d dummyx, xmatrix;

							if(geStrBlock_FindString(pvphdata->pSBObjectNames, name, &j) == GE_FALSE)
							{
								Printf("ERROR: Could not find vph data for mesh '%s'\n", name);
								SET_ERROR_CLOSE_RETURN;
							}

							pVPHVerts = pvphdata->ppVerts[j];
							j = pvphdata->NumVerts[j] - 1;
							while(j >= 0)
							{
								if(geVec3d_Compare(&pVPHVerts[j].WSPoint, &pfacevert->v, VERT_EQUALITY_TOLERANCE) != GE_FALSE)
									break; // found it!

								j--;
							}
							if(j < 0)
							{
								// didn't find vert
								Printf("ERROR: Could not find matching vertex\n");
								SET_ERROR_CLOSE_RETURN;
							}

							for(n=0;n<NumNodes;n++)
							{
								if(strcmp(pNodes[n].Name, pvphdata->pLinks[pVPHVerts[j].bone].Name) == 0)
									break; // found it!
							}
							if(n == NumNodes)
							{
								// didn't find vert
								Printf("ERROR: (4) Could not find node '%s' for vertex\n", pvphdata->pLinks[pVPHVerts[j].bone].Name);
								SET_ERROR_CLOSE_RETURN;
							}

							// Need to do a NoScale
							geQuaternion_ToMatrix(&pNodes[n].Q, &xmatrix);
							xmatrix.Translation = pNodes[n].T;

							MaxMath_GetInverse(&xmatrix, &xmatrix);

							// fixup the vert
							MaxMath_Transform(&xmatrix, &pfacevert->v, &pfacevert->v);

							// fixup the normal
							geVec3d_Clear(&xmatrix.Translation);
							MaxMath_Transform(&xmatrix, &pfacevert->v, &pfacevert->v);

							// find the vert's bone
							if(geBody_GetBoneByName(pBody, pNodes[n].Name, &pfacevert->bone, &dummyx, &n) == GE_FALSE)
							{
								Printf("ERROR: Could not find bone '%s' for vertex\n", pNodes[n].Name);
								SET_ERROR_CLOSE_RETURN;
							}
						}
					}
					fCount++;
					if (fCount%25==0)
						Printf("\t%d Faces Processed\n",fCount);

					if(geBody_AddFace(pBody,	&face.verts[0].v, &face.verts[0].n, 
												face.verts[0].tu, face.verts[0].tv, 
												face.verts[0].bone,
											&face.verts[1].v, &face.verts[1].n, 
												face.verts[1].tu, face.verts[1].tv, 
												face.verts[1].bone,
											&face.verts[2].v, &face.verts[2].n, 
												face.verts[2].tu, face.verts[2].tv, 
												face.verts[2].bone,
											face.material) == GE_FALSE)
					{
						Printf("ERROR: Could not add face for '%s'\n", line);
						SET_ERROR_CLOSE_RETURN;
					}

					k--;
				}

				NumMeshes--;
			}
		}

		assert(fp != NULL);
		fclose(fp);
		fp = NULL;

		// Rename any existing body file
		{
			char bakname[_MAX_PATH];

			strcpy(bakname, options->BodyFile);
			strcat(bakname, ".bak");
			remove(bakname);
			rename(options->BodyFile, bakname);
		}

		// Write the body
		VF = geVFile_OpenNewSystem(NULL,GE_VFILE_TYPE_DOS,options->BodyFile,NULL,GE_VFILE_OPEN_CREATE);
		if(VF == NULL)
		{
			Printf("ERROR: Could not open output file '%s'\n", options->BodyFile);
			unlink(options->BodyFile);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		}
		else
		{
			if(geBody_WriteToFile(pBody, VF) == GE_FALSE)
			{
				Printf("ERROR: Body file '%s' was not written correctly\n", options->BodyFile);
				unlink(options->BodyFile);
				MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			}
			else
			{
				if (geVFile_Close(VF) == GE_FALSE)
					{
						Printf("ERROR: Body file '%s' was not written correctly\n", options->BodyFile);
						unlink(options->BodyFile);
						MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
					}
				else
					{
						Printf("SUCCESS: Body file '%s' written successfully\n", options->BodyFile);
					}
			}
		}

		geBody_Destroy(&pBody);
		FreeVPHData(&pvphdata);
		geRam_Free(pBoneIDs);
		geRam_Free(pNodes);
	}
	else
	{
		if (MkUtil_Interrupt())
			{
				Printf("Interrupted... Could not create Body\n");
			}
		else
			{
				Printf("ERROR: Could not create Body\n");
			}
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
	}

	remove(vlgfile);

	return retValue;
}

void MkBody_OutputUsage(MkUtil_Printf PrintfCallback)
{
	Printf = PrintfCallback;
	//COLS: 0         1         2         3         4         5         6         7       | 8
	Printf("\n");
	Printf("Builds a body from WildTangent NFO and Physique data from 3DSMax.\n");
	Printf("\n");
	Printf("MKBODY [options] /B<bodyfile> /N<nfofile> /V<vphfile> [/A] [/C] [/R]\n");
	Printf("       [/T<texturepath>]\n");
	Printf("\n");
	Printf("/B<bodyfile>    Specifies body file.\n");
	Printf("/C              Capitalize all node names.\n");
	Printf("/N<nfofile>     Specifies the WildTangent NFO file.\n");
	Printf("/R              Permit rotational attachments in the body.\n");
	Printf("/T<texturepath> Specifies the path to append to all texture maps.\n");
	Printf("/V<vphfile>     Specifies the Physique vph file.\n");
	Printf("\n");
	Printf("Any existing body file will be renamed to bodyfile.bak\n");
}

MkBody_Options* MkBody_OptionsCreate()
{
	MkBody_Options* pOptions;

	pOptions = GE_RAM_ALLOCATE_STRUCT(MkBody_Options);
	if(pOptions != NULL)
	{
		*pOptions = DefaultOptions;
	}
	pOptions->ExtraMaterials = geStrBlock_Create();
	if (pOptions->ExtraMaterials == NULL)
		{
			geRam_Free(pOptions);
			pOptions = NULL;
		}

	return pOptions;
}

void MkBody_OptionsDestroy(MkBody_Options** ppOptions)
{
	assert(ppOptions != NULL);
	assert(*ppOptions != NULL);

	geStrBlock_Destroy( &((*ppOptions)->ExtraMaterials) );
	geRam_Free(*ppOptions);

	*ppOptions = NULL;
}

ReturnCode MkBody_ParseOptionString(MkBody_Options* options, 
							const char* string, MK_Boolean InScript,
							MkUtil_Printf PrintfCallback)
{

	Printf = PrintfCallback;
	ReturnCode retValue = RETURN_SUCCESS;

	assert(options != NULL);
	assert(string != NULL);

#define NO_FILENAME_WARNING Printf("WARNING: '%s' specified with no filename\n", string)

	if( (string[0] == '-') || (string[0] == '/') )
	{
		switch(string[1])
		{
		case 'b':
		case 'B':
			if(string[2] == 0)
			{
				NO_FILENAME_WARNING;
				retValue = RETURN_WARNING;
			}
			else
			{
				if( (InScript != MK_FALSE) && (options->BodyFile[0] != 0) )
				{
					Printf("WARNING: Body filename in script ignored\n");
					retValue = RETURN_WARNING;
				}
				else
				{
					strcpy(options->BodyFile, string + 2);
				}
			}
			break;

		case 'c':
		case 'C':
			options->Capitalize = MK_TRUE;
			break;

		case 'e':
		case 'E':
			// hidden feature to diddle with vector compare tolerance
			VERT_EQUALITY_TOLERANCE = (float)atof(string + 2);
			break;

		case 'n':
		case 'N':
			if(string[2] == 0)
			{
				NO_FILENAME_WARNING;
				retValue = RETURN_WARNING;
			}
			else
			{
				if( (InScript != MK_FALSE) && (options->NFOFile[0] != 0) )
				{
					Printf("WARNING: NFO filename in script ignored\n");
					retValue = RETURN_WARNING;
				}
				else
				{
					strcpy(options->NFOFile, string + 2);
				}
			}
			break;

		case 'r':
		case 'R':
			options->RotationInBody = MK_TRUE;
			break;

		case 't':
		case 'T':
			if(string[2] == 0)
			{
				NO_FILENAME_WARNING;
				retValue = RETURN_WARNING;
			}
			else
			{
				if( (InScript != MK_FALSE) && (options->TexturePath[0] != 0) )
				{
					Printf("WARNING: texture path in script ignored\n");
					retValue = RETURN_WARNING;
				}
				else
				{
					int len;

					strcpy(options->TexturePath, string + 2);

					// Be sure there is a slash at the end
					len = strlen(options->TexturePath);
					if( (len > 0) && (options->TexturePath[len - 1] != '/') )
						strcat(options->TexturePath, "/");
				}
			}
			break;

		case 'M':
		case 'm':
			if(string[2] == 0)
			{
				NO_FILENAME_WARNING;
				retValue = RETURN_WARNING;
			}
			else
			{
				if (geStrBlock_Append(&(options->ExtraMaterials),string+2)==GE_FALSE)
					{
						Printf("ERROR: Unable to store extra material name '%s'\n",string+2);
						retValue = RETURN_ERROR;
					}
			}
			break;

		case 'V':
			options->WriteTextVPH = MK_TRUE;
			// Fall thru to get VPH filename
		case 'v':
			if(string[2] == 0)
			{
				NO_FILENAME_WARNING;
				retValue = RETURN_WARNING;
			}
			else
			{
				if( (InScript != MK_FALSE) && (options->VPHFile[0] != 0) )
				{
					Printf("WARNING: VPH filename in script ignored\n");
					retValue = RETURN_WARNING;
				}
				else
				{
					strcpy(options->VPHFile, string + 2);
				}
			}
			break;

		default:
			retValue = RETURN_NOACTION;
		}
	}

	return retValue;
}
