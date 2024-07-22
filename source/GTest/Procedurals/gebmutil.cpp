
#define WIN32_LEAN_AND_MEAN
#pragma warning(disable : 4201 4214 4115)
#include <windows.h>
#pragma warning(default : 4201 4214 4115)

#include "gebmutil.h"
#include "RAM.H"
#include "Errorlog.h"
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "vfile.h"

#ifdef _LOG
#include "log.h"
#else
#pragma warning (disable:4100)
static _inline void Log_Printf(const char * str, ...) { }
#pragma warning (default:4100)
#endif

#ifdef _TSC
#include "tsc.h"
#else
#define pushTSC()
#define showPopTSC(s)
#endif

/*}{*************** HBITMAP utilities ******************/

HBITMAP geBitmapUtil_CreateHBITMAP(geBitmap *Bitmap,HWND window,int mip)
{
geBitmap * Lock;
gePixelFormat Format;
geBitmap_Info info;
HBITMAP hbm = NULL;

#pragma todo("gebmu_CreateHBITMAP : choose format to be 8,16,or 24, whichever is closest to Bitmap")

	Format = GE_PIXELFORMAT_24BIT_BGR;

	if ( geBitmap_GetBits(Bitmap) )
	{
		Lock = Bitmap;
	}
	else
	{
		if ( ! geBitmap_LockForRead(Bitmap, &Lock, mip,mip, Format,	GE_FALSE,0) )
			return NULL;
	}

	geBitmap_GetInfo(Lock,&info,NULL);

	if ( info.Format != Format )
		return NULL;

	{
	void * bits;
	HDC hdc;
	BITMAPINFOHEADER bmih;
	int pelbytes;

		pelbytes = gePixelFormat_BytesPerPel(Format);
		bits = geBitmap_GetBits(Lock);

		hdc = GetDC(window);

		bmih.biSize = sizeof(bmih);
		bmih.biHeight = - info.Height;
		bmih.biPlanes = 1;
		bmih.biBitCount = 24;
		bmih.biCompression = BI_RGB;
		bmih.biSizeImage = 0;
		bmih.biXPelsPerMeter = bmih.biYPelsPerMeter = 10000;
		bmih.biClrUsed = bmih.biClrImportant = 0;

		if ( (info.Stride*pelbytes) == (((info.Stride*pelbytes)+3)&(~3)) )
		{
			bmih.biWidth = info.Stride;
			hbm = CreateDIBitmap( hdc, &bmih , CBM_INIT , bits, (BITMAPINFO *)&bmih , DIB_RGB_COLORS );
		}
		else
		{
		void * newbits;
		int Stride;
			bmih.biWidth = info.Width;
			Stride = (((info.Width*pelbytes)+3)&(~3));
			newbits = geRam_Allocate(Stride * info.Height);
			if ( newbits )
			{
			char *newptr,*oldptr;
			int y;
				newptr = static_cast<char*>(newbits);
				oldptr = static_cast<char*>(bits);
				for(y=0;y<info.Height;y++)
				{
					memcpy(newptr,oldptr,(info.Width)*pelbytes);
					oldptr += info.Stride*pelbytes;
					newptr += Stride;
				}
				hbm = CreateDIBitmap( hdc, &bmih , CBM_INIT , newbits, (BITMAPINFO *)&bmih , DIB_RGB_COLORS );
				geRam_Free(newbits);
			}
		}

		ReleaseDC(window,hdc);
	}

	if ( Lock != Bitmap )
		geBitmap_UnLock(Lock);

return hbm;
}

