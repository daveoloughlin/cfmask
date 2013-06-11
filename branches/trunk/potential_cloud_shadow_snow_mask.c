#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "cv.h"
#include "ml.h"
#include "cxcore.h"
#include "highgui.h"
#include "ias_misc_2d_array.h"
#include "input.h"

#define MINSIGMA 1e-5

void majority_filter(unsigned char **mask, int nrows, int ncols)
{
 int row, col;
 int cloud_pixels = 0;

   for (row = 0; row < nrows; row++)
   {
      for (col = 0; col < ncols; col++)
      {
       if ((row-1) > 0 && (row+1) < (nrows-1))
        {
         cloud_pixels = 0;
         if ((col-1) > 0 && (col+1) < (ncols-1))
          {
           if (mask[row-1][col-1] == 1)
            cloud_pixels++;
           if (mask[row-1][col] == 1)
            cloud_pixels++;
           if (mask[row-1][col+1] == 1)
            cloud_pixels++;
           if (mask[row][col-1] == 1)
            cloud_pixels++;
           if (mask[row][col] == 1)
            cloud_pixels++;
           if (mask[row][col+1] == 1)
            cloud_pixels++;
           if (mask[row+1][col-1] == 1)
            cloud_pixels++;
           if (mask[row+1][col] == 1)
            cloud_pixels++;
           if (mask[row+1][col+1] == 1)
            cloud_pixels++;
          }
        }
       if (cloud_pixels >= 5)
        mask[row][col] = 1;

       cloud_pixels = 0;
       if (row == 0 && col == 0)
        {
           if (mask[row][col] == 1)
            cloud_pixels++;
           if (mask[row][col+1] == 1)
            cloud_pixels++;
            cloud_pixels++;
           if (mask[row+1][col] == 1)
            cloud_pixels++;
           if (mask[row+1][col+1] == 1)
            cloud_pixels++;
        }
       if (cloud_pixels >= 2)
        mask[row][col] = 1;

       cloud_pixels = 0;
       if (row == 0 && col == ncols-1)
        {
           if (mask[row][col-1] == 1)
            cloud_pixels++;
           if (mask[row][col] == 1)
            cloud_pixels++;
            cloud_pixels++;
           if (mask[row+1][col-1] == 1)
            cloud_pixels++;
           if (mask[row+1][col] == 1)
            cloud_pixels++;
        }
       if (cloud_pixels >= 2)
        mask[row][col] = 1;

       cloud_pixels = 0;
       if (row == nrows-1 && col == 0)
        {
           if (mask[row-1][col] == 1)
            cloud_pixels++;
           if (mask[row-1][col+1] == 1)
            cloud_pixels++;
            cloud_pixels++;
           if (mask[row][col] == 1)
            cloud_pixels++;
           if (mask[row][col+1] == 1)
            cloud_pixels++;
        }
       if (cloud_pixels >= 2)
        mask[row][col] = 1;

       cloud_pixels = 0;
       if (row == nrows-1 && col == ncols-1)
        {
           if (mask[row-1][col-1] == 1)
            cloud_pixels++;
           if (mask[row-1][col] == 1)
            cloud_pixels++;
            cloud_pixels++;
           if (mask[row][col] == 1)
            cloud_pixels++;
           if (mask[row][col-1] == 1)
            cloud_pixels++;
        }
       if (cloud_pixels >= 2)
        mask[row][col] = 1;

       cloud_pixels = 0;
       if (row == 0 && (col > 0 && col < (ncols-1)))
        {
         if (mask[row][col-1] == 1)
          cloud_pixels++;
         if (mask[row][col] == 1)
          cloud_pixels++;
         if (mask[row][col+1] == 1)
          cloud_pixels++;
         if (mask[row+1][col-1] == 1)
          cloud_pixels++;
         if (mask[row+1][col] == 1)
          cloud_pixels++;
         if (mask[row+1][col+1] == 1)
          cloud_pixels++;
        }
       if (cloud_pixels >= 3)
        mask[row][col] = 1;

       cloud_pixels = 0;
       if (row == ncols-1 && (col > 0 && col < (ncols-1)))
        {
         if (mask[row][col-1] == 1)
          cloud_pixels++;
         if (mask[row][col] == 1)
          cloud_pixels++;
         if (mask[row][col+1] == 1)
          cloud_pixels++;
         if (mask[row-1][col-1] == 1)
          cloud_pixels++;
         if (mask[row-1][col] == 1)
          cloud_pixels++;
         if (mask[row-1][col+1] == 1)
          cloud_pixels++;
        }
       if (cloud_pixels >= 3)
        mask[row][col] = 1;

       cloud_pixels = 0;
       if ((row > 0 && row < (nrows-1)) && col == 0)
        {
         if (mask[row-1][col] == 1)
          cloud_pixels++;
         if (mask[row][col] == 1)
          cloud_pixels++;
         if (mask[row+1][col] == 1)
          cloud_pixels++;
         if (mask[row-1][col+1] == 1)
          cloud_pixels++;
         if (mask[row][col+1] == 1)
          cloud_pixels++;
         if (mask[row+1][col+1] == 1)
          cloud_pixels++;
        }
       if (cloud_pixels >= 3)
        mask[row][col] = 1;

       cloud_pixels = 0;
       if ((row > 0 && row < (nrows-1)) && col == ncols-1)
        {
         if (mask[row-1][col-1] == 1)
          cloud_pixels++;
         if (mask[row][col-1] == 1)
          cloud_pixels++;
         if (mask[row+1][col-1] == 1)
          cloud_pixels++;
         if (mask[row-1][col] == 1)
          cloud_pixels++;
         if (mask[row][col] == 1)
          cloud_pixels++;
         if (mask[row+1][col-1] == 1)
          cloud_pixels++;
        }
       if (cloud_pixels >= 3)
        mask[row][col] = 1;
      }
   }
}

