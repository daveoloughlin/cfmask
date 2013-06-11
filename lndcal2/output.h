/*
!C****************************************************************************

!File: output.h

!Description: Header file for output.c - see output.c for more information.

!Revision History:
 Revision 1.0 2001/05/08
 Robert Wolfe
 Original Version.

!Team Unique Header:
  This software was developed by the MODIS Land Science Team Support 
  Group for the Laboratory for Terrestrial Physics (Code 922) at the 
  National Aeronautics and Space Administration, Goddard Space Flight 
  Center, under NASA Task 92-012-00.

 ! References and Credits:

  ! MODIS Science Team Member:
      Christopher O. Justice
      MODIS Land Science Team           University of Maryland
      justice@hermes.geog.umd.edu       Dept. of Geography
      phone: 301-405-1600               1113 LeFrak Hall
                                        College Park, MD, 20742

  ! Developers:
      Robert E. Wolfe (Code 922)
      MODIS Land Team Support Group     Raytheon ITSS
      robert.e.wolfe.1@gsfc.nasa.gov    4400 Forbes Blvd.
      phone: 301-614-5508               Lanham, MD 20770  

 ! Design Notes:
   1. Structure is declared for the 'output' data type.
  
!END****************************************************************************
*/

#ifndef OUTPUT_H
#define OUTPUT_H

#include "lndcal.h"
#include "input.h"
#include "bool.h"
#include "myhdf.h"
#include "lut.h"
#include "param.h"

/* Structure for the 'output' data type */

typedef struct {
  char *file_name;      /* Output file name */
  bool open;            /* Flag to indicate whether output file is open 
                           for access; 'true' = open, 'false' = not open */
  int nband;            /* Number of bands */
  Img_coord_int_t size; /* Output image size */
  int32 sds_file_id;    /* SDS file id */
  Myhdf_sds_t sds[NBAND_CAL_MAX];
                        /* SDS data structures */
  Myhdf_dim_t dim[MYHDF_MAX_RANK];
                        /* Dimension data structure */
  int16 *buf[NBAND_REFL_MAX];
                        /* Output data buffer (one line of data) */
  unsigned char *qabuf;
  int satu_value[NBAND_REFL_MAX];
} Output_t;

/* Prototypes */

bool CreateOutput(char *file_name);
Output_t *OpenOutput(char *file_name, int nband, int *iband, 
                     Img_coord_int_t *size, int mss_flag);
bool PutOutputLine(Output_t *this, int iband, int iline, int *line);
bool CloseOutput(Output_t *this);
bool FreeOutput(Output_t *this);
bool PutMetadata(Output_t *this, int nband, Input_meta_t *meta, Lut_t *lut,
  Param_t *param, Geo_bounds_t *bounds, Geo_coord_t *ul_corner,
  Geo_coord_t *lr_corner);
bool PutMetadata6(Output_t *this, Input_meta_t *meta, Lut_t *lut, Param_t *param);

#endif