geBitmap * geBitmapUtil_CreateFromHBITMAP(HWND window,HBITMAP hbm)
{
geBitmap * Bmp;
char bmidata[sizeof(BITMAPINFOHEADER)+12];
BITMAPINFOHEADER * pbmi;
HDC hdc;
int lines;

	if ( ! window || ! hbm )
		return NULL;

	pbmi = (BITMAPINFOHEADER *)bmidata;

	pbmi->biSize = sizeof(BITMAPINFOHEADER);
	pbmi->biWidth = 0;
	pbmi->biHeight = 0;
	pbmi->biPlanes = 1;
	pbmi->biBitCount = 0;
	pbmi->biCompression = BI_RGB;
	pbmi->biSizeImage = 0;
	pbmi->biXPelsPerMeter = pbmi->biYPelsPerMeter = 1000;
	pbmi->biClrUsed = pbmi->biClrImportant = 0;

	hdc = GetDC(window);
	if ( ! hdc )
		return NULL;

	lines = GetDIBits(hdc,hbm,0,0,NULL,(BITMAPINFO *)pbmi,0);

	ReleaseDC(window,hdc);

	if ( ! lines )
		return NULL;

	{
	geBitmap * Lock;
	void * bits;
	int ret;
	uint32 * masks;

		Bmp = geBitmap_Create(pbmi->biWidth,abs(pbmi->biHeight),1,GE_PIXELFORMAT_32BIT_XRGB);
		if ( ! Bmp )
			return NULL;

		masks = (uint32 *)(bmidata + sizeof(BITMAPINFOHEADER));
		masks[0] = 0x00FF0000;
		masks[1] = 0x0000FF00;
		masks[2] = 0x000000FF;

		if ( ! geBitmap_LockForWrite(Bmp,&Lock,0,0) )
		{
			geBitmap_Destroy(&Bmp);
			return NULL;
		}

		bits = geBitmap_GetBits(Lock);
		assert(bits);

		pbmi->biSize = sizeof(BITMAPINFOHEADER);
		pbmi->biHeight = - abs(pbmi->biHeight);
		pbmi->biPlanes = 1;
		pbmi->biBitCount = 32;
		pbmi->biCompression = BI_RGB;
		pbmi->biSizeImage = 0;
		pbmi->biClrUsed = pbmi->biClrImportant = 0;

		hdc = GetDC(window);
		assert(hdc);

		ret = GetDIBits(hdc,hbm,0,lines,bits,(BITMAPINFO *)pbmi,0);

		ReleaseDC(window,hdc);

		geBitmap_UnLock(Lock);

		if ( ! ret )
		{
			geBitmap_Destroy(&Bmp);
			return NULL;
		}
	}

return Bmp;
}

/*}{*************** SmoothBits !! ******************/

