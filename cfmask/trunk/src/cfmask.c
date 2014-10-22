
#include <string.h>
#include <time.h>

#include "espa_metadata.h"
#include "parse_metadata.h"
#include "write_metadata.h"
#include "envi_header.h"
#include "espa_geoloc.h"
#include "raw_binary_io.h"

#include "const.h"
#include "error.h"
#include "input.h"
#include "output.h"
#include "2d_array.h"
#include "cfmask.h"

/******************************************************************************
METHOD:  cfmask

PURPOSE:  the main routine for fmask in C

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           An error occurred during processing of the cfmask
SUCCESS         Processing was successful

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
3/15/2013   Song Guo         Original Development
5/14/2013   Song Guo         Added in Polar Stereographic support
7/17/2013   Song Guo         Added in Map info 
8/15/2013   Song Guo         Modified to use TOA reflectance file 
                             as input instead of metadata file
2/19/2014   Gail Schmidt     Modified to utilize the ESPA internal raw binary
                             file format

NOTES: type ./cfmask --help for information to run the code
******************************************************************************/
int
main (int argc, char *argv[])
{
    char errstr[MAX_STR_LEN];     /* error string */
    char *cptr = NULL;            /* pointer to the file extension */
    char *xml_name = NULL;        /* input XML filename */
    char directory[MAX_STR_LEN];  /* input/output data directory */
    char extension[MAX_STR_LEN];  /* input TOA file extension */
    int ib;                       /* band counters */
    Input_t *input = NULL;        /* input data and meta data */
    char envi_file[MAX_STR_LEN];  /* output ENVI file name */
    char scene_name[MAX_STR_LEN]; /* input data scene name */
    unsigned char **pixel_mask;   /* pixel mask */
    int status;               /* return value from function call */
    float ptm;                /* percent of clear-sky pixels */
    float t_templ = 0.0;      /* percentile of low background temperature */
    float t_temph = 0.0;      /* percentile of high background temperature */
    Output_t *output = NULL;  /* output structure and metadata */
    bool verbose;             /* verbose flag for printing messages */
    int cldpix = 2;           /* Default buffer for cloud pixel dilate */
    int sdpix = 2;            /* Default buffer for shadow pixel dilate */
    float cloud_prob;         /* Default cloud probability */
    float sun_azi_temp = 0.0; /* Keep the original sun azimuth angle */
    int max_cloud_pixels; /* Maximum cloud pixel number in cloud division */
    Espa_internal_meta_t xml_metadata; /* XML metadata structure */
    Envi_header_t envi_hdr;            /* output ENVI header information */

    time_t now;
    time (&now);
    printf ("CFmask start_time=%s\n", ctime (&now));

    /* Read the command-line arguments, including the name of the input
       Landsat TOA reflectance product and the DEM */
    status = get_args (argc, argv, &xml_name, &cloud_prob, &cldpix,
                       &sdpix, &max_cloud_pixels, &verbose);
    if (status != SUCCESS)
    {
        sprintf (errstr, "calling get_args");
        CFMASK_ERROR (errstr, "main");
    }

    /* Validate the input metadata file */
    if (validate_xml_file (xml_name) != SUCCESS)
    {                           /* Error messages already written */
        CFMASK_ERROR (errstr, "main");
    }

    /* Initialize the metadata structure */
    init_metadata_struct (&xml_metadata);

    /* Parse the metadata file into our internal metadata structure; also
       allocates space as needed for various pointers in the global and band
       metadata */
    if (parse_metadata (xml_name, &xml_metadata) != SUCCESS)
    {                           /* Error messages already written */
        CFMASK_ERROR (errstr, "main");
    }

    /* Split the filename to obtain the directory, scene name, and extension */
    split_filename (xml_name, directory, scene_name, extension);
    if (verbose)
        printf ("directory, scene_name, extension=%s,%s,%s\n",
                directory, scene_name, extension);

    /* Open input file, read metadata, and set up buffers */
    input = OpenInput (&xml_metadata);
    if (input == NULL)
    {
        sprintf (errstr, "opening the TOA and brightness temp files in: %s",
                 xml_name);
        CFMASK_ERROR (errstr, "main");
    }

    if (verbose)
    {
        /* Print some info to show how the input metadata works */
        printf ("DEBUG: Number of input TOA bands: %d\n", input->nband);
        printf ("DEBUG: Number of input thermal bands: %d\n", 1);
        printf ("DEBUG: Number of input TOA lines: %d\n", input->size.l);
        printf ("DEBUG: Number of input TOA samples: %d\n", input->size.s);
        printf ("DEBUG: ACQUISITION_DATE.DOY is %d\n",
                input->meta.acq_date.doy);
        printf ("DEBUG: Fill value is %d\n", input->meta.fill);
        for (ib = 0; ib < input->nband; ib++)
        {
            printf ("DEBUG: Band %d-->\n", ib);
            printf ("DEBUG:   band satu_value_ref: %d\n",
                    input->meta.satu_value_ref[ib]);
            printf ("DEBUG:   band satu_value_max: %d\n",
                    input->meta.satu_value_max[ib]);
            printf ("DEBUG:   band gains: %f, band biases: %f\n",
                    input->meta.gain[ib], input->meta.bias[ib]);
        }
        printf ("DEBUG: Thermal Band -->\n");
        printf ("DEBUG:   therm_satu_value_ref: %d\n",
                input->meta.therm_satu_value_ref);
        printf ("DEBUG:   therm_satu_value_max: %d\n",
                input->meta.therm_satu_value_max);
        printf ("DEBUG:   therm_gain: %f, therm_bias: %f\n",
                input->meta.gain_th, input->meta.bias_th);

        printf ("DEBUG: SUN AZIMUTH is %f\n", input->meta.sun_az);
        printf ("DEBUG: SUN ZENITH is %f\n", input->meta.sun_zen);
    }

    /* If the scene is an ascending polar scene (flipped upside down), then
       the solar azimuth needs to be adjusted by 180 degrees.  The scene in
       this case would be north down and the solar azimuth is based on north
       being up clock-wise direction. Flip the south to be up will not change
       the actual sun location, with the below relations, the solar azimuth
       angle will need add in 180.0 for correct sun location */
    if (input->meta.ul_corner.is_fill &&
        input->meta.lr_corner.is_fill &&
        (input->meta.ul_corner.lat - input->meta.lr_corner.lat) < MINSIGMA)
    {
        /* Keep the original solar azimuth angle */
        sun_azi_temp = input->meta.sun_az;
        input->meta.sun_az += 180.0;
        if ((input->meta.sun_az - 360.0) > MINSIGMA)
            input->meta.sun_az -= 360.0;
        if (verbose)
            printf ("  Polar or ascending scene."
                    "  Readjusting solar azimuth by 180 degrees.\n"
                    "  New value: %f degrees\n", input->meta.sun_az);
    }

    /* Dynamic allocate the 2d mask memory */
    pixel_mask = (unsigned char **) allocate_2d_array (input->size.l,
                                                       input->size.s,
                                                       sizeof (unsigned
                                                               char));
    if (pixel_mask == NULL)
    {
        sprintf (errstr, "Allocating mask memory");
        CFMASK_ERROR (errstr, "main");
    }

    /* Build the potential cloud, shadow, snow, water mask */
    status = potential_cloud_shadow_snow_mask (input, cloud_prob, &ptm,
                                               &t_templ, &t_temph, pixel_mask,
                                               verbose);
    if (status != SUCCESS)
    {
        sprintf (errstr, "processing potential_cloud_shadow_snow_mask");
        CFMASK_ERROR (errstr, "main");
    }

    printf ("Pcloud done, starting cloud/shadow match\n");

    /* Build the final cloud shadow based on geometry matching and
       combine the final cloud, shadow, snow, water masks into fmask
       the pixel_mask is a bit mask as input and a value mask as output */
    status = object_cloud_shadow_match (input, ptm, t_templ, t_temph,
                                        cldpix, sdpix, max_cloud_pixels,
                                        pixel_mask, verbose);
    if (status != SUCCESS)
    {
        sprintf (errstr, "processing object_cloud_and_shadow_match");
        CFMASK_ERROR (errstr, "main");
    }

    /* Reassign solar azimuth angle for output purpose if south up north
       down scene is involved */
    if (input->meta.ul_corner.is_fill &&
        input->meta.lr_corner.is_fill &&
        (input->meta.ul_corner.lat - input->meta.lr_corner.lat) < MINSIGMA)
        input->meta.sun_az = sun_azi_temp;

    /* Open the output file */
    output = OpenOutput (&xml_metadata, input);
    if (output == NULL)
    {                           /* error message already printed */
        sprintf (errstr, "Opening output file");
        CFMASK_ERROR (errstr, "main");
    }

    if (!PutOutput (output, pixel_mask))
    {
        sprintf (errstr, "Writing output fmask in HDF files\n");
        CFMASK_ERROR (errstr, "main");
    }

    /* Close the output file */
    if (!CloseOutput (output))
    {
        sprintf (errstr, "closing output file");
        CFMASK_ERROR (errstr, "main");
    }

    /* Create the ENVI header file this band */
    if (create_envi_struct (&output->metadata.band[0], &xml_metadata.global,
                            &envi_hdr) != SUCCESS)
    {
        sprintf (errstr, "Creating ENVI header structure.");
        CFMASK_ERROR (errstr, "main");
    }

    /* Write the ENVI header */
    strcpy (envi_file, output->metadata.band[0].file_name);
    cptr = strchr (envi_file, '.');
    if (cptr == NULL)
    {
        sprintf (errstr, "error in ENVI header filename");
        CFMASK_ERROR (errstr, "main");
    }

    strcpy (cptr, ".hdr");
    if (write_envi_hdr (envi_file, &envi_hdr) != SUCCESS)
    {
        sprintf (errstr, "Writing ENVI header file.");
        CFMASK_ERROR (errstr, "main");
    }

    /* Append the cfmask band to the XML file */
    if (append_metadata (output->nband, output->metadata.band, xml_name)
        != SUCCESS)
    {
        sprintf (errstr, "Appending spectral index bands to XML file.");
        CFMASK_ERROR (errstr, "main");
    }

    /* Free the structure */
    if (!FreeOutput (output))
    {
        sprintf (errstr, "freeing output file structure");
        CFMASK_ERROR (errstr, "main");
    }

    /* Free the metadata structure */
    free_metadata (&xml_metadata);

    /* Free the pixel mask */
    status = free_2d_array ((void **) pixel_mask);
    if (status != SUCCESS)
    {
        sprintf (errstr, "Freeing mask memory");
        CFMASK_ERROR (errstr, "main");
    }

    /* Close the input file and free the structure */
    CloseInput (input);
    FreeInput (input);

    free (xml_name);

    printf ("Processing complete.\n");
    time (&now);
    printf ("CFmask end_time=%s\n", ctime (&now));

    return SUCCESS;
}

