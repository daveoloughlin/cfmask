#include "cal.h"
#include "const.h"
#define nint(A)(A<0?(int)(A-0.5):(int)(A+0.5))
/*
#define PRINT_X (1543)
#define PRINT_Y (  58)*/
#define PRINT_X (   0)
#define PRINT_Y (   0)

/* Functions */
/* !Revision:
 *
 * revision 1.0.0 9/13/2012  Gail Schmidt, USGS
 * - modified cal6 application to flag the saturated thermal pixels to be
 *   consistent with the processing of the reflective bands
 */

bool Cal(Lut_t *lut, int iband, Input_t *input, unsigned char *line_in, 
         int *line_out, int *line_out_qa, int iy) {
  int is,val,jband;
  float gain, bias, rad, ref_conv, ref, fval;
  int nsamp= input->size.s;
  int ifill= input->short_flag ? FILL_VAL[iband]: (int)lut->in_fill;
  float scale;
  float Gold, Gnew, Alpha, Grescale;
  jband= iband==5 ? 6 : iband;

    float esun[6];

    if(input->meta.sat== 5)
    {
     esun[0] = 1997.0;
     esun[1] = 1812.0;
     esun[2] = 1533.0;
     esun[3] = 1039.0;
     esun[4] = 230.8;
     esun[5] = 84.9;
    }
    else if(input->meta.sat == 4)
    {
     esun[0] = 1983.0;
     esun[1] = 1796.0;
     esun[2] = 1536.0;
     esun[3] = 1031.0;
     esun[4] = 220.0;
     esun[5] = 83.44;
    }
    else if(input->meta.sat == 3)
    {
     esun[0] = 1983.0;
     esun[1] = 1795.0;
     esun[2] = 1539.0;
     esun[3] = 1028.0;
     esun[4] = 219.8;
     esun[5] = 83.49;
    }
    else
    {
       printf("Unsupported satellite sensor\n");
       return false;
    }

  if ( input->dnout )
    scale=(input->dn_map[3]-input->dn_map[2]+1)/
          (input->dn_map[1]-input->dn_map[0]+1);

  if ( lut->recal_flag && lut->gnew_flag && 
     ( lut->gold_flag || lut->work_order_flag ) )
    {
    jband= iband==5 ? 6 : iband;
    if (  lut->work_order_flag )
      {
      Alpha= lut->work_order->final_gain[jband];
      Grescale= lut->work_order->DN_to_Radiance_gain[jband];
      Gold=  Alpha / Grescale;
      }
    else
      Gold= lut->gold->gains[jband];
    Gnew= lut->gnew->gains[jband];
    }
  else 
    {
    Gold= Gnew= 1.0;
    }

  //  gain = lut->meta.gain[iband];
  //  bias = ( lut->meta.bias[iband] * ( Gold / Gnew ));
  gain = input->meta.gain[iband];
  bias = input->meta.bias[iband];

  //  ref_conv = (PI * lut->dsun2) / (lut->esun[iband] * lut->cos_sun_zen);
  ref_conv = ((PI * input->dsun_doy[input->meta.acq_date.doy-1] *
               input->dsun_doy[input->meta.acq_date.doy-1])) / 
               (esun[iband] * cos(input->meta.sun_zen));

  if ( iy==0 )
    {
    printf("*** band=%1d gain=%f bias=%f ref_conv=%f=(PI*%f*%f)/(%f*%f) ***\n",
         iband+1,gain,bias,ref_conv,
         input->dsun_doy[input->meta.acq_date.doy-1], 
         input->dsun_doy[input->meta.acq_date.doy-1], esun[iband], 
           cos(input->meta.sun_zen));
    fflush(stdout);
    }

  for (is = 0; is < nsamp; is++) {

    val= getValue((unsigned char *)line_in, is, input->short_flag, input->swap_flag );
    /*if ( PRINT_X!=0 && PRINT_Y!=0 && is==PRINT_X && iy==PRINT_Y  ) 
      printf("val = %d ifill=%d qa_val=%d qa_fill=%d\n", val, ifill, line_out_qa[is], lut->qa_fill);*/

    if (val == ifill || line_out_qa[is]==lut->qa_fill ) {
      line_out[is] = lut->out_fill;
      continue;
    }
#if 0
    /* for saturated pixels, added by Feng (3/23/09) */
    if (val == SATU_VAL[iband]) {
      line_out[is] = lut->out_satu;
      continue;
    }
#endif
    fval= (float)val;

/*  if (  cal_stats->nvalid[iband]== 1  )  */
    if ( PRINT_X!=0 && PRINT_Y!=0 && is==PRINT_X && iy==PRINT_Y  ) 
      {
      jband= iband==5 ? 6 : iband;
      printf("%1d xy=(%4d,%4d) input_dn=%f ",jband,is,iy,fval);
      }
      
    if ( lut->gnew_flag && ( lut->gold_flag || lut->work_order_flag ) )
      fval*= ( Gold / Gnew );

    if ( lut->gnew_flag && ( lut->gold_flag || lut->work_order_flag ) )
    if ( PRINT_X!=0 && PRINT_Y!=0 && is==PRINT_X && iy==PRINT_Y  ) 
      {
      printf(" Gold=%f Gnew=%f Gold/Gnew=%f\n",Gold,Gnew,Gold/Gnew); fflush(stdout);
      }

    rad = (gain * fval) + bias;
    ref = rad * ref_conv;

    if ( PRINT_X!=0 && PRINT_Y!=0 && is==PRINT_X && iy==PRINT_Y  ) 
      {
      printf("  fval=%f gain=%f bias*(Gold/Gnew)=%f rad=%f ref_conv=%f ref=%f\n",fval,gain,bias,rad,ref_conv,ref);
      fflush(stdout);
      }

    if ( input->dnout )
    {
      line_out[is]=
        (int)(input->dn_map[2]+(((float)val-input->dn_map[0])*scale)+0.5);
    }
    else
    {
     line_out[is] = (int)(ref * 10000.0);
    }
  }

  return true;
}