/*******

SmoothBits with radius == 1 is actually the solution of the diffusion equation (!!)

*pSrc = (pSrc[-1] + pSrc[1] + pSrc[s]+ pSrc[s-1])>>2;

rewriting :

f'(x,y) = ( f(x+1,y) + f(x-1,y) + f(x,y+1) + f(x,y-1) )/4

f'(x,y) - f(x,y)  = ( f(x+1,y) + f(x-1,y) + f(x,y+1) + f(x,y-1) )/4 - f(x,y)

[f'(x,y) - f(x,y)]/a  = (b^2/a) [ ( f(x+1,y) + f(x-1,y) + f(x,y+1) + f(x,y-1) )/4 - f(x,y) ]/b^2

now the left is a time-derivative with time step a and the right side is a grad-squared with
x-spacing of b.  Since b = 1 pixel :

dt f(x,y)  = (1 pixel^2/a) grad^2 f(x,y)

in our application, 'a' is like 1/50 second = 0.02 or such, so we have a diffusion equation:

	dt f() = lambda  grad^2 f()

with a fixed diffusion coefficient lambda = 50 pixel^2/second

********/
geBoolean geBitmapUtil_SmoothBits(geBitmap_Info *pInfo,void *FmBits,void *ToBits,int radius,geBoolean wrap)
{
int bpp,x,y,w,h,s;

	assert(FmBits && ToBits);
	assert(radius > 0);

	if ( radius > 3 ) radius = 3;

	bpp = gePixelFormat_BytesPerPel(pInfo->Format);

	w = pInfo->Width;
	h = pInfo->Height;
	s = pInfo->Stride;

	switch(bpp)
	{
		case 0:
			return GE_FALSE;
		case 1:
		{
		uint8 *pSrc,*pDst;

			pSrc = static_cast<uint8*>(FmBits);
			pDst = static_cast<uint8*>(ToBits);

			if ( wrap )
			{
			uint8 *pSrcN,*pSrcP;
				switch(radius)
				{
				case 1:
					for(y=0;y<h;y++)
					{						
						pSrcN = pSrc+s;
						pSrcP = pSrc-s;

						if ( y == 0 )
						{
							pSrcP = pSrc + s*(h-1);
						}
						else if ( y == (h-1) )
						{
							pSrcN = static_cast<uint8*>(FmBits);
						}

						//first pel						
						*pDst++ = (pSrc[w-1] + pSrc[1] + *pSrcN + *pSrcP)>>2;
						pSrc++;
						pSrcN++;
						pSrcP++;

						for(x = w-2;x--;)
						{
							*pDst++ = (pSrc[-1] + pSrc[1] + *pSrcN + *pSrcP)>>2;
							pSrc++;
							pSrcN++;
							pSrcP++;
						}

						// last pel
						*pDst++ = (pSrc[-1] + pSrc[1-w] + *pSrcN + *pSrcP)>>2;
						pSrc++;
						pSrcN++;
						pSrcP++;

						pDst += (s-w);
						pSrc += (s-w);
					}
					break;

				default:
				case 2:
					for(y=0;y<h;y++)
					{						
						pSrcN = pSrc+s;
						pSrcP = pSrc-s;

						if ( y == 0 )
						{
							pSrcP = pSrc + s*(h-1);
						}
						else if ( y == (h-1) )
						{
							pSrcN = static_cast<uint8*>(FmBits);
						}

						//first pel						
						*pDst++ = (pSrcN[-1] + pSrc[1] + *pSrcN + *pSrcP)>>2;
						pSrc++;
						pSrcN++;
						pSrcP++;

						for(x = w-2;x--;)
						{
							*pDst++ = (pSrc[-1] + pSrc[1] + pSrcN[0] + pSrcP[0] +
										pSrcN[1] + pSrcN[-1] + pSrcP[1] + pSrcP[-1])>>3;
							pSrc++;
							pSrcN++;
							pSrcP++;
						}

						// last pel
						*pDst++ = (pSrc[-1] + pSrcP[1] + *pSrcN + *pSrcP)>>2;
						pSrc++;
						pSrcN++;
						pSrcP++;

						pDst += (s-w);
						pSrc += (s-w);
					}
					break;
				}
			}
			else
			{
				switch(radius)
				{
				case 1:
					
						// first line
						*pDst++ = (pSrc[1] + pSrc[s])>>1;
						pSrc++;
						for(x=w-1;x--;)
						{
							*pDst++ = (pSrc[-1] + pSrc[1] + pSrc[s]+ pSrc[s-1])>>2;
							pSrc++;
						}
						pDst += (s-w);
						pSrc += (s-w);

					for(y=h-2;y--;)
					{
						// middle lines
						
						x = w;

						#ifdef DONT_USE_ASM //{

						while(x--)
						{
							*pDst++ = (pSrc[-1] + pSrc[1] + pSrc[s] + pSrc[-s])>>2;
							pSrc++;
						}

						pDst += (s-w);
						pSrc += (s-w);

						#else //}{

						__asm 
						{
							mov esi, pSrc
							mov edi, pDst
						
							// something like 10 clocks per byte = molto bene
							
						mainloop:
							xor eax,eax						// load the sum into eax

							movzx edx,BYTE PTR [esi+1]		// port 2, latency 1
							add eax,edx						// port 1, latency 1

							movzx edx,BYTE PTR [esi-1]
							add eax,edx	
								
							mov ecx,esi						// ecx = esi + s
							add ecx,s
							movzx edx,BYTE PTR [ecx]		// eax += (BYTE) [ecx]
							add eax,edx						// we have to use edx cuz of the byte 

							mov ecx,esi	
							sub ecx,s
							movzx edx,BYTE PTR [ecx]
							add eax,edx	

							shr eax,2

							mov [edi],al

							inc edi
							inc esi
							dec x

							jnz mainloop
						}

						pDst += s;
						pSrc += s;

						#endif // }
					}
					
						// last line
						for(x=w-1;x--;)
						{
							*pDst++ = (pSrc[-1] + pSrc[1] + pSrc[-s]+ pSrc[-s+1])>>2;
							pSrc++;
						}
						*pDst++ = (pSrc[-1] + pSrc[-s])>>1;
						pSrc++;
						pSrc += (s-w);
						pDst += (s-w);
					break;

				default:
				case 2:			
						// first line
						*pDst++ = (pSrc[1] + pSrc[s])>>1;
						pSrc++;
						for(x=w-1;x--;)
						{
							*pDst++ = (pSrc[-1] + pSrc[1] + pSrc[s]+ pSrc[s-1])>>2;
							pSrc++;
						}
						pSrc += (s-w);
						pDst += (s-w);

					for(y=h-2;y--;)
					{
						*pDst++ = (pSrc[-1] + pSrc[1] + pSrc[s] + pSrc[-s])>>2;
						pSrc++;

						#ifdef DONT_USE_ASM //{

						for(x=w-2;x--;)
						{
							*pDst++ = (pSrc[-1] + pSrc[1] + pSrc[s] + pSrc[-s] +
										pSrc[s+1] + pSrc[s-1] + pSrc[-s+1] + pSrc[-s-1])>>3;
							pSrc++;
						}

						#else //}{

						x = w-2;

						__asm 
						{
							mov esi, pSrc
							//lea esi, [pSrc]
							mov edi, pDst
							//lea edi, [pDst]
							
						mainloop2:
							xor eax,eax						// load the sum into eax

							movzx edx,BYTE PTR [esi+1]		// eax += (BYTE) esi[1]
							add eax,edx						// we have to use edx cuz of the byte

							movzx edx,BYTE PTR [esi-1]
							add eax,edx	

							mov ecx,esi						// ecx = esi + s
							add ecx,s
							movzx edx,BYTE PTR [ecx-1]		
							add eax,edx						
							movzx edx,BYTE PTR [ecx]		
							add eax,edx
							movzx edx,BYTE PTR [ecx+1]
							add eax,edx						

							mov ecx,esi	
							sub ecx,s
							movzx edx,BYTE PTR [ecx-1]
							add eax,edx	
							movzx edx,BYTE PTR [ecx]
							add eax,edx	
							movzx edx,BYTE PTR [ecx+1]
							add eax,edx	

							shr eax,3

							mov [edi],al

							inc edi
							inc esi
							dec x

							jnz mainloop2
						}

						pDst += w-2;
						pSrc += w-2;

						#endif //}

						*pDst++ = (pSrc[-1] + pSrc[1] + pSrc[s] + pSrc[-s])>>2;
						pSrc++;
						pSrc += (s-w);
						pDst += (s-w);
					}
						// last line
						for(x=w-1;x--;)
						{
							*pDst++ = (pSrc[-1] + pSrc[1] + pSrc[-s]+ pSrc[-s+1])>>2;
							pSrc++;
						}
						*pDst++ = (pSrc[-1] + pSrc[-s])>>1;
						pSrc++;
						pSrc += (s-w);
						pDst += (s-w);
					break;
				}
			}

			assert( pSrc == ((uint8 *)FmBits + h*s) );
			assert( pDst == ((uint8 *)ToBits + h*s) );
			break;
		}


		case 2:
		case 3:
		case 4:
			// can't just use a simple blender for 2-3-4-byte pixel formats;
			// must decompose pixels to RGBA and blend
			geErrorLog_AddString(-1,"geBitmapUtil_SmoothBits : only implemented for 1-byte data",NULL);
			return GE_FALSE;
	}

	return GE_TRUE;
}

