#ifndef GEBMUTIL_H
#define GEBMUTIL_H

#include "genesis.h"

#ifndef _INC_WINDOWS
#ifdef STRICT
typedef struct HWND__ * HWND;
typedef struct HBITMAP__ * HBITMAP;
#else // STRICT
typedef void * HWND;
typedef void * HBITMAP;
#endif // STRICT
#endif // _INC_WINDOWS

#ifdef __cplusplus
extern "C" {
#endif

/** helper functions for geBitmap *****/

/*** Windows HBITMAP conversions : ****/

HBITMAP geBitmapUtil_CreateHBITMAP(geBitmap *Bitmap,HWND window,int mip);
	// use DeleteObject when done

geBitmap * geBitmapUtil_CreateFromHBITMAP(HWND window,HBITMAP hbm);

/*** Drawing primitives *****/

geBoolean geBitmapUtil_SetColor(geBitmap *Bmp,int R,int G,int B,int A);
	// sets all of the bitmap to one color
	// on palettized, use R=G=B=A= 0 to set all palette indeces to zero

geBoolean geBitmapUtil_SmoothBits(geBitmap_Info *pInfo,void *FmBits,void *ToBits,int radius, geBoolean wrap);
	// a convolution filter for linear smoothing
	// assembly & fast
	// Fm == To is ok
	// (btw wrap is slow & not in assembly yet)

geBoolean geBitmapUtil_SetAlphaFromBrightness(geBitmap *Bmp);

/*** Misc Utils ***/

geBitmap * geBitmapUtil_CreateFromFileName(const geVFile * BaseFS,const char * BmName);
geBitmap * geBitmapUtil_CreateFromFileAndAlphaNames(const geVFile * BaseFS,const char * BmName,
																	const char *AlphaName);

geBoolean geBitmapUtil_SetPaletteFromString(geBitmap * Bitmap,char ** pParams);
				// moves pParams past the used palette entries

geBoolean geBitmapUtil_CompareBitmaps(geBitmap *bm1,geBitmap *bm2,char * IntoStr,double * pMSE);
				// compares YUV's

double geBitmapUtil_MSE2PSNR(double mse);
double geBitmapUtil_PSNR2MSE(double psnr);

/******

todo :

	WriteToBMP

********/

#ifdef __cplusplus
}
#endif


#endif