bool Cal6(Lut_t *lut, Input_t *input, unsigned char *line_in, int *line_out, 
         int *line_out_qa, int iy) {
  int is, val;
  float gain, bias, rad, temp;
  int nsamp= input->size_th.s;
  int ifill= input->short_flag ? FILL_VAL6 : (int)lut->in_fill;
  float scale;
  float k1, k2;
  
    if(input->meta.sat == 5) /* Landsat 7 */
    {
       k1 = 666.09;
       k2 = 1282.71;
    }
    else if(input->meta.sat == 4) /* Landsat 5 */
    {
       k1 = 607.76;
       k2 = 1260.56;
    }
    else if(input->meta.sat == 3) /* Landsat 4 */
    {
       k1 = 671.62;
       k2 = 1284.30;
    }
    else
    {
       printf("Unsupported satellite sensor\n");
       return false;
    }

  if ( input->dnout )
    scale=(input->dn_map[3]-input->dn_map[2]+1)/(input->dn_map[1]-input->dn_map[0]+1);

  //  gain = lut->meta.gain_th;
  //  bias = lut->meta.bias_th;
  gain = input->meta.gain_th;
  bias = input->meta.bias_th;
  
  if ( iy==0 )
    {
    printf("*** band=%1d gain=%f bias=%f ***\n",6,gain,bias);
    fflush(stdout);
    }

  for (is = 0; is < nsamp; is++) {
    val= getValue((unsigned char *)line_in, is, input->short_flag, input->swap_flag );
    if (val == ifill || line_out_qa[is]==lut->qa_fill ) {
      line_out[is] = lut->out_fill;
      continue;
    }
#if 0
    /* for saturated pixels, added by Gail (9/13/2012 to match code added by
	   Feng (3/23/09) to the reflective bands) */
    if (val >= SATU_VAL6) {
      line_out[is] = lut->out_satu;
      continue;
    }
#endif
/* Put 100.0 (scale_factor_th) and 273.15 (th_zero_celcius_in_degrees_kelvin)
   into lut. */

    rad = (gain * (float)val) + bias;
    //    temp = ( lut->K2 / log(1.0 + (lut->K1/rad) ) );
    temp = ( k2 / log(1.0 + (k1/rad) ) );
    temp -= 273.15;

    if ( input->dnout )
      {
      line_out[is]=(int)(input->dn_map[2]+(((float)val-input->dn_map[0])*scale)+0.5);
      }
    else
    {
      line_out[is] = (int)(temp * 100.0);
    }

  }

  return true;
}