geBoolean geBitmapUtil_SetAlphaFromBrightness(geBitmap *Bmp)
{
geBitmap_Palette * Pal;
	assert(Bmp);

	if ( Pal = geBitmap_GetPalette(Bmp) )
	{
	uint32 PalData[256];
	int p;

		if ( ! geBitmap_Palette_SetFormat(Pal,GE_PIXELFORMAT_32BIT_ARGB) )
			return GE_FALSE;

		if ( ! geBitmap_Palette_GetData(Pal,PalData,GE_PIXELFORMAT_32BIT_ARGB,256) )
			return GE_FALSE;

		for(p=0;p<256;p++)
		{
		uint32 r,g,b,a,pel;
			pel = PalData[p];
			r = (pel>>16)&0xFF;
			g = (pel>>8)&0xFF;
			b = (pel>>0)&0xFF;
			a = (r + g + b + 2)/3;
			pel = (pel & 0x00FFFFFF) + (a<<24);
			PalData[p] = pel;
		}

		if ( ! geBitmap_Palette_SetData(Pal,PalData,GE_PIXELFORMAT_32BIT_ARGB,256) )
			return GE_FALSE;

		return GE_TRUE;
	}
	else
	{
	geBitmap * Alpha;
	geBitmap_Info Info;
		if ( ! geBitmap_GetInfo(Bmp,&Info,NULL) )
			return GE_FALSE;
		Info.Format = GE_PIXELFORMAT_8BIT_GRAY;
		Alpha = geBitmap_CreateFromInfo(&Info);
		if ( ! geBitmap_BlitBitmap(Bmp,Alpha) )
			return GE_FALSE;
		if ( ! geBitmap_SetAlpha(Bmp,Alpha) )
			return GE_FALSE;
		return GE_TRUE;
	}
}

