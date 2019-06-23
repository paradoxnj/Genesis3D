/****************************************************************************************/
/*  VPH.C																				*/
/*                                                                                      */
/*  Author: Stephen Balkum	                                                            */
/*  Description: 3DS MAX Physique file parsing.											*/
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
// VPH READER
//   reads a vph file and displays the parsed contents in a log file.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ram.h"

//---------------------------------------------------------------------------
// Logging code absorbed from old module

typedef struct log_type
{
	char filename[_MAX_PATH];
} log_type;

void log_close(log_type **log)
{
	assert(log);
	assert(*log);
	geRam_Free(*log);
	*log=NULL;
}
 
static void log_file_reset(log_type *log)
{
	FILE *f;

	f = fopen(log->filename,"w");
	assert(f);
	fclose(f);
}
 
log_type *log_open(const char *filename)
{
	log_type *log;
	log = geRam_Allocate(sizeof(log_type));
	assert(log);
	strcpy(log->filename, filename);
	
	log_file_reset(log);

	return log;
}
 
static void log_file_write(log_type *log,const char *buf)
{
	FILE *f;

	f = fopen(log->filename,"a");
	assert(f);
	fseek(f,0L, SEEK_END);
	fwrite(buf, strlen(buf),1,f);
	fflush(f);
	fclose(f);
}

void log_output(log_type *log, char *format, ... )
{
	char buf[ 1024 ];
	assert(log);
	assert(format);
 
	// I use vsprintf() from the C RTL instead of wvsprintf() from the
	// WIN API so that I can use floating point.
 
	vsprintf( buf, format, (char *)&format + sizeof( format ) );
	strcat(buf,"\n");
	log_file_write(log,buf);
}

//---------------------------------------------------------------------------
 
log_type *vphlog;

#define VPH_FILE  532 

int vph_read_short(FILE *f,short *s)
{
	int cnt,size;
	assert(f);
	assert(s);
	size = sizeof(*s);
	assert( size == sizeof(short));
	cnt = fread(s,1,size,f);
	if (cnt != size)
		return 1;
	return 0;
}

int vph_read_long(FILE *f,long *l)
{
	int cnt,size;
	assert(f);
	assert(l);
	size = sizeof(*l);
	assert( size == sizeof(long));
	cnt = fread(l,1,size,f);
	if (cnt != size)
		return 1;
	return 0;
}

int vph_read_int(FILE *f,int *i)
{
	int cnt,size;
	assert(f);
	assert(i);
	size = sizeof(*i);
	assert( size == sizeof(int));
	cnt = fread(i,1,size,f);
	if (cnt != size)
		return 1;
	return 0;
}


int vph_read_float(FILE *f,float *flt)
{
	int cnt,size;
	assert(f);
	assert(flt);
	size = sizeof(*flt);
	assert( size == sizeof(float));
	cnt = fread(flt,1,size,f);
	if (cnt != size)
		return 1;
	return 0;
}

int vph_read_string(FILE *f,char *s)
{
	int cnt,i,err;
	char shortstr[2]=" ";
	int length;
	
	assert(f);
	assert(s);
	
	err = vph_read_int(f,&length);			
	if (err != 0)
		{
			log_output(vphlog,"Failure to read string length");
			return 1;
		}
	if (length==0)
		log_output(vphlog,"[string has zero length]");
	s[0] = 0;
	for (i=0; i<length; i++)
		{
			cnt = fread(shortstr,1,1,f);
			if (cnt != 1)
				{
					log_output(vphlog,"Failure to read string body (%s)", s);
					return 1;
				}
			strcat(s, shortstr);
		}
	return 0;
}

#define ROWLENGTH 3

void vph_report_int_array(int *array, int length)
{
	int i;

	assert(array);
	assert(length>0);
	for (i=0; i<=length-ROWLENGTH; i+=ROWLENGTH)
		{
			log_output(vphlog," %d %d %d",array[i],array[i+1],array[i+2]);
		}
	i=length%ROWLENGTH;
	switch (i)
	{
		case (0): break;
		case (1): log_output(vphlog," %d",array[i]); break;
		case (2): log_output(vphlog," %d %d",array[i],array[i+1]); break;
		default : assert(0);
	}
}