bool potential_cloud_shadow_snow_mask
(
 Input_t *input,
 float cloud_prob_threshold,
 float *ptm,
 float *t_templ,
 float *t_temph,
 unsigned char **cloud_mask,
 unsigned char **shadow_mask,
 unsigned char **snow_mask,
 unsigned char **water_mask,
 unsigned char **final_mask
)
{
    char errstr[MAX_STR_LEN];
    int nrows = input->size.l;
    int ncols = input->size.s;
    int ib = 0;
    int row =0; 
    int col = 0;
    int mask_counter = 0;
    int clear_pixel_counter = 0;
    int clear_land_pixel_counter = 0;
    float ndvi, ndsi;
    int16 *f_temp = NULL;
    int16 *f_wtemp = NULL;
    float visi_mean;
    float whiteness = 0;
    float hot;
    float lndptm;
    float l_pt;
    float h_pt;
    int mask;
    float t_wtemp;
    float **wfinal_prob;
    float **final_prob;
    float wtemp_prob;
    int t_bright;
    float brightness_prob;
    int t_buffer;
    float temp_l;
    float temp_prob;
    float vari_prob;
    float max_value;
    float *prob = NULL;
    float clr_mask;
    float wclr_mask;
    int16 *nir = NULL;
    int16 *swir = NULL;
    float backg_b4;
    float backg_b5;
    float shadow_prob;
    int status;

    /* Dynamic memory allocation */
    unsigned char **clear_mask;
    unsigned char **clear_land_mask;

    clear_mask = (unsigned char **)ias_misc_allocate_2d_array(input->size.l, 
                 input->size.s, sizeof(unsigned char)); 
    clear_land_mask = (unsigned char **)ias_misc_allocate_2d_array(
                 input->size.l, input->size.s, sizeof(unsigned char)); 
    if (clear_mask == NULL || clear_land_mask ==NULL)
    {
        sprintf (errstr, "Allocating mask memory");
        ERROR (errstr, "pcloud");
    }
    
    printf("The first pass\n");

    /* Loop through each line in the image */
    for (row = 0; row < nrows; row++)
    {
        /* Print status on every 100 lines */
        if (!(row%1000)) 
        {
           printf ("Processing line %d\r",row);
           fflush (stdout);
        }

        /* For each of the image bands */
        for (ib = 0; ib < input->nband; ib++)
        {
            /* Read each input reflective band -- data is read into
               input->buf[ib] */
            if (!GetInputLine(input, ib, row))
            {
                sprintf (errstr, "Reading input image data for line %d, "
                    "band %d", row, ib);
                ERROR (errstr, "pcloud");
            }
        }

	/* For the thermal band */
	/* Read the input thermal band -- data is read into input->therm_buf */
	if (!GetInputThermLine(input, row))
        {
	  sprintf (errstr, "Reading input thermal data for line %d", row);
	  ERROR (errstr, "pcloud");
	}

   for (col = 0; col < ncols; col++)
    {
     if ((input->buf[2][col] +  input->buf[3][col]) != 0)
      ndvi = (input->buf[3][col] -  input->buf[2][col]) / 
       (input->buf[3][col] +  input->buf[2][col]);
     else
      ndvi = 0.01;

     if ((input->buf[1][col] +  input->buf[4][col]) != 0)
      ndsi = (input->buf[1][col] -  input->buf[4][col]) / 
       (input->buf[1][col] +  input->buf[4][col]);
     else
      ndsi = 0.01;

     /* process non-fill pixels only */
     if (input->therm_buf[col] > -9999)
       {
	 mask = 1;
	 mask_counter++;
       }
     else
       mask = 0;

     /* Basic cloud test */
     if ((ndsi < 0.8) && (ndvi < 0.8) && 
         (input->buf[5][col] > 300) && (input->therm_buf[col] < 2700))
         cloud_mask[row][col] = 1;
     else
         cloud_mask[row][col] = 0;

     /* It takes every snow pixels including snow pixel under thin clouds or 
        icy clouds */
     if ((ndsi > 0.15) && (input->therm_buf[col] < 380) && 
         (input->buf[3][col] > 1100) && (input->buf[1][col] > 1000))
         snow_mask[row][col] = 1;
     else
         snow_mask[row][col] = 0;

     /* Zhe's water test (works over thin cloud) */
     if (((ndvi < 0.01) && (input->buf[3][col] < 1100)) || 
         ((ndvi < 0.1) && (ndvi > 0.0) && 
          (input->buf[3][col] < 500)))
         water_mask[row][col] = 1;
     else 
         water_mask[row][col] = 0;
     if (mask == 0)
         water_mask[row][col] = 255;

     /* visible bands flatness (sum(abs)/mean < 0.6 => brigt and dark cloud) */
     visi_mean = (input->buf[0][col] + input->buf[1][col] +
                  input->buf[2][col]) / 3.0;
     whiteness = ((abs(input->buf[0][col] - visi_mean) + 
      abs(input->buf[1][col] - visi_mean) +
                   abs(input->buf[2][col] - visi_mean)))/ visi_mean;

     /* Update cloud_mask,  if one visible band is saturated, whiteness = 0 */
     if (input->buf[0][col] > input->meta.therm_satu_value_ref ||
	 input->buf[1][col] > input->meta.therm_satu_value_ref ||
	 input->buf[2][col] > input->meta.therm_satu_value_ref)
      whiteness = 0;
     if (cloud_mask[row][col] == 1 && whiteness < 0.7)
      cloud_mask[row][col] = 1; 
     else
      cloud_mask[row][col] = 0;

     /* Haze test */
     hot = input->buf[0][col] - 0.5 *input->buf[2][col] - 800;
     if (cloud_mask[row][col] == 1 && (hot > 0.0 || abs(whiteness) < MINSIGMA))
       cloud_mask[row][col] = 1;
     else
       cloud_mask[row][col] = 0;

     /* Ratio 4/5 > 0.75 test */
     if (cloud_mask[row][col] == 1 && (input->buf[3][col] / 
         input->buf[4][col] > 0.75))
       cloud_mask[row][col] = 1;
     else
       cloud_mask[row][col] = 0;
     
     /* Test whether use thermal band or not */
     if (cloud_mask[row][col] == 0 && mask == 1)
       {
	 clear_mask[row][col] = 1;
	 clear_pixel_counter++;

       }
     else
         clear_mask[row][col] = 0;

     if (water_mask[row][col] != 1 && clear_mask[row][col] == 1)
       {
	 clear_land_mask[row][col] = 1;
         clear_land_pixel_counter++;
       }
     else
       clear_land_mask[row][col] = 0;
    }
}

    *ptm = 100. * ((float)clear_pixel_counter / (float)mask_counter);
    lndptm = 100. * ((float)clear_land_pixel_counter / (float)mask_counter);

    if (*ptm <= 0.1)
     majority_filter(cloud_mask, nrows, ncols);
    else
    {
        f_temp = malloc(input->size.l * input->size.s * sizeof(int16));
	f_wtemp = malloc(input->size.l * input->size.s * sizeof(int16));
	if (f_temp == NULL   || f_wtemp == NULL)
	{
	     sprintf (errstr, "Allocating temp memory");
	     ERROR (errstr, "pcloud");
	}
    }
     
     printf("The second pass\n");
     int16 f_temp_max = 0;
     int16 f_temp_min = 0;
     int16 f_wtemp_max = 0;
     int16 f_wtemp_min = 0;
     int index = 0;
     int index2 = 0;
    /* Loop through each line in the image */
    for (row = 0; row < nrows; row++)
    {
        /* Print status on every 100 lines */
        if (!(row%1000)) 
        {
	     printf ("Processing line %d\r", row);
	     fflush (stdout);
	   }
	 
	 /* For each of the image bands */
	 for (ib = 0; ib < input->nband; ib++)
	   {
	     /* Read each input reflective band -- data is read into
		input->buf[ib] */
	     if (!GetInputLine(input, ib, row))
	       {
		 sprintf (errstr, "Reading input image data for line %d, "
			  "band %d", row, ib);
		 ERROR (errstr, "pcloud");
	       }
	   }

	 /* For the thermal band */
	 /* Read the input thermal band -- data is read into input->therm_buf */
	 if (!GetInputThermLine(input, row))
	   {
	     sprintf (errstr, "Reading input thermal data for line %d", row);
	     ERROR (errstr, "pcloud");
	   }

	 for (col =0; col < ncols; col++)
	   {
	     if (*ptm <= 0.1) /* No thermal test, 
                                meaningless for snow detection */
	     {
		 /* All cloud */
		 if (cloud_mask[row][col] != 1)
		   shadow_mask[row][col] = 1;
                 else
		   shadow_mask[row][col] = 0;
#if 0
                 /* Tempoarary outpouts */
                 if (water_mask[row][col] == 1)
                     final_mask[row][col] = 1;
                 if (shadow_mask[row][col] == 1)
                     final_mask[row][col] = 2;
                 if (cloud_mask[row][col] == 1)
	             final_mask[row][col] = 4;
                 if (input->therm_buf[col] = -9999)
                     final_mask[row][col] = 255;
#endif
	     }
	     else
	     {		 
		 if (lndptm >= 0.1)
		 {
                     /* get clear land temperature */
		     if (clear_land_mask[row][col] == 1 && 
                         input->therm_buf[col] != -9999)
	             {
			 f_temp[index] = input->therm_buf[col];
                         if (f_temp_max < f_temp[index])
                            f_temp_max = f_temp[index];
                         if (f_temp_min > f_temp[index])
                            f_temp_min = f_temp[index];
			 index++;
	             }
	         }
		 else
	         {
		     /*get clear water temperature */
		     if (clear_mask[row][col] == 1 &&  
                         input->therm_buf[col] != -9999)
	             {
			 f_temp[index] = input->therm_buf[col];
                         if (f_temp_max < f_temp[index])
                            f_temp_max = f_temp[index];
                         if (f_temp_min > f_temp[index])
                            f_temp_min = f_temp[index];
			 index++;
	             }
	         }
		 if (water_mask[row][col] == 1 && input->therm_buf[col] <= 300
                     && input->therm_buf[col] != -9999)
	         {
		     f_wtemp[index2] = input->therm_buf[col];
                     if (f_wtemp[index2] > f_wtemp_max)
                        f_wtemp_max = f_wtemp[index2];
                     if (f_wtemp[index2] < f_wtemp_max)
                        f_wtemp_min = f_wtemp[index2];
                     index2++;
	         }
	     }
         }
    }

   printf("Clear sky pixel percentage in this scene = %.2f\n", *ptm);

   if (*ptm <= 0.1)
   {
      *t_templ = -1.0;
      *t_temph = -1.0;
      return 0;
   }
   else
   {
     /* Tempearture for snow test */
     l_pt = 0.175;
     h_pt = 1 - l_pt;

     printf("====%d,%d,%d,%d\n",f_wtemp_max,f_wtemp_min,f_temp_max,f_temp_min);

#if 0
     prctile(f_wtemp, index2 + 1, 100*h_pt, &t_wtemp);
     /* 0.175 percentile background temperature (low) */
     prctile(f_temp, index + 1, 100*l_pt, t_templ);
     /* 0.825 percentile background temperature (high) */
     prctile (f_temp, index + 1, 100*h_pt, t_temph);
#endif
     t_wtemp = h_pt * (float)(f_wtemp_max-f_wtemp_min) + (float)f_wtemp_min;
     *t_templ = l_pt * (float)(f_temp_max-f_temp_min) + (float)f_temp_min;
     *t_temph = h_pt * (float)(f_temp_max-f_temp_min) + (float)f_temp_min; 
     int f_temp_length;
     int f_wtemp_length;

     f_temp_length = f_temp_max - f_temp_min + 1;
     f_wtemp_length = f_wtemp_max - f_wtemp_min + 1;
#if 0
     prctile(f_wtemp, index2 + 1, f_wtemp_length, 100*h_pt, &t_wtemp);
     /* 0.175 percentile background temperature (low) */
     prctile(f_temp, index + 1, f_temp_length, 100*l_pt, t_templ);
     /* 0.825 percentile background temperature (high) */
     prctile (f_temp, index + 1, f_temp_length, 100*h_pt, t_temph);
#endif
     printf("index, index2 = %d,%d\n",index,index2);

     /* Temperature test */
     t_buffer = 4*100;    
     *t_templ -= t_buffer;
     *t_temph += t_buffer;
     temp_l=*t_temph-*t_templ;

     printf("t_wtemp,t_templ,t_temph = %f,%f,%f\n",t_wtemp,*t_templ,*t_temph);

     /* Relase f_temp memory */
     free(f_wtemp);
     free(f_temp);

     wfinal_prob = (float **)ias_misc_allocate_2d_array(input->size.l, 
                 input->size.s, sizeof(float)); 
     final_prob = (float **)ias_misc_allocate_2d_array(input->size.l, 
                 input->size.s, sizeof(float)); 
     if (wfinal_prob == NULL   ||  final_prob == NULL)
     {
	 sprintf (errstr, "Allocating prob memory");
	 ERROR (errstr, "pcloud");
     }

     printf("The third pass\n");
     /* Loop through each line in the image */
     for (row = 0; row < nrows; row++)
     {
        /* Print status on every 100 lines */
        if (!(row%1000)) 
        {
           printf ("Processing line %d\r",row);
           fflush (stdout);
        }

        /* For each of the image bands */
        for (ib = 0; ib < input->nband; ib++)
        {
            /* Read each input reflective band -- data is read into
               input->buf[ib] */
            if (!GetInputLine(input, ib, row))
            {
                sprintf (errstr, "Reading input image data for line %d, "
                    "band %d", row, ib);
                ERROR (errstr, "pcloud");
            }
        }

	/* For the thermal band */
	/* Read the input thermal band -- data is read into input->therm_buf */
	if (!GetInputThermLine(input, row))
        {
          sprintf (errstr, "Reading input thermal data for line %d", row);
          ERROR (errstr, "pcloud");
	}

	for (col = 0; col <ncols; col++)
	{
	    /* Get cloud prob over water */
	    /* Temperature test over water */
	    wtemp_prob = (t_wtemp - input->therm_buf[col]) / 400.0;
	    
	    /* Brightness test (over water) */
	    t_bright = 1100;
	    brightness_prob = input->buf[4][col] / t_bright;
	    if (brightness_prob > 1)
	      brightness_prob = 1;
	    
	    /*Final prob mask (water), cloud over water probability */
	    wfinal_prob[row][col] =100 * wtemp_prob * brightness_prob;
	    
            temp_prob=(*t_temph-input->therm_buf[col]) / temp_l;
	    /* Temperature can have prob > 1 */
	    if (temp_prob < 0)
	      temp_prob = 0;
	    
            if ((input->buf[2][col] +  input->buf[3][col]) != 0)
                ndvi = (input->buf[3][col] -  input->buf[2][col]) / 
                   (input->buf[3][col] +  input->buf[2][col]);
            else
                ndvi = 0.01;

            if ((input->buf[1][col] +  input->buf[4][col]) != 0)
                ndsi = (input->buf[1][col] -  input->buf[4][col]) / 
                    (input->buf[1][col] +  input->buf[4][col]);
            else
                ndsi = 0.01;
	    
	    /* NDVI and NDSI should not be negative */
	    if (input->buf[2][col] >= input->meta.therm_satu_value_ref 
                && ndsi < 0)
	      ndsi = 0;
	    if (input->buf[3][col] >= input->meta.therm_satu_value_ref 
                && ndvi < 0)
	      ndvi = 0;
	    
	    /*      Vari_prob=1-max(max(abs(NDSI),abs(NDVI)),whiteness); */
	    if (abs(ndsi) > abs(ndvi))
	      max_value = abs(ndsi);
	    else
	      max_value = abs(ndvi);
	    if (whiteness > max_value)
	      max_value = whiteness;
	    vari_prob = 1 - max_value;
	    
	    /*Final prob mask (land) */
	    final_prob[row][col] = 100 * (temp_prob * vari_prob);
        }
     }

     prob = malloc(input->size.l * input->size.s * sizeof(float));
     if(prob == NULL)
     {
          sprintf (errstr, "Allocating prob memory");
	  ERROR (errstr, "pcloud");
     }

     float prob_max = 0.0;
     float prob_min = 0.0;
     int index3 = 0;
     for (row = 0; row < nrows; row++)
     {
	for (col = 0; col <ncols; col++)
	{	    
	    if (clear_land_mask[row][col] == 1);
	    {
		prob[index3] = final_prob[row][col];
                if ((prob[index3] - prob_max) > MINSIGMA)
                    prob_max = prob[index3];
                if ((prob_min - prob[index3]) > MINSIGMA)
                    prob_min = prob[index3];
		index3++;
	    }
        }
    }

    /*Dynamic threshold for land */
    //    prctile2(prob, index3+1, 100*h_pt, &clr_mask);
    printf("index3,prob_max,prob_min =%d, %f, %f\n",index3,prob_max,prob_min);
    clr_mask = h_pt * (prob_max - prob_min) + prob_min;
    printf("clr_mask =%d, %f\n",index3,clr_mask);
#if 0
    prctile2(prob, index3+1, 100*h_pt, &clr_mask);
    printf("clr_mask = %f\n",clr_mask);
#endif
    clr_mask += cloud_prob_threshold;

    printf("clr_mask = %f\n",clr_mask);
	    
    /* Relase memory for prob */
    free(prob);
	    
    /* Fixed threshold for water */
    wclr_mask = 50.0;
	    
    printf("pcloud probability threshold (land) = %.2f\n", clr_mask);
    
    printf("The fourth pass\n");
    /* Loop through each line in the image */
    for (row = 0; row < nrows; row++)
    {
        /* Print status on every 100 lines */
        if (!(row%1000)) 
        {
           printf ("Processing line %d\r",row);
           fflush (stdout);
        }

        /* For each of the image bands */
        for (ib = 0; ib < input->nband; ib++)
        {
            /* Read each input reflective band -- data is read into
               input->buf[ib] */
            if (!GetInputLine(input, ib, row))
            {
                sprintf (errstr, "Reading input image data for line %d, "
                    "band %d", row, ib);
                ERROR (errstr, "pcloud");
            }
        }

	/* For the thermal band */
	/* Read the input thermal band -- data is read into input->therm_buf */
	if (!GetInputThermLine(input, row))
        {
	  sprintf (errstr, "Reading input thermal data for line %d", row);
	  ERROR (errstr, "pcloud");
	}

        for (col =0; col < ncols; col++)
        {
           if ((cloud_mask[row][col] == 1 && final_prob[row][col] > clr_mask &&
              water_mask[row][col] == 0) || (cloud_mask[row][col] == 1 && 
              wfinal_prob[row][col] > wclr_mask && water_mask[row][col] == 1) 
              || (final_prob[row][col] > 99 && water_mask[row][col] == 0) ||
              (input->therm_buf[col] < *t_templ - 3500))
                 cloud_mask[row][col] = 1;
        }
    }

    /* Free the memory */
    status = ias_misc_free_2d_array((void **)wfinal_prob);
    status = ias_misc_free_2d_array((void **)final_prob);

    /* Band 4 flood fill */
    nir = malloc(input->size.l * input->size.s * sizeof(int16)); 
    swir = malloc(input->size.l * input->size.s * sizeof(int16)); 
    if (nir == NULL   || swir == NULL)
    {
        sprintf(errstr, "Allocating nir and swir memory");
        ERROR (errstr, "pcloud");
    }
      
    int16 nir_max = 0;
    int16 swir_max = 0;
    index = 0;
    index2 = 0;
    for (row = 0; row < nrows; row++)
    {
      for (col = 0; col < ncols; col++)
      {
        if (clear_land_mask[row][col] == 1)
	{
	  nir[index] = input->buf[3][col];
          if (nir[index] > nir_max)
           nir_max = nir[index];
	  index++;
	}
      
        if (clear_land_mask[row][col] == 1)
	{
	  nir[index2] = input->buf[4][col];
          if (swir[index2] > swir_max)
           swir_max = swir[index2];
	  index2++;
	}
      }
    }

    status = ias_misc_free_2d_array((void **)clear_mask);
    status = ias_misc_free_2d_array((void **)clear_land_mask);

    /* Improve cloud mask by majority filtering */
    majority_filter(cloud_mask, nrows, ncols);

    /* Estimating background (land) Band 4 Ref */
    backg_b4 = l_pt * nir_max;
    backg_b5 = h_pt * swir_max;
    /* Release the memory */
    free(nir);
    free(swir);
	
    /* May need allocate two memory for new band 4 and 5 after imfill 
       (flood filling), also may need read in whole scene of bands 4 and 5
       for flood filling purpose */
    int16 **new_nir;
    int16 **new_swir;   

    new_nir = (int16 **)ias_misc_allocate_2d_array(input->size.l, 
              input->size.s, sizeof(int16)); 
    new_swir = (int16 **)ias_misc_allocate_2d_array(input->size.l, 
              input->size.s, sizeof(int16)); 
    if (wfinal_prob == NULL  || final_prob == NULL)
    {
        sprintf (errstr, "Allocating prob memory");
        ERROR (errstr, "pcloud");
    }

    printf("The fifth pass\n");

    /* Loop through each line in the image */
    for (row = 0; row < nrows; row++)
    {
       /* Print status on every 100 lines */
        if (!(row%1000)) 
        {
           printf ("Processing line %d\r",row);
           fflush (stdout);
        }

        /* For each of the image bands */
        for (ib = 0; ib < input->nband; ib++)
        {
            /* Read each input reflective band -- data is read into
               input->buf[ib] */
            if (!GetInputLine(input, ib, row))
            {
                sprintf (errstr, "Reading input image data for line %d, "
                    "band %d", row, ib);
                ERROR (errstr, "pcloud");
            }
        }

	/* For the thermal band */
	/* Read the input thermal band -- data is read into input->therm_buf */
	if (!GetInputThermLine(input, row))
        {
	  sprintf (errstr, "Reading input thermal data for line %d", row);
	  ERROR (errstr, "pcloud");
	}

        for (col = 0; col < ncols; col++)
        {
           if (input->therm_buf[col] == -9999)
           {
               new_nir[row][col] = backg_b4;
	       new_swir[row][col] = backg_b5;
           }
           else
           {
	       new_nir[row][col] = input->buf[3][col];
	       new_swir[row][col] = input->buf[4][col];
           }
        }
    }

    /* TODO: Fill in regional minimum band 4 ref*/
    IplImage* img = cvCreateImage(cvSize(ncols, nrows), IPL_DEPTH_8U, 1);
    if (!img)
    {
        sprintf (errstr, "Reading input image data for line %d, "
                 "band %d", row, ib);
        ERROR (errstr, "pcloud");
    }
    //     cvSet2D(img, ncols, nrows, new_nir);
    for (row = 0; row < nrows; row++)
    {
       for (col = 0; col < ncols; col++)
       {
           img->imageData[row*ncols+col] = new_nir[row][col];
           // cvSet2D(img, col, row, new_nir[row][col]);
       }
    }
    CvPoint seed_point = cvPoint(3,3);
    CvScalar color = CV_RGB(1,0,0);

    cvFloodFill(img, seed_point, color, cvScalarAll(5.0), cvScalarAll(5.0), 
                NULL, 4, NULL );

    // cvGet2D(img, ncols, nrows, new_nir);
    for (row = 0; row < nrows; row++)
    {
       for (col = 0; col < ncols; col++)
       {
           new_nir[row][col] = img->imageData[row*ncols+col];
           // new_nir[row][col] = cvGet2D(img, col, row);          
       }
    }

    /* Release image memory */
    cvReleaseImage(&img);

    /* TODO: Fill in regional minimum band 5 ref*/
    IplImage* img2 = cvCreateImage(cvSize(ncols, nrows), IPL_DEPTH_8U, 1);
    if (!img2)
    {
        sprintf (errstr, "Reading input image data for line %d, "
                 "band %d", row, ib);
        ERROR (errstr, "pcloud");
    }
    //     cvSet2D(img, ncols, nrows, new_swir);
    for (row = 0; row < nrows; row++)
    {
       for (col = 0; col < ncols; col++)
       {
           img2->imageData[row*ncols+col] = new_swir[row][col];
           // cvSet2D(img2, col, row, new_swir[row][col]);
       }
    }

    cvFloodFill(img2, seed_point, color, cvScalarAll(5.0), cvScalarAll(5.0), 
                NULL, 4, NULL );

    //     cvGet2D(img, ncols, nrows, new_swir);
    for (row = 0; row < nrows; row++)
    {
       for (col = 0; col < ncols; col++)
       {
           new_swir[row][col] = img2->imageData[row*ncols+col];
           // new_swir[row][col] = cvGet2D(img2, col, row);          
       }
    }

    /* Release image memory */
    cvReleaseImage(&img2);

    for (row = 0; row < nrows; row++)
    {
      for (col = 0; col < ncols; col++)
      {
        if (new_nir[row][col] < new_swir[row][col])
	  shadow_prob = new_nir[row][col];
        else
	  shadow_prob = new_swir[row][col];
      
        if (shadow_prob > 200)
	  shadow_mask[row][col] = 1;
        else
	  shadow_mask[row][col] = 0;
      }
    }
    status = ias_misc_free_2d_array((void **)new_nir);
    status = ias_misc_free_2d_array((void **)new_swir);
   }

    printf("The sixth pass\n");
    /* Loop through each line in the image */
    for (row = 0; row < nrows; row++)
    {
       /* Print status on every 100 lines */
        if (!(row%1000)) 
        {
           printf ("Processing line %d\r",row);
           fflush (stdout);
        }

        /* For each of the image bands */
        for (ib = 0; ib < input->nband; ib++)
        {
            /* Read each input reflective band -- data is read into
               input->buf[ib] */
            if (!GetInputLine(input, ib, row))
            {
                sprintf (errstr, "Reading input image data for line %d, "
                    "band %d", row, ib);
                ERROR (errstr, "pcloud");
            }
        }

	/* For the thermal band */
	/* Read the input thermal band -- data is read into input->therm_buf */
	if (!GetInputThermLine(input, row))
        {
	  sprintf (errstr, "Reading input thermal data for line %d", row);
	  ERROR (errstr, "pcloud");
	}
     for (col = 0; col < ncols; col++)
     {
        /* refine Water mask - Zhe's water mask (no confusion water/cloud) */
        if (water_mask[row][col] == 1 && cloud_mask[row][col] == 0)
          water_mask[row][col] = 1;
        else
	  water_mask[row][col] = 0;

        if (input->therm_buf[col]==-9999)
	{
	  cloud_mask[row][col] = 255;
	  shadow_mask[row][col] = 255;
          //          final_mask[row][col] = 255;
	}
#if 0
      /* Temporary outputs */      
      if (water_mask[row][col] == 1)
	final_mask[row][col] = 1;
      if (snow_mask[row][col] == 1)
        final_mask[row][col] = 3;
      if (shadow_mask[row][col] == 1)
	final_mask[row][col] = 2;
      if (cloud_mask[row][col] == 1)
	final_mask[row][col] = 4;

      if ((water_mask[row][col] != 1) && (snow_mask[row][col] != 1) &&
          (shadow_cal[row][col] != 1) && (cloud_cal[row][col] != 1) &&
          boundary_test[row][col] != 255)
         final_mask[row][col] = 0;
#endif
     }
   }

   printf("t_wtemp,t_templ,t_temph = %f,%f,%f\n",t_wtemp,*t_templ,*t_temph);

    return 0;
}