geBoolean geBitmapUtil_SetColor(geBitmap *Bmp,int R,int G,int B,int A)
{
geBitmap *Lock=NULL;
geBitmap_Info Info;
void *Bits;
int w,h,s,bpp,x;

	assert(Bmp);

	if ( ! geBitmap_LockForWrite(Bmp,&Lock,0,0) )
		goto fail;

	if ( ! geBitmap_GetInfo(Lock,&Info,NULL) )
		goto fail;

	Bits = geBitmap_GetBits(Lock);

	if ( ! Bits )
		goto fail; 

	bpp = gePixelFormat_BytesPerPel(Info.Format);

	w = Info.Width;
	h = Info.Height;
	s = Info.Stride;

	switch(bpp)
	{
		default:		
			goto fail;
		case 1:
		{
		uint8 * pBits,Pixel;
			pBits = static_cast<uint8*>(Bits);
			Pixel = (R+G+B)/3;
			while(h--)
			{
				for(x=w;x--;)
				{
					*pBits++ = Pixel;
				}	
				pBits += (s-w);				
			}
			break;
		}		
		case 2:
		{
		uint16 * pBits,Pixel;
			pBits = static_cast<uint16*>(Bits);
			Pixel = (uint16)gePixelFormat_ComposePixel(Info.Format,R,G,B,A);
			while(h--)
			{
				for(x=w;x--;)
				{
					*pBits++ = Pixel;
				}	
				pBits += (s-w);				
			}
			break;
		}		
		case 4:
		{
		uint32 * pBits,Pixel;
			pBits = static_cast<uint32*>(Bits);
			Pixel = (uint32)gePixelFormat_ComposePixel(Info.Format,R,G,B,A);
			while(h--)
			{
				for(x=w;x--;)
				{
					*pBits++ = Pixel;
				}	
				pBits += (s-w);				
			}
			break;
		}		
		case 3:
		{
		uint8 * pBits,b1,b2,b3;
		uint32 Pixel;
			pBits = static_cast<uint8*>(Bits);
			Pixel = (uint32)gePixelFormat_ComposePixel(Info.Format,R,G,B,A);
			b1 = (uint8)((Pixel>>16)&0xFF);
			b2 = (uint8)((Pixel>>8 )&0xFF);
			b3 = (uint8)((Pixel>>0 )&0xFF);
			while(h--)
			{
				for(x=w;x--;)
				{
					*pBits++ = b1;
					*pBits++ = b2;
					*pBits++ = b3;
				}	
				pBits += 3*(s-w);				
			}
			break;
		}
	}

	if ( ! geBitmap_UnLock(Lock) )
		goto fail;

	geBitmap_RefreshMips(Bmp);

	return GE_TRUE;
fail:

	if ( Lock )
		geBitmap_UnLock(Lock);

	return GE_FALSE;
}