void vph_report_float_array(float *array, int length)
{
	int i;

	assert(array);
	assert(length>0);
	for (i=0; i<=length-ROWLENGTH; i+=ROWLENGTH)
		{
			log_output(vphlog," %f %f %f",array[i],array[i+1],array[i+2]);
		}
	i=length%ROWLENGTH;
	switch (i)
	{
		case (0): break;
		case (1): log_output(vphlog," %f",array[i]); break;
		case (2): log_output(vphlog," %f %f",array[i],array[i+1]); break;
		default : assert(0);
	}
}

	

int vph_read_int_array(FILE *f,int *array, int length)
{
	int i,err;

	assert(f);
	assert(array);
	assert(length>0);
	for (i=0; i<length; i++)
		{
			err = vph_read_int(f,&(array[i]));
			if (err != 0)
				{
					return i+1;
				}
		}
	return 0;
}


int vph_read_float_array(FILE *f,float *array, int length)
{
	int i,err;
	
	assert(f);
	assert(array);
	assert(length>0);
	for (i=0; i<length; i++)
		{
			err = vph_read_float(f,&(array[i]));
			if (err != 0)
				{
					return i+1;
				}
			
		}
	return 0;
}


// parses vph file.  Just scans format blocks, and reports to vphlog
int vph_parse(FILE *f)
{
	int i,err;
	short header;
	long  links;
	long li;
	int num_objects;

	assert(f);

	err = vph_read_short(f,&header);
	if (err != 0)
		{
			log_output(vphlog,"Failure to read Header");
			return 1;
		}
	log_output(vphlog,"Header = %d  (should be=%d)",(int)header,(int)VPH_FILE);
	
	err = vph_read_long(f,&links);
	if (err != 0)
		{
			log_output(vphlog,"Failure to read number of links");
			return 1;
		}
	log_output(vphlog,"Number of links = %ld",(long)links);
	if (links<0)
		{
			log_output(vphlog,"Failure: negative number of links");
			return 1;
		}

	{
		float matrix[12];

		log_output(vphlog,"Matrix:");
		err = vph_read_float_array(f,matrix,12);
		if (err != 0)
			{
				log_output(vphlog,"Failure to read matrix");
				return 1;
			}
		vph_report_float_array(matrix,12); 
	}

	for (li=0; li<links; li++)
		{
			char string[1024];
			int int_specifics[5];
			float specifics32[32];
			float matrix[12];
			float specifics15[32];
			short specific;

			log_output(vphlog,"link %ld",(long)li);		
			err = vph_read_string(f, string);			
			if (err != 0)
				{
					log_output(vphlog,"Failure to read string (link%ld)",(long)li);
					return 1;
				}
			log_output(vphlog,"NAME = %s", string);
	
			log_output(vphlog,"(physique specifics int*5)");
			err = vph_read_int_array(f,int_specifics,5);
			if (err != 0)
				{
					log_output(vphlog,"Failure to read int specifics index %d",err-1);
					return 1;
				}
			vph_report_int_array(int_specifics,5);
			
			log_output(vphlog,"(physique specifics float*32)");
			err = vph_read_float_array(f,specifics32,32);
			if (err != 0)
				{
					log_output(vphlog,"Failure to read specifics32 index %d",err-1);
					return 1;
				}
			vph_report_float_array(specifics32,32);
			

			log_output(vphlog,"Matrix ZM:");
			err = vph_read_float_array(f,matrix,12);
			if (err != 0)
				{
					log_output(vphlog,"Failure to read matrix index %d",err-1);
					return 1;
				}
			vph_report_float_array(matrix,12);
			
			log_output(vphlog,"(physique specifics float*15)");
			err = vph_read_float_array(f,specifics15,15);
			if (err != 0)
				{
					log_output(vphlog,"Failure to read specifics15 index %d",err-1);
					return 1;
				}
			vph_report_float_array(specifics15,15);
			
			err = vph_read_short(f,&specific);
			if (err != 0)
				{
					log_output(vphlog,"Failure to read specific short");
					return 1;
				}
			log_output(vphlog,"(physic specific short) %d  ",(int)specific);

		}


	err = vph_read_int(f,&num_objects);
	if (err != 0)
		{
			log_output(vphlog,"Failure to read num_objects");
			return 1;
		}
	log_output(vphlog,"num_objects = %d",(int)num_objects);
	if (num_objects<0)
		{
			log_output(vphlog,"num_objects < 0");
			return 1;
		}
	

	for (li=0; li<num_objects; li++)
		{
		
			long v,num_vertices;
			char string[1024];
			log_output(vphlog,"object %ld",(long)li);
					
			err = vph_read_string(f, string);			
			if (err != 0)
				{
					log_output(vphlog,"Failure to read string (object %ld)",(long)li);
					return 1;
				}
			log_output(vphlog,"NAME = %s", string);

			for (i=0; i<4; i++)
				{
					float offsetTM[3];
					log_output(vphlog,"OffsetTM Row (%d):",i);
					err = vph_read_float_array(f,offsetTM,3);
					if (err != 0)
						{
							log_output(vphlog,"Failure to read offsetTM element %d, row %d, object %ld",err-1,i,(long)li);
							return 1;
						}
					vph_report_float_array(offsetTM,3);
				}
	
			err = vph_read_long(f,&num_vertices);
			if (err != 0)
				{
					log_output(vphlog,"Failure to read number of vertices (object %ld)",(long)li);
					return 1;
				}
			log_output(vphlog,"Number of vertices = %ld",(long)num_vertices);
			if (num_vertices<0)
				{
					log_output(vphlog,"Failure: negative number of vertices");
					return 1;
				}

			for (v=0; v<num_vertices; v++)
				{
					int rigid_link;
					int deform_link;
					float rigid[3];
					float deform[3];

					err = vph_read_int(f,&rigid_link);			
					if (err != 0)
						{
							log_output(vphlog,"Failure to read rigid flag, vertex %ld, object %ld",(long)v,(long)li);
							return 1;
						}

					err = vph_read_float_array(f,rigid,3);
					if (err != 0)
						{
							log_output(vphlog,"Failure to read rigid element %d, vertex %d, object %ld",err-1,v,(long)li);
							return 1;
						}
					log_output(vphlog,"  rigid (%ld): (rigid_link=%d) %f %f %f",rigid_link,(long)v,rigid[0],rigid[1],rigid[2]);
					

					err = vph_read_int(f,&deform_link);			
					if (err != 0)
						{
							log_output(vphlog,"Failure to read deform flag, vertex %ld, object %ld",(long)v,(long)li);
							return 1;
						}

					err = vph_read_float_array(f,deform,3);
					if (err != 0)
						{
							log_output(vphlog,"Failure to read deform element %d, vertex %d, object %ld",err-1,v,(long)li);
							return 1;
						}
					log_output(vphlog,"  deform (%ld): (deform_link=%d,%s) %f %f %f",
						(long)v,
						deform_link,
						((deform_link<=0)?"rigid":"deformable"),
						rigid[0],rigid[1],rigid[2]);
					
				}	

				
		}

	return 0;
}



int vphmain( const char* vphfile, const char* outfile )
{
	FILE *f;
	int err;

	vphlog = log_open(outfile);
	if (vphlog == NULL)
		{
			printf("VPH -- Failure to open output log %s\n",outfile);
			return 1;
		}

	f = fopen(vphfile,"rb");
	if (f==NULL)
		{
			log_output(vphlog,"VPH: cant open input file%s\n",vphfile);
			log_close(&vphlog);
			return 1;
		}	
	
	log_output(vphlog,"VPH SCANNER:  file=%s",vphfile);

	err = vph_parse(f);
	if (err != 0)
		{
			log_output(vphlog,"vph_parse() returned an error %d",err);
		}

	if (fclose(f)!=0)
		{
			log_output(vphlog,"PATHENV: couldn't close output file %s\n",vphfile);
			log_close(&vphlog);
			return 1;
		}
	log_close(&vphlog);

	return 0;
}			
