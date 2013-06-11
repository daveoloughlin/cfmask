#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lndcal.h"
#include "keyvalue.h"
#include "const.h"
#include "param.h"
#include "input.h"
#include "lut.h"
#include "output.h"
#include "cal.h"
#include "bool.h"
#include "error.h"
#include "util.h"

#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* Type definitions */
#define MSS 1
#define TM 2
#define ETM 3

#define NSDS (NBAND_CAL_MAX)
typedef enum {FAILURE = 0, SUCCESS = 1} Status_t;

/* Functions */
/* !Revision:
 *
 * revision 1.0.0 9/12/2012  Gail Schmidt, USGS
 * - modified the application to write the thermal band QA bits to the
 *   output thermal product
 */

int main (int argc, const char **argv) {
  Param_t *param;
  Input_t *input;
  Lut_t *lut;
  Output_t *output;
  Output_t *output_th= (Output_t*)NULL;
  int iline, isamp,oline, ib, jb, iz, val;
  unsigned char *line_in;
  int *line_out;
  int *line_out_qa;
  int *line_out_th= (int*)NULL;
  int *line_out_thz= (int*)NULL;
  unsigned char *line_in_thz= (unsigned char*)NULL;
  int nps,nls, nps6, nls6;
  int zoomx, zoomy;
  int i,odometer_flag=0;
  char msgbuf[1024];

  /*  Space_t *space; */
  Space_def_t space_def;
  Space_t *space;
  Geo_bounds_t bounds;
  Geo_coord_t ul_corner;
  Geo_coord_t lr_corner;
  char *grid_name = "Grid";
  int isds;
  int nsds = NSDS;
  char *sds_names[NSDS];
  int sds_types[NSDS];
  size_t input_psize;
  int qa_band= QA_BAND_NUM;
  int nband_refl= NBAND_REFL_MAX;
  int ifill, num_zero;
  int maxth=0;\
  int mss_flag=0;

  printf ("\nRunning lndcal ...\n");
  for (i=1; i<argc; i++)if ( !strcmp(argv[i],"-o") )odometer_flag=1;

  param = GetParam(argc, argv);
  if (param == (Param_t *)NULL) ERROR("getting runtime parameters", "main");
  
  if ( !setETM_GB(param) )
    ERROR("getting ETM_GB", "main");
    
  /*
    if (  param->ETM_GB )
       printf("*** ETM_GB = true ***\n");
    else
       printf("*** ETM_GB = false ***\n");
  if(1)exit(0); 
  */
   
  /* Open input file */

  input = OpenInput(param->input_header_file_name, param);
  if (input == (Input_t *)NULL) ERROR("bad input header file", "main");

  input->dnout=     param->dnout;
  input->dn_map[0]= param->dn_map[0];
  input->dn_map[1]= param->dn_map[1];
  input->dn_map[2]= param->dn_map[2];
  input->dn_map[3]= param->dn_map[3];

/* Zero(es) gain/bias of thermal band-6, changed to 0.0551583/1.2377996     9/15/05)*/
  if ( input->nband_th > 0 && param->output_therm_file_name != (char*)NULL &&
       fabs(input->meta.gain_th) < 0.5e-6  &&
       fabs(input->meta.bias_th) < 0.5e-6)
     {
       input->meta.gain_th = 0.0551583;
       input->meta.bias_th = 1.2377996;
       param->est_gainbias = 1;
     }

  /* Get Lookup table */
  lut = GetLut(param, input->nband, input);
  if (lut == (Lut_t *)NULL) ERROR("bad lut file", "main");

  if ( !lut->recal_flag ) 
    {
      /*
    if ( lut->work_order_flag ) 
      printf("*** work_order alg=(%s) date=(%d/%d/%d) ***\n",
              ALG[lut->work_order->algorithm]
	     ,lut->work_order->completion_year
	     ,lut->work_order->completion_month
	     ,lut->work_order->completion_day  );
      */
    }
  else
    {
    for ( ib=0; ib<6; ib++ ) 
      {
      if ( lut->work_order_flag && lut->recal_flag ) 
    printf(
"band=%d G-rescale=%10.6f alpha=%10.6f Gold=%10.6f Gnew=%10.6f Gold/Gnew=%10.6f\n",
         (ib==5?7:ib+1)                                      ,
         lut->work_order->DN_to_Radiance_gain[ib==5?6:ib]    ,  
         lut->work_order->final_gain[ib==5?6:ib]             , 
         lut->work_order->final_gain[ib==5?6:ib] / 
           lut->work_order->DN_to_Radiance_gain[ib==5?6:ib]  ,
         lut->gnew->gains[ib==5?6:ib]                        ,
         ((lut->work_order->final_gain[ib==5?6:ib] / 
           lut->work_order->DN_to_Radiance_gain[ib==5?6:ib])/ 
           lut->gnew->gains[ib==5?6:ib])
        );
else
    printf(
"band=%d  Gold=%10.6f Gnew=%10.6f Gold/Gnew=%10.6f\n",
         (ib==5?7:ib+1)                                      ,
         lut->gold->gains[ib==5?6:ib]                        ,
         lut->gnew->gains[ib==5?6:ib]                        ,
        (lut->gold->gains[ib==5?6:ib]/
         lut->gnew->gains[ib==5?6:ib])                       
        );
    fflush(stdout);
    }
  }

  /* Get space definition from file */

  if (!GetSpaceDefFile(param->input_header_file_name, &space_def))
    ERROR("getting definition from file", "main");

  /* Setup Space */

  space = SetupSpace(&space_def);
  if (space == (Space_t *)NULL) ERROR("setting up space", "main");
    
  /* compute bounds and UL/LR corners.  For ascending scenes and scenes in
     the polar regions, the scenes are flipped upside down.  The bounding coords
     will be correct in North represents the northernmost latitude and South
     represents the southernmost latitude.  However, the UL corner in this case
     would be more south than the LR corner.  Comparing the UL and LR corners
     will allow the user to determine if the scene is flipped. */

  if (!computeBounds(&bounds, &ul_corner, &lr_corner, space, input->size.s,
    input->size.l))
    ERROR("computing bounds", "main");

   nps6=  input->size_th.s;
   nls6=  input->size_th.l;
   nps =  input->size.s;
   nls =  input->size.l;
   zoomx= nint( (float)nps / (float)nps6 );
   zoomy= nint( (float)nls / (float)nls6 );

  /* Create and open output file */

  if (!CreateOutput(param->output_file_name))
    ERROR("creating output file", "main");
    
  if (input->meta.inst == INST_MSS)mss_flag=1; 

  output = OpenOutput(param->output_file_name, input->nband, input->meta.iband, 
		      &input->size, mss_flag);
  if (output == (Output_t *)NULL) ERROR("opening output file", "main");

   input_psize= ( input->file_type == INPUT_TYPE_BINARY_2BYTE_BIG ||
                 input->file_type == INPUT_TYPE_BINARY_2BYTE_LITTLE    ) ? 
                 sizeof(unsigned short int) :  sizeof(unsigned char);
		 
  line_in = (unsigned char *)calloc((size_t)input->size.s*nband_refl, input_psize);
   if (line_in == (unsigned char *)NULL) 
     ERROR("allocating input line buffer", "main");

  if ( param->output_therm_file_name  == (char *)NULL )
    printf("*** no output thermal file ***\n"); 

  /* Create and open output thermal file */

  if ( input->nband_th > 0 && param->output_therm_file_name != (char*)NULL ){

    if (!CreateOutput(param->output_therm_file_name))
      ERROR("creating output thermal file", "main");

    output_th = OpenOutput(param->output_therm_file_name, input->nband_th,  
              		   &input->meta.iband_th,  &input->size, mss_flag);
			   
    if ( output_th == (Output_t *)NULL)
      ERROR("opening output therm file", "main");

    line_out_th = (int *)calloc((size_t)input->size_th.s, sizeof(int));
    if (line_out_th == (int *)NULL) 
      ERROR("allocating thermal output line buffer", "main");

   if ( zoomx == 1 )
      {
      line_out_thz = (int *)line_out_th;
      line_in_thz = (unsigned char *)line_in;
      }
    else {
      line_out_thz = (int *)calloc((size_t)input->size.s, sizeof(int));
      if (line_out_thz == (int *)NULL) 
        ERROR("allocating thermal zoom output line buffer", "main");
      line_in_thz = (unsigned char *)calloc((size_t)input->size.s, sizeof(char));
      if (line_in_thz == (unsigned char *)NULL) 
        ERROR("allocating thermal zoom input line buffer", "main");
    }
 } 

  /* Allocate memory for lines */
  line_out = (int *)calloc((size_t)input->size.s, sizeof(int));
  if (line_out == (int *)NULL) 
    ERROR("allocating output line buffer", "main");

  line_out_qa = (int *)calloc((size_t)input->size.s, sizeof(int));
  if (line_out_qa == (int *)NULL) 
    ERROR("allocating qa output line buffer", "main");
  memset(line_out_qa,0,input->size.s*sizeof(int));    

  /* Do for each THERMAL line */
  oline= 0;
  if ( input->nband_th > 0 && param->output_therm_file_name != (char*)NULL )
  {
    ifill= input->short_flag ? FILL_VAL6: (int)lut->in_fill;
    for (iline = 0; iline < input->size_th.l; iline++)
      {
      ib=0;
      if (!GetInputLineTh(input, iline, line_in))
        ERROR("reading input data for a line", "main");

      if ( odometer_flag && ( iline==0 || iline ==(nls-1) || iline%100==0  ) ){ 
        if ( zoomy == 1 )
          printf("--- main loop BAND6 Line %d --- \r",iline); 
        else
          printf("--- main loop BAND6 Line in=%d out=%d --- \r",iline,oline); 
        fflush(stdout); 
      }

      memset(line_out_qa,0,input->size.s*sizeof(int)); 
   
      if ( !Cal6(lut, input, line_in, line_out_th, line_out_qa, iline) )
        ERROR("doing calibration for a line", "main");
#if 0
      if ( !dn_to_bt(input, line_in, line_out_th) )
        ERROR("doing calibration for a line", "main");
      int ii;
      for (ii=0;ii<input->size.s;ii++)
       line_out_qa[ii] = 0;
#endif
      if ( zoomx>1 ) 
        {
        zoomIt(line_out_thz, line_out_th, nps/zoomx, zoomx );
        zoomIt8((char*)line_in_thz,  (char*)line_in,   nps/zoomx, zoomx );
	}

      for ( iz=0; iz<zoomy; iz++ ) {
        for (isamp = 0; isamp < input->size.s; isamp++){
          val= getValue((unsigned char *)line_in_thz, isamp, input->short_flag, input->swap_flag );
          if ( val> maxth) maxth=val;
          if ( val==ifill) line_out_qa[isamp] = lut->qa_fill; 
          else if ( val>=SATU_VAL6 ) line_out_qa[isamp] = ( 0x000001 << 6 ); 
        }

        if ( oline<nls ) {
          if (!PutOutputLine(output_th, ib, oline, line_out_thz)){
	    sprintf(msgbuf,"write thermal error ib=%d oline=%d iline=%d",ib,oline,iline);
            ERROR(msgbuf, "main");
	      }

          if (input->meta.inst != INST_MSS) 
            if (!PutOutputLine(output_th, ib+1, oline, line_out_qa)){
	          sprintf(msgbuf,"write thermal QA error ib=%d oline=%d iline=%d",ib+1,oline,iline);
              ERROR(msgbuf, "main");
	      }
        }
        oline++;
      }

  } /* End loop for each thermal line */
  }
  if ( odometer_flag )printf("\n");

  if ( input->nband_th > 0 && param->output_therm_file_name != (char*)NULL )
 {
   if ( !PutMetadata6(output_th, &input->meta, lut, param) )
     ERROR("writing the thermal output metadata", "main");

   if (!CloseOutput(output_th)) ERROR("closing output thermal file", "main");
 }

  for (ib = 0; ib < input->nband; ib++) {
     output->satu_value[ib] = 0;
  }

  /* Do for each line */
  ifill= input->short_flag ? FILL_VAL[ib]: (int)lut->in_fill;
  for (iline = 0; iline < input->size.l; iline++){
/*if (iline == 6770){
	printf("  Debug:lndcal.280:iline= %d\n",iline);
}*/
    /* Do for each band */

    if ( odometer_flag && ( iline==0 || iline ==(nls-1) || iline%100==0  ) )
     {printf("--- main reflective loop Line %d ---\r",iline); fflush(stdout);}

    memset(line_out_qa,0,input->size.s*sizeof(int));    
    
    for (ib = 0; ib < input->nband; ib++) {
      if (!GetInputLine(input, ib, iline, &line_in[ib*nps]))
        ERROR("reading input data for a line", "main");
    }
    
    for (isamp = 0; isamp < input->size.s; isamp++){
      num_zero=0;
      for (ib = 0; ib < input->nband; ib++) {
        jb= (ib != 5 ) ? ib+1 : ib+2;
        val= getValue((unsigned char *)&line_in[ib*nps], isamp, input->short_flag, input->swap_flag );
	if ( val==ifill   )num_zero++;
        if ( val==SATU_VAL[ib] ) line_out_qa[isamp]|= ( 0x000001 <<jb ); 
      }
      /* Feng fixed bug by changing "|=" to "=" below (4/17/09) */
      if ( num_zero >  0 )line_out_qa[isamp] = lut->qa_fill; 
    }
    for (ib = 0; ib < input->nband; ib++) {

     if (!Cal(lut, ib, input, &line_in[ib*nps], line_out, line_out_qa,iline))
        ERROR("doing calibraton for a line", "main");
#if 0
      if (!dn_to_toa(input, ib, &line_in[ib*nps], line_out))
        ERROR("doing calibraton for a line", "main");
#endif
      int ii;
      for (ii=0;ii<input->size.s;ii++)
      {
       if (output->satu_value[ib] < line_out[ii])
        output->satu_value[ib] = line_out[ii];
      }

      if (!PutOutputLine(output, ib, iline, line_out))
        ERROR("reading input data for a line", "main");

    } /* End loop for each band */
        
  if (input->meta.inst != INST_MSS) 
    if (!PutOutputLine(output, qa_band, iline, line_out_qa))
      ERROR("writing qa data for a line", "main");
  } /* End loop for each line */
#if 0
    for (ib = 0; ib < input->nband; ib++)
     printf("output->satu_value[ib] = %d\n", output->satu_value[ib]);
#endif
  /* Write the output metadata */
    if ( !PutMetadata(output, input->nband, &input->meta, lut, param, &bounds,
      &ul_corner, &lr_corner))
    ERROR("writing the output metadata", "main");
  /* Close input files */

  if (!CloseInput(input)) ERROR("closing input file", "main");
  if (!CloseOutput(output)) ERROR("closing input file", "main");
  
  if (input->meta.inst == INST_MSS) nsds--;
  
  /* Add the QA band to the number of SDSs */

  for (isds = 0; isds < nsds; isds++) {
    sds_names[isds] = output->sds[isds].name;
    sds_types[isds] = output->sds[isds].type;
  }
  if (!PutSpaceDefHDF(&space_def, param->output_file_name, nsds,
                      sds_names, sds_types, grid_name))
    ERROR("putting space metadata in HDF file", "main");

  /* do the same for the thermal band and its QA band */

  if ( input->nband_th > 0 && param->output_therm_file_name != (char*)NULL )
  {
    nsds = 2;
    for (isds = 0; isds < nsds; isds++) {
      sds_names[isds] = output_th->sds[isds].name;
      sds_types[isds] = output_th->sds[isds].type;
    }
    if (!PutSpaceDefHDF(&space_def, param->output_therm_file_name, nsds, 
                        sds_names, sds_types, grid_name))
     ERROR("putting space metadata in thermal HDF file", "main");
  }
  /* Free memory */

  if (!FreeParam(param)) 
    ERROR("freeing parameter stucture", "main");

  if (!FreeInput(input)) 
    ERROR("freeing input file stucture", "main");

  if (!FreeLut(lut)) 
    ERROR("freeing lut file stucture", "main");

  if (!FreeOutput(output)) 
    ERROR("freeing output file stucture", "main");

  free(line_out);
  free(line_in);
  /* All done */

  printf ("lndcal complete.\n");
  return (EXIT_SUCCESS);
}