bool dn_to_bt(Input_t *input, unsigned char *line_in, int *line_out)
{
    float k1, k2;
    int is;

    if(input->meta.sat == 5) /* Landsat 7 */
    {
       k1 = 666.09;
       k2 = 1282.71;
    }
    else if(input->meta.sat == 4) /* Landsat 5 */
    {
       k1 = 607.76;
       k2 = 1260.56;
    }
    else if(input->meta.sat == 3) /* Landsat 4 */
    {
       k1 = 671.62;
       k2 = 1284.30;
    }
    else
    {
       printf("Unsupported satellite sensor\n");
       return false;
    }

    float temp;
    for (is = 0; is < input->size.s; is++)
    {
        temp = getValue((unsigned char *)line_in, is, input->short_flag, 
                   input->swap_flag );
        if (temp == 0)
            line_out[is] = -9999;
        else
        {
            temp = (input->meta.gain_th * (float)line_in[is]) + 
                    input->meta.bias_th;
            temp = k2 / log ((k1/temp)+1);
            line_out[is]  = (int) (100.0 * (temp - 273.15) + 0.5);
        }
    }

    return true;
}

bool dn_to_toa(Input_t *input, int ib, unsigned char *line_in, int *line_out)
{
    int is;
    float esun[6];

    if(input->meta.sat== 5)
    {
     esun[0] = 1997.0;
     esun[1] = 1812.0;
     esun[2] = 1533.0;
     esun[3] = 1039.0;
     esun[4] = 230.8;
     esun[5] = 84.9;
    }
    else if(input->meta.sat == 4)
    {
     esun[0] = 1983.0;
     esun[1] = 1796.0;
     esun[2] = 1536.0;
     esun[3] = 1031.0;
     esun[4] = 220.0;
     esun[5] = 83.44;
    }
    else if(input->meta.sat == 3)
    {
     esun[0] = 1983.0;
     esun[1] = 1795.0;
     esun[2] = 1539.0;
     esun[3] = 1028.0;
     esun[4] = 219.8;
     esun[5] = 83.49;
    }
    else
    {
       printf("Unsupported satellite sensor\n");
       return false;
    }

    float temp;
    for (is = 0; is < input->size.s; is++)
    {
        temp = getValue((unsigned char *)line_in, is, input->short_flag, 
                input->swap_flag );
        if (temp == 0)
            line_out[is] = -9999;
        else
        {   
            temp = (input->meta.gain[ib] * (float)line_in[is]) + 
                    input->meta.bias[ib];
            line_out[is] = (int)((10000.0 * PI * temp * 
                input->dsun_doy[input->meta.acq_date.doy-1] * 
                input->dsun_doy[input->meta.acq_date.doy-1]) / 
                   (esun[ib] * cos(input->meta.sun_zen)) + 0.5);
        }
    }

    return true;
}



/*************************************************************************
 *** this program returns the correct value (as an int) after swapping ***
 *** if if indicated                                                   ***
 *************************************************************************/
 int getValue(unsigned char* line_in, int ind, int short_flag, int swap_flag)
 {
 unsigned short int* line_in2_byte= (unsigned short int*)line_in;
 unsigned short int i2_temp;
 unsigned short int out_value;
 char* inc= (char*)&i2_temp;
 char* outc= (char*)&out_value;
 if ( short_flag )
   {
   if ( swap_flag ) 
     {
     i2_temp= (int)line_in2_byte[ ind ];
     outc[0]= inc[1];
     outc[1]= inc[0];
     }
   else
     out_value= (int)line_in2_byte[ ind ];
   }
 else
   out_value= (int)line_in[ ind ];
 return (int)out_value;
}

