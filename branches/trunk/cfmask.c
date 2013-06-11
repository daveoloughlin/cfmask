#include <string.h>
#include <stdarg.h>
#include "input.h"
#include "cfmask.h"
#include "ias_logging.h"
#include "ias_misc_2d_array.h"

/**************************************************************************
Description: This application is a sample test program to read in the user
specified HDF file and write it back out to temp.hdf.

Usage: HDF_sample_code input_lnd_meatadata_file

Notes:
1. This code reads the reflective image data, thermal image data, and the QA
bands.  It skips the atmospheric opacity.
2. The code simply copies the reflective image data and QA data to the output
structure, and duplicates those bands in the output product.  In most cases,
the output bands and QA information would be calculated by the application
using these libraries.
**************************************************************************/

int main (int argc, const char *argv[])
{
    char errstr[MAX_STR_LEN];           /* error string */
    char lndcal_name[MAX_STR_LEN];
    char lndsr_name[MAX_STR_LEN];
    char fmask_name[MAX_STR_LEN];
    char *lndmeta_name;                 /* input lndmeta data filename */
    char directory[MAX_STR_LEN];
    char extension[MAX_STR_LEN];
    int ib;                         /* band counters */
    char sds_names[NBAND_REFL_MAX][MAX_STR_LEN]; /* array of image SDS names */
    Input_t *input = NULL;              /* input data and metadata */
    //    Param_t *param;
    char  scene_name[MAX_STR_LEN];
    char description[MAX_STR_LEN];
    unsigned char **cloud_mask;
    unsigned char **shadow_mask;
    unsigned char **snow_mask;
    unsigned char **water_mask;
    unsigned char **final_mask;
    int status;
    int cldpix = 3;
    int sdpix = 3;
    float cloud_prob = 22.5;
    FILE *fd;
    float ptm;
    float t_templ;
    float t_temph;
  
    /* Check the arguments */
    if (argc != 2 || strstr (argv[1], "-help"))
    {
        sprintf (errstr, "Usage: HDF_sample_code input_lnd_metadata_file");
        ERROR (errstr, "main");
    }

    /* Get the input file names */
    lndmeta_name = strdup (argv[1]);
    if (lndmeta_name == NULL)
    {
        sprintf (errstr, "Input filename was not provided or memory "
            "allocation was not successful.");
        ERROR (errstr, "main");
    }
    //    tokenptr = strtok(lndmeta_name, ".");
    ias_misc_split_filename(lndmeta_name, directory, scene_name, extension);
    printf("=%s,%s,%s\n", directory, scene_name, extension);
    //    sprintf(scene_name, "%s", tokenptr);  
    sprintf(lndcal_name, "%slndcal.%s.hdf", directory, scene_name);
    sprintf(lndsr_name, "%slndsr.%s.hdf", directory, scene_name);
    sprintf(fmask_name, "%sfmask.%s.img", directory, scene_name);

    lndmeta_name = strdup (argv[1]);
    printf("lndmeta_name=%s\n",lndmeta_name);
    printf("lndcal_name, lndsr_name = %s, %s\n", lndcal_name, lndsr_name); 
 
    /* Open input file, read metadata, and set up buffers */
    input = OpenInput(lndsr_name, lndcal_name, lndmeta_name);
    if (input == (Input_t *)NULL)
    {
        sprintf (errstr, "opening the input files: %s & %s", lndsr_name,
                 lndcal_name);
        ERROR (errstr, "main");
    }

    /* Print some info to show how the input metadata works */
    printf ("DEBUG: Number of input TOA bands: %d\n", input->nband);
    printf ("DEBUG: Number of input thermal bands: %d\n", 1);
    printf ("DEBUG: Number of input lines: %d\n", input->size.l);
    printf ("DEBUG: Number of input samples: %d\n", input->size.s);
    printf ("DEBUG: Number of input TOA lines: %d\n", input->toa_size.l);
    printf ("DEBUG: Number of input TOA samples: %d\n", input->toa_size.s);
    printf ("DEBUG: Provider is %s\n", input->meta.provider);
    printf ("DEBUG: Satellite is %s\n", input->meta.sat);
    printf ("DEBUG: Instrument is %s\n", input->meta.inst);
    printf ("DEBUG: WRS system is %s\n", input->meta.wrs_sys);
    printf ("DEBUG: Path is %d\n", input->meta.path);
    printf ("DEBUG: Row is %d\n", input->meta.row);
    printf ("DEBUG: Fill value is %d\n", input->meta.fill);
    for (ib = 0; ib < input->nband; ib++)
    {
        printf ("DEBUG: Band %d-->\n", ib);
        printf ("DEBUG:   SDS name is %s\n", input->sds[ib].name);
        printf ("DEBUG:   SDS rank: %d\n", input->sds[ib].rank);
    }
    printf ("DEBUG: Thermal Band -->\n");
    printf ("DEBUG:   SDS name is %s\n", input->therm_sds.name);
    printf ("DEBUG:   SDS rank: %d\n", input->therm_sds.rank);

    printf("DEBUG: ROW is %d\n", input->meta.row);
    printf("DEBUG: PATH is %d\n", input->meta.path);
    printf("DEBUG: SUN AZIMUTH is %f\n", input->meta.sun_az);
    printf("DEBUG: SUN ZENITH is %f\n", input->meta.sun_zen);
    printf("DEBUG: UL latitude is %f\n", input->meta.ul_lat);
    printf("DEBUG: UL longitude is %f\n", input->meta.ul_lon);
    printf("DEBUG: Projection Zone is %d\n", input->meta.zone);

    printf("DEBUG: unit_ref is %s\n", input->meta.unit_ref);
    printf("DEBUG: valid_range_ref is %f & %f\n", input->meta.valid_range_ref[0], 
           input->meta.valid_range_ref[1]);
    printf("DEBUG: satu_value_ref is %f\n", input->meta.satu_value_ref);
    printf("DEBUG: scale_factor_ref is %f\n", input->meta.scale_factor_ref);
    printf("DEBUG: add_offset_ref is %f\n", input->meta.add_offset_ref);
    printf("DEBUG: add_offset_err_ref is %f\n", input->meta.add_offset_err_ref);
    printf("DEBUG: calibrated_nt_ref is %f\n", input->meta.calibrated_nt_ref);

    printf("DEBUG: therm_unit_ref is %s\n", input->meta.therm_unit_ref);
    printf("DEBUG: therm_valid_range_ref is %f & %f\n", 
           input->meta.therm_valid_range_ref[0], 
           input->meta.therm_valid_range_ref[1]);
    printf("DEBUG: therm_satu_value_ref is %f\n", 
           input->meta.therm_satu_value_ref);
    printf("DEBUG: therm_scale_factor_ref is %f\n", 
           input->meta.therm_scale_factor_ref);
    printf("DEBUG: therm_add_offset_ref is %f\n", 
           input->meta.therm_add_offset_ref);
    printf("DEBUG: therm_add_offset_err_ref is %f\n", 
           input->meta.therm_add_offset_err_ref);
    printf("DEBUG: therm_calibrated_nt_ref is %f\n", 
           input->meta.therm_calibrated_nt_ref);

    printf("DEBUG: UL projection X is %f\n", input->meta.ul_projection_x);
    printf("DEBUG: UL projection Y is %f\n", input->meta.ul_projection_y);
    /* Copy the SDS names and QA SDS names from the input structure for the
       output structure, since we are simply duplicating the input */
    for (ib = 0; ib < input->nband; ib++)
        strcpy (&sds_names[ib][0], input->sds[ib].name);

    /* Dynamic allocate the 2d mask memory */
    cloud_mask = (unsigned char **)ias_misc_allocate_2d_array(input->size.l, 
                 input->size.s, sizeof(unsigned char)); 
    shadow_mask = (unsigned char **)ias_misc_allocate_2d_array(input->size.l, 
                 input->size.s, sizeof(unsigned char)); 
    snow_mask = (unsigned char **)ias_misc_allocate_2d_array(input->size.l, 
                 input->size.s, sizeof(unsigned char)); 
    water_mask = (unsigned char **)ias_misc_allocate_2d_array(input->size.l, 
                 input->size.s, sizeof(unsigned char)); 
    final_mask = (unsigned char **)ias_misc_allocate_2d_array(input->size.l, 
                 input->size.s, sizeof(unsigned char)); 
    if (cloud_mask == NULL  || shadow_mask == NULL || snow_mask == NULL
        || water_mask == NULL || final_mask == NULL)
    {
        sprintf (errstr, "Allocating mask memory");
        ERROR (errstr, "main");
    }

    /* Do plcloud  */
    status = potential_cloud_shadow_snow_mask(input, cloud_prob, &ptm,
             &t_templ, &t_temph, cloud_mask, shadow_mask, snow_mask, 
                                              water_mask, final_mask);
    if (status != 0)
    {
        sprintf (errstr, "calling potential_cloud_shadow_snow_mask");
        ERROR (errstr, "main");
    }

    printf("Pcloud done, starting cloud/shadow match\n");

    /* Call fcssm routine and do all masks together */
    status = object_cloud_shadow_match(input, ptm, t_templ, t_temph,
             cldpix, sdpix, cloud_mask, shadow_mask, snow_mask, water_mask,
             final_mask);
    if (status != 0)
    {
        sprintf (errstr, "calling object_cloud_and_shadow_match");
        ERROR (errstr, "main");
    }    

    int cloud = 0;
    int shadow = 0;
    int water = 0;
    int snow = 0;
    int fill = 0;
    int row,col;

    for (row = 0; row < input->size.l; row++)
     {
      for (col = 0; col < input->size.s; col++)
       {
        if (final_mask[row][col] == 1)
           water++;
        if (final_mask[row][col] == 2)
           shadow++;
        if (final_mask[row][col] == 3)
           snow++;
        if (final_mask[row][col] == 4)
           cloud++;
        if (final_mask[row][col] == 255)
           fill++;
       }
     }
    printf("water, shadow, snow, cloud, fill, total_pixels=%d,%d, %d, %d, %d "
           "%d\n", water, shadow, snow, cloud, fill, 
           input->size.l*input->size.s);

    /* Close the input file and free the structure */
    CloseInput (input);
    FreeInput (input);

    status = ias_misc_free_2d_array((void **)shadow_mask);
    status = ias_misc_free_2d_array((void **)snow_mask);
    status = ias_misc_free_2d_array((void **)water_mask);
    status = ias_misc_free_2d_array((void **)cloud_mask);
    if (status != 0)
    {
        sprintf (errstr, "Freeing mask memory");
        ERROR (errstr, "main");
    }

    IAS_PROJECTION proj_info;
    proj_info.proj_code = 1;
    proj_info.zone = input->meta.zone;
    status = snprintf(description, sizeof(description), "Fmask for cloud, "
            "cloud shadow, snow, and water");
    if (status < 0 || status >= sizeof(description))
    {
        IAS_LOG_ERROR("Buffer for envi header description not large enough");
        exit(EXIT_FAILURE);
    }

    /* Create an ENVI header file for the image */
    status = ias_misc_write_envi_header(fmask_name, &proj_info, 
             description, input->size.l, input->size.s, 1, 
             input->meta.ul_projection_x, input->meta.ul_projection_y, 
             30, 30, IAS_BYTE);
    if (status != 0)
    {
        IAS_LOG_ERROR("Creating ENVI header for image");
        exit(EXIT_FAILURE);
    }

    /* Open the mask file for writing */
    fd = fopen(fmask_name, "w"); 
    if (fd == NULL)
    {
        IAS_LOG_ERROR("Opening report file: %s", fmask_name);
        exit(EXIT_FAILURE);
    }

    /* Write out the mask file */
    status = fwrite(final_mask, sizeof(unsigned char), input->size.l * 
                 input->size.s, fd);
    if (status != input->size.l * input->size.s)
    {
        IAS_LOG_ERROR("Writing to %s", fmask_name);
        exit(EXIT_FAILURE);
    }

    /* Close the mask file */
    status = fclose(fd);
    if ( status )
    {
        IAS_LOG_ERROR("Closing file %s", fmask_name);
        exit(EXIT_FAILURE);
    }

    status = ias_misc_free_2d_array((void **)final_mask);
    if (status != 0)
    {
        sprintf (errstr, "Freeing cloud mask memory");
        ERROR (errstr, "main");
    }


    printf ("Processing complete.\n");
    return (SUCCESS);
}