/*}{*************** Bitmap_Create : ******************/

geBitmap * geBitmapUtil_CreateFromFileName(const geVFile * BaseFS,const char * BmName)
{
geVFile * File;
geBitmap * Bmp;

	if ( BaseFS )
		File = geVFile_Open((geVFile *)BaseFS,BmName,GE_VFILE_OPEN_READONLY);
	else
		File = geVFile_OpenNewSystem(NULL,GE_VFILE_TYPE_DOS,BmName,NULL,GE_VFILE_OPEN_READONLY);
	if ( ! File )
		return NULL;

	Bmp = geBitmap_CreateFromFile(File);
	geVFile_Close(File);

return Bmp;
}

geBitmap * geBitmapUtil_CreateFromFileAndAlphaNames(const geVFile * BaseFS,const char * BmName,const char *AlphaName)
{
geBitmap *Bmp,*AlphaBmp;

	Bmp = geBitmapUtil_CreateFromFileName(BaseFS,BmName);
	if ( ! Bmp )
		return NULL;

	AlphaBmp = geBitmapUtil_CreateFromFileName(BaseFS,AlphaName);
	if ( ! AlphaBmp )
	{
		geBitmap_Destroy(&Bmp);
		return NULL;
	}

	if ( ! geBitmap_SetAlpha(Bmp,AlphaBmp) )
	{
		geBitmap_Destroy(&Bmp);
		geBitmap_Destroy(&AlphaBmp);
		return NULL;
	}

	geBitmap_Destroy(&AlphaBmp);

	geBitmap_SetPreferredFormat(Bmp,GE_PIXELFORMAT_16BIT_4444_ARGB);

return Bmp;
}

/*}{****** MSE bitmap compare stuff *********/

static void RGBi_to_YUVi(int R,int G,int B,int *Y,int *U,int *V);
static void YUVi_to_RGBi(int y,int u,int v,int *R,int *G,int *B);

double geBitmapUtil_MSE2PSNR(double mse)
{
return ( 48.165 - 10.0*log10(mse));
}

double geBitmapUtil_PSNR2MSE(double psnr)
{
return pow(10.0,4.8165 - psnr*0.1);
}

geBoolean geBitmapUtil_CompareBitmaps(geBitmap *bm1,geBitmap *bm2,char * IntoStr,double * pMSE)
{
geBitmap *lock1,*lock2;
uint8 *ptr1,*ptr2;
geBitmap_Info wi,oi;
double diff,mse;
int x,y,w,h,cnt;
geBoolean success;

	assert(bm1 && bm2);

	if ( ! geBitmap_LockForRead(bm1,&lock1,0,0,GE_PIXELFORMAT_32BIT_BGRA,0,0) )
		return GE_FALSE;
	if ( ! geBitmap_LockForRead(bm2,&lock2,0,0,GE_PIXELFORMAT_32BIT_BGRA,0,0) )
		return GE_FALSE;

	ptr1 = static_cast<uint8*>(geBitmap_GetBits(lock1));
	ptr2 = static_cast<uint8*>(geBitmap_GetBits(lock2));
	assert( ptr1 && ptr2 );

	success = geBitmap_GetInfo(lock1,&wi,NULL); assert(success);
	success = geBitmap_GetInfo(lock2,&oi,NULL); assert(success);

	h = min(oi.Height,wi.Height);
	w = min(oi.Width,wi.Width);

	diff = cnt = 0;
	for(y=h; y--; )
	{
		for(x=w; x--;)
		{
		int dR,dG,dB,dA,R,G,B,A;
			dA = *ptr1++; dR = *ptr1++; dG = *ptr1++; dB = *ptr1++;
			A  = *ptr2++; R  = *ptr2++; G  = *ptr2++; B  = *ptr2++;

			RGBi_to_YUVi(dR,dG,dB,&dR,&dG,&dB);
			RGBi_to_YUVi( R, G, B, &R, &G, &B);

			dA -= A;
			dR -= R;
			dG -= G;
			dB -= B;

			if ( dA )
			{
				diff += dA*dA;
			}

			if ( A > 40 )
			{
				diff +=(dR*dR + dG*dG + dB*dB);
				cnt ++;
			}
		}
		ptr1 += (wi.Stride - w)*4;
		ptr2 += (oi.Stride - w)*4;
	}

	geBitmap_UnLock(lock1);
	geBitmap_UnLock(lock2);

	mse = diff / (double)cnt;
	if ( pMSE )
		*pMSE = mse;
	if ( IntoStr )
		sprintf(IntoStr,"mse = %f, rmse = %f, psnr = %f",mse,sqrt(mse),geBitmapUtil_MSE2PSNR(mse));

return GE_TRUE;
}