/*
!C******************************************************************************

!Description: 'computeBounds' computes the boundary corners of the output image
and also outputs the UL and LR corners.  For ascending scenes and scenes in
the polar regions, the scenes are flipped upside down.  The bounding coords
will be correct in North represents the northernmost latitude and South
represents the southernmost latitude.  However, the UL corner in this case
would be more south than the LR corner.  Comparing the UL and LR corners will
allow the user to determine if the scene is flipped.

!Prototype : 
  bool computeBounds(Geo_bounds_t *bounds, Geo_coord_t *ul_corner,
    Geo_coord_t lr_corner, Space_t *space, int nps, int nls)
 
!Input Parameters:
 space            space (transformation) definition structure that defines
                  the transformation between geo to map coordinates
                  (Space_t*)
 nps              image size number of samples (int)
 nls              image size number of lines   (int)

!Output Parameters:
 bounds           output boundary structure (Geo_bounds_t)
 ul_corner        output UL corner (Geo_coord_t)
 lr_corner        output LR corner (Geo_coord_t)

!Team Unique Header:
 ! Design Notes:
   1. An error status is returned when:
       a. the transformaion can not proceed
   2. Error messages are handled with the 'RETURN_ERROR' macro.

!END****************************************************************************
*/
bool computeBounds(Geo_bounds_t *bounds, Geo_coord_t *ul_corner,
    Geo_coord_t *lr_corner, Space_t *space, int nps, int nls)
{
  const float pixcorn_x[4]={-0.5,-0.5, 0.5, 0.5}; /* 4 corners of a pixel */
  const float pixcorn_y[4]={-0.5, 0.5,-0.5, 0.5}; /* 4 corners of a pixel */
  Img_coord_float_t img;
  Geo_coord_t geo;
  int i,ix,iy,ic;
  bounds->min_lat=  99999.9999;
  bounds->min_lon=  99999.9999;
  bounds->max_lat= -99999.9999;
  bounds->max_lon= -99999.9999;

  /* Determine the bounding coords */
  for ( i=0; i<(nps*2+nls*2); i++ )
    {
    if ( i<nps ) /* top edge */
      {
      ix= i;
      iy= 0;
      }
    else if ( i<(nps*2) )
      {
      ix= i-nps;
      iy= (nls-1);
      }
    else if ( i <(nps*2+nls) )
      {
      ix= 0;
      iy= i-2*nps;
      }
    else
      {
      ix= nps-1;
      iy= i-(2*nps+nls);
      }
    for ( ic=0; ic<4; ic++ )
      {
      img.l = (double)iy + pixcorn_y[ic];
      img.s = (double)ix + pixcorn_x[ic];
      img.is_fill = false;
      if (!FromSpace(space, &img, &geo))
      {
          printf("mapping from space\n");
          return false;
      }
      bounds->max_lat= max(bounds->max_lat,geo.lat);
      bounds->min_lat= min(bounds->min_lat,geo.lat);
      bounds->max_lon= max(bounds->max_lon,geo.lon);
      bounds->min_lon= min(bounds->min_lon,geo.lon);
      }
    }

  /* Determine the exact UL and LR corners */
  img.l = 0;
  img.s = 0;
  img.is_fill = false;
  if (!FromSpace(space, &img, ul_corner))
  {
      printf("mapping from space for UL corner\n");
      return false;
  }

  img.l = nls-1;
  img.s = nps-1;
  img.is_fill = false;
  if (!FromSpace(space, &img, lr_corner))
  {
      printf("mapping from space for LR corner\n");
      return false;
  }

  return true;
 }