/******************************************************************************
MODULE:  usage

PURPOSE:  Prints the usage information for this application.

RETURN VALUE:
Type = None

HISTORY:
Date        Programmer       Reason
--------    ---------------  -------------------------------------
3/15/2013   Song Guo         Original Development
8/15/2013   Song Guo         Modified to use TOA reflectance file 
                             as input instead of metadata file
2/19/2014   Gail Schmidt     Modified to utilize the ESPA internal raw binary
                             file format

NOTES: 
******************************************************************************/
void
usage ()
{
    printf ("Fmask identify the cloud, shadow, snow, water and clear pixels"
            " using the input Landsat scene (top of atmosphere (TOA)"
            " reflection and brightness temperature (BT) for band 6) output"
            " from LEDAPS\n");

    printf ("\nusage: ./cfmask"
            " --xml=input_xml_filename"
            " --prob=input_cloud_probability_value"
            " --cldpix=input_cloud_pixel_buffer"
            " --sdpix=input_shadow_pixel_buffer"
            " --max_cloud_pixels=maximum_cloud_pixel_numbers_for_cloud_division"
            " [--verbose]\n");

    printf ("\nwhere the following parameters are required:\n");
    printf ("    -xml: name of the input XML file which contains the TOA"
            " reflectance and brightness temperature files output from"
            " LEDAPS\n");

    printf ("\nwhere the following parameters are optional:\n");
    printf ("    -prob: cloud_probability, default value is 22.5\n");
    printf ("    -cldpix: cloud_pixel_buffer for image dilate,"
            " (default value is 3)\n");
    printf ("    -sdpix: shadow_pixel_buffer for image dilate,"
            " (default value is 3)\n");
    printf ("    -max_cloud_pixels: maximum_cloud_pixel_number for cloud"
            " division, (default value is 0)\n");
    printf ("    -verbose: should intermediate messages be printed?"
            " (default is false)\n");

    printf ("\n./cfmask --help will print the usage statement\n");

    printf ("\nExample: ./cfmask --xml=LE70390032010263EDC00.xml"
            " --prob=22.5 --cldpix=3 --sdpix=3"
            " --max_cloud_pixels=5000000 --verbose\n");
}