/*}{*************** the YUV routines : ******************/

#define YUV_SHIFT 	14
#define YUV_HALF	(1<<(YUV_SHIFT-1))
#define YUV_ONE		(1<<YUV_SHIFT)
#define Y_R   ((int)( 0.29900 * YUV_ONE ))
#define Y_G   ((int)( 0.58700 * YUV_ONE ))
#define Y_B   ((int)( 0.11400 * YUV_ONE ))
#define U_R   ((int)(-0.16874 * YUV_ONE ))
#define U_G   ((int)(-0.33126 * YUV_ONE ))
#define U_B   ((int)( 0.50000 * YUV_ONE ))
#define V_R   ((int)(-0.50000 * YUV_ONE ))	// ** important sign change of 'V' from jpeg default
#define V_G   ((int)( 0.41869 * YUV_ONE ))
#define V_B   ((int)( 0.08131 * YUV_ONE ))
#define R_Y   (    				YUV_ONE )       
#define R_U   (0)
#define R_V   ((int)(-1.40200 * YUV_ONE ))
#define G_Y   (    				YUV_ONE )       
#define G_U   ((int)(-0.34414 * YUV_ONE ))
#define G_V   ((int)( 0.71414 * YUV_ONE ))
#define B_Y   (     			YUV_ONE )       
#define B_U   ((int)( 1.77200 * YUV_ONE ))
#define B_V   (0)       

#define Y_RGB(R,G,B) (( Y_R * (R) + Y_G * (G) + Y_B * (B) + YUV_HALF ) >> YUV_SHIFT)
#define U_RGB(R,G,B) (( U_R * (R) + U_G * (G) + U_B * (B) + YUV_HALF ) >> YUV_SHIFT)
#define V_RGB(R,G,B) (( V_R * (R) + V_G * (G) + V_B * (B) + YUV_HALF ) >> YUV_SHIFT)
#define R_YUV(Y,U,V) (( R_Y * (Y) + R_U * (U) + R_V * (V) + YUV_HALF ) >> YUV_SHIFT)
#define G_YUV(Y,U,V) (( G_Y * (Y) + G_U * (U) + G_V * (V) + YUV_HALF ) >> YUV_SHIFT)
#define B_YUV(Y,U,V) (( B_Y * (Y) + B_U * (U) + B_V * (V) + YUV_HALF ) >> YUV_SHIFT)

#define minmax(x,lo,hi) ( (x)<(lo)?(lo):( (x)>(hi)?(hi):(x)) )

static void RGBi_to_YUVi(int R,int G,int B,int *Y,int *U,int *V)
{
	*Y = Y_RGB(R,G,B);
	*U = U_RGB(R,G,B) + 127;
	*V = V_RGB(R,G,B) + 127;
}

static void YUVi_to_RGBi(int y,int u,int v,int *R,int *G,int *B)
{
int r,g,b;

	u -= 127;
	v -= 127;
	r = R_YUV(y,u,v);
	g = G_YUV(y,u,v);
	b = B_YUV(y,u,v);

	*R = minmax(r,0,255);	// we could get negative ones and whatnot
	*G = minmax(g,0,255);	//	because the y,u,v are not really 24 bits;
	*B = minmax(b,0,255);	//	there are regions of YUV space that will never be reached by RGBb_to_YUVb
}


