/**
 * !Description
 *  This application appends the cfmask band from the cfmask cloud mask HDF
 *  file to the surface reflectance product.
 *
 * !Input  
 *  surface reflectance file (lndsr*) in HDF format (from "lndsr")
 *  cloud mask file in HDF format (from "cfmask")
 *
 * !Output
 *  updated lndsr* file in HDF format with cfmask band appended
 *
 * !Developer: 
 *  Gail Schmidt, USGS EROS LSRD project  6/11/2013
 *  Quick and dirty application to handle the append of the cfmask cloud
 *    mask to the lndsr surface reflectance product.  This will be replaced
 *    when the project migrates to raw binary as the internal file format.
 *    Used lndapp application from LEDAPS as the baseline for this application.
 *
 * !History 
 *  Gail Schmidt, USGS EROS LSRD project   7/8/2013
 *  Updated to read the CFmaskVersion from the cfmask file and append it to
 *    the lndsr global attributes.
 *  Modified FAILURE and SUCCESS to be standard values instead of using the
 *    HDF and HDF-EOS failure value.
 *
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "bool.h"
#include "hdf.h"
#include "mfhdf.h"
#include "HdfEosDef.h"

#define MAX_NCOLS   10000 
#define Max_StrLen  1000
#define HDF_FAILURE -1
#define FAILURE     1
#define SUCCESS     0

typedef struct {

  char  fileName[Max_StrLen];
  int32 nrows;           
  int32 ncols;
  float ulx;
  float uly;

  int32 SD_ID;            
  int32 band_id; 
  char    versionNum[Max_StrLen];

  char    sdsName[Max_StrLen];
  char    longName[Max_StrLen];
  char    units[Max_StrLen];
  int16   range[2];
  int16   fillValue;
  int16   satValue;
  float64 scale;

} GRID_SR;    

void usage(char *);
int  getMetaInfo(GRID_SR *sr, bool processSR);
int  openForWrite(GRID_SR *sr, GRID_SR *cfmask);
int  closeUp(GRID_SR *sr, bool close);

//#define DEBUG 1

int main(int argc, char *argv[])
{
  int i, irow, ret;
  int32 start[2];
  int32 length[2];
  uint8 *buf = NULL;
  GRID_SR sr, cfmask;

  if(argc != 5) {
    usage(argv[0]);
    exit(EXIT_FAILURE);
  }
  
  /* parse command line */
  for(i=1; i<argc; i++){
    if(strcmp(argv[i],"-sr")==0)
      strcpy(sr.fileName, argv[++i]);
    else
      if(strcmp(argv[i],"-cfmask")==0)
	strcpy(cfmask.fileName, argv[++i]);
      else{
	printf("\nWrong option:%s\n",argv[i]);
	usage(argv[0]);
	exit(EXIT_FAILURE);
      }    
  }

  strcpy(sr.sdsName, "band1");
  strcpy(cfmask.sdsName, "fmask_band");

  /* get metadata from lndsr* file */
  printf ("Reading meta info from SR file ...\n");
  if((ret=getMetaInfo(&sr, true))==FAILURE) {
    printf("Retrieve %s metadata error\n", sr.fileName);
	exit(EXIT_FAILURE);
  }
  printf ("\n");

  /* get metadata from cfmask* file */
  printf ("Reading meta info from fmask file ...\n");
  if((ret=getMetaInfo(&cfmask, false))==FAILURE) {
    printf("Retrieve %s metadata error\n", cfmask.fileName);
	exit(EXIT_FAILURE);
  }
  printf ("\n");
  
  if(sr.nrows!=cfmask.nrows || sr.ncols!=cfmask.ncols || sr.ulx!=cfmask.ulx ||
     sr.uly!=cfmask.uly) {
    printf("Warning: lndsr and cfmask images do not match!  The number of "
        "lines and samples, along with the UL x and y pixel locations need "
        "to be the same.\n");
	exit(EXIT_FAILURE);
  }

  /* allocate memory */
  buf = calloc(cfmask.ncols, sizeof(uint8));
  if(!buf) {
    printf("Memory allocation error\n");
	exit(EXIT_FAILURE);
  }
  
  /* create the cfmask band */
  printf("Updating lndsr file for creating/modifying cfmask SDS ...\n");
  if((ret=openForWrite(&sr, &cfmask))==FAILURE) {
    printf("Open for write error\n");
	exit(EXIT_FAILURE);
  }
  printf("Appending cfmask for line --- ");

  /* loop through each row of data */
  for(irow=0; irow<sr.nrows; irow++) {
    printf("%4d\b\b\b\b",irow);
   
    start[0] = irow;
    start[1] = 0;
    length[0] = 1;
    length[1] = cfmask.ncols;

    if((SDreaddata(cfmask.band_id, start, NULL, length, buf)) == FAILURE) {
      printf("Error: reading cfmask error irow: %d\n", irow);
	  exit(EXIT_FAILURE);
    }
    
    if((SDwritedata(sr.band_id, start, NULL, length, buf)) == FAILURE) {
      printf("Error: writing to combined file error irow: %d\n", irow);
	  exit(EXIT_FAILURE);
    }
  } /* end of irow */
  printf ("\n");

  /* close HDF files */
  closeUp(&sr, true);
  closeUp(&cfmask, true);

  /* Free buffers */
  free(buf);
  exit(EXIT_SUCCESS);      /* success */
}


/* display usage */
void usage(char *command)
{
  printf("Usage: %s -sr lndsr_HDF_file -cfmask cfmask_HDF_file\n", command);
  printf("  where ...\n");
  printf("  lndsr_HDF_file is the surface reflectance file\n");
  printf("  cfmask_HDF_file is the cfmask cloud mask file\n");
}


/* get metadata and open specific sds
   processSr specifies if we are reading the surface reflectance product,
     if not then we are reading the cfmask product
*/
int getMetaInfo(GRID_SR *sr, bool processSR) {

  int index, ret;

  char GD_gridlist[100];
  int32 gfid=0, ngrid=0, gid=0;
  int32 bufsize=100;
  float64 GD_upleft[2];
  float64 GD_lowright[2];
  int32 att_id;

  /* open a hdf file */
  gfid = GDopen(sr->fileName, DFACC_READ);
  if(gfid==HDF_FAILURE){
      printf ("Not successful in retrieving grid file ID/open\n");
      return FAILURE;
  }

  /* find out about grid type */
  ngrid=GDinqgrid(sr->fileName, GD_gridlist,&bufsize);
  if(ngrid==HDF_FAILURE){
      printf ("Not successful in retrieving grid name list\n");
      return FAILURE;
  }

  /* attach grid */
  gid = GDattach(gfid, GD_gridlist);
  if(gid==HDF_FAILURE){
      printf ("Not successful in attaching grid.\n");
      return FAILURE;
  }

  /* get grid info */
  ret = GDgridinfo(gid, &sr->ncols, &sr->nrows, GD_upleft, GD_lowright);
  if(ret==HDF_FAILURE){
      printf ("Failed to read grid info.\n");
      return FAILURE;
  }
  sr->ulx = GD_upleft[0];
  sr->uly = GD_upleft[1];

  /* detach grid */
  ret = GDdetach(gid);
  if(ret==HDF_FAILURE){
      printf ("Failed to detach grid.\n");
      return FAILURE;
  }

  /* close for grid access */
  ret = GDclose(gfid);
  if(ret==HDF_FAILURE){
      printf ("GD-file close failed.\n");
      return FAILURE;
  }

  /* open hdf file */ 
  if ((sr->SD_ID = SDstart(sr->fileName, DFACC_READ))<0) {
    printf("Can't open file %s\n",sr->fileName);
    return FAILURE;
  }

  /* look for the CFMaskVersion in the main attributes of fmask product */
  if (!processSR) {
    if ((att_id = SDfindattr(sr->SD_ID, "CFmaskVersion")) == HDF_FAILURE) {
      printf ("Can't retrieve CFmaskVersion from HDF file.\n");
      return FAILURE;
    }
    else if (SDreadattr(sr->SD_ID, att_id, sr->versionNum) == HDF_FAILURE) {
      printf ("Can't retrieve CFmaskVersion from HDF file.\n");
      return FAILURE;
    }
  }

  /* open band1 and get sds_id from given sds_name */
  if ((index=SDnametoindex(sr->SD_ID, sr->sdsName))<0) {
    printf("Not successful in convert sdsName %s to index\n", sr->sdsName);
    return FAILURE;
  }
  sr->band_id = SDselect(sr->SD_ID, index);

  /* retrieve SR Fill Value and other attributes */
  if ((att_id = SDfindattr(sr->band_id, "_FillValue")) == HDF_FAILURE) {
    printf ("Can't retrieve fill value from SDS %s.\n", sr->sdsName);
    return FAILURE;
  }
  else if (SDreadattr(sr->band_id, att_id, &sr->fillValue) == HDF_FAILURE) {
    printf ("Can't retrieve fill value from SDS %s.\n", sr->sdsName);
    return FAILURE;
  }

  if (processSR) {
    if ((att_id = SDfindattr(sr->band_id, "_SaturateValue")) == HDF_FAILURE) {
      printf ("Can't retrieve saturation value from SDS %s.\n", sr->sdsName);
      return FAILURE;
    }
    else if (SDreadattr(sr->band_id, att_id, &sr->satValue) == HDF_FAILURE) {
      printf ("Can't retrieve saturation value from SDS %s.\n", sr->sdsName);
      return FAILURE;
    }
  }

  if ((att_id = SDfindattr(sr->band_id, "long_name")) == HDF_FAILURE) {
    printf ("Can't retrieve long name from SDS %s.\n", sr->sdsName);
    return FAILURE;
  }
  else if (SDreadattr(sr->band_id, att_id, sr->longName) == HDF_FAILURE) {
    printf ("Can't retrieve long name from SDS %s.\n", sr->sdsName);
    return FAILURE;
  }

  if (processSR) {
    if ((att_id = SDfindattr(sr->band_id, "units")) == HDF_FAILURE) {
      printf ("Can't retrieve units from SDS %s.\n", sr->sdsName);
      return FAILURE;
    }
    else if (SDreadattr(sr->band_id, att_id, sr->units) == HDF_FAILURE) {
      printf ("Can't retrieve units from SDS %s.\n", sr->sdsName);
      return FAILURE;
    }
  }

  if ((att_id = SDfindattr(sr->band_id, "valid_range")) == HDF_FAILURE) {
    printf ("Can't retrieve valid range from SDS %s.\n", sr->sdsName);
    return FAILURE;
  }
  else if (SDreadattr(sr->band_id, att_id, sr->range) == HDF_FAILURE) {
    printf ("Can't retrieve valid range from SDS %s.\n", sr->sdsName);
    return FAILURE;
  }

  if (processSR) {
    if ((att_id = SDfindattr(sr->band_id, "scale_factor")) == HDF_FAILURE) {
      printf ("Can't retrieve scale factor from SDS %s.\n", sr->sdsName);
      return FAILURE;
    }
    else if (SDreadattr(sr->band_id, att_id, &sr->scale) == HDF_FAILURE) {
      printf ("Can't retrieve scale factor from SDS %s.\n", sr->sdsName);
      return FAILURE;
    }
  }

#ifdef DEBUG
  printf("File: %s\n",sr->fileName);
  printf("nrows:%d, ncols:%d\t",sr->nrows, sr->ncols);
  printf("ulx:%8.1f, uly:%8.1f\t", sr->ulx, sr->uly);
  printf("fillV:%d\n", sr->fillValue); 
  printf("satV:%d\n", sr->satValue); 
#endif

  closeUp(sr, true);
  return SUCCESS;
}


/* create output HDF file and open sds_id for write */
int openForWrite(GRID_SR *sr, GRID_SR *cfmask)
{
  int ret, index, exist;
  int32 GDfid, GDid; 
  char lndsr_index[1000];
  
  /* open hdf file and check if cfmask band has already been appended */  
  if ((sr->SD_ID = SDstart(sr->fileName, DFACC_READ))<0) {
    printf("Can't open file %s",sr->fileName);
    return FAILURE;
  }
  
  /* check if band already exists */
  if ((index=SDnametoindex(sr->SD_ID, cfmask->sdsName))<0) {
    exist = 0;
  }
  else {
    exist = 1;
    printf("Information: %s already exists in the surface reflectance file."
        "  The SDS won't be recreated but will be overwritten.\n",
        cfmask->sdsName);
  }

  if((SDend(sr->SD_ID)) == HDF_FAILURE) {
    printf("SDend for %s error!\n", sr->fileName);
    return FAILURE;
  }
  sr->SD_ID = 9999;

  /* create output use HDF-EOS functions */
  GDfid = GDopen(sr->fileName, DFACC_RDWR);
  if(GDfid == HDF_FAILURE){
    printf("Not successful in creating grid file %s", sr->fileName);
    return FAILURE;
  }
  
  GDid = GDattach(GDfid, "Grid");
  if(GDid == HDF_FAILURE) {
    printf("Not successful in attaching grid ID/ create for %s", sr->fileName);
    return FAILURE;
  }

  /* Create the new SDS in the Grid, if it doesn't already exist */
  if(exist == 0) {
    ret = GDdeffield(GDid, cfmask->sdsName, "YDim,XDim", DFNT_UINT8,
      HDFE_NOMERGE);
    if(ret==HDF_FAILURE){
      printf ("Not successful in defining %s SDS", cfmask->sdsName);
      return FAILURE;
    }
  }
  
  /* detach grid */
  ret = GDdetach(GDid);
  if(ret==HDF_FAILURE){
    printf ("Failed to detach grid.");
    return FAILURE;
  }

  /* close for grid access */
  ret = GDclose(GDfid);
  if(ret==HDF_FAILURE){
    printf ("GD-file close failed.");
    return FAILURE;
  }
 
  /* open hdf file */
  if ((sr->SD_ID = SDstart(sr->fileName, DFACC_RDWR))<0) {
    printf("Can't open file %s",sr->fileName);
    return FAILURE;
  }
  
  /* write the fmask version */
  ret = SDsetattr(sr->SD_ID, "CFmaskVersion", DFNT_CHAR8,
    strlen(cfmask->versionNum), cfmask->versionNum);
  if (ret == HDF_FAILURE) {
    printf ("Can't write CFmaskVersion to global attributes.\n");
    return FAILURE;
  }

  /* get sds_id from given sds_name and then write metedata */
  if ((index = SDnametoindex(sr->SD_ID, cfmask->sdsName))<0) {
    printf("Unable to find the newly created SDS (%s) in the surface "
      "reflectance file", cfmask->sdsName);
    return FAILURE;
  }
  sr->band_id = SDselect(sr->SD_ID, index);

  /* write SDS metadata */
  ret = SDsetattr(sr->band_id, "long_name", DFNT_CHAR8,
    strlen(cfmask->longName), cfmask->longName);
  if (ret == HDF_FAILURE) {
    printf("Can't write SR long_name for SDS %s\n", cfmask->sdsName);
    return FAILURE;
  } 
    
  ret = SDsetattr(sr->band_id, "valid_range", DFNT_INT16, 2, cfmask->range);
  if (ret == HDF_FAILURE) {
    printf ("Can't write SR valid_range for SDS %s", cfmask->sdsName);
    return FAILURE;
  } 
  
  ret = SDsetattr(sr->band_id, "_FillValue", DFNT_INT16, 1,
    &(cfmask->fillValue));
  if (ret == HDF_FAILURE) {
    printf ("Can't write SR _FillValue for SDS %s", cfmask->sdsName);
    return FAILURE;
  } 
  
  sprintf (lndsr_index, "0 clear; 1 water; 2 cloud_shadow; 3 snow; 4 cloud");
  ret = SDsetattr(sr->band_id, "mask_index", DFNT_CHAR8, strlen(lndsr_index),
    lndsr_index);
  if (ret == HDF_FAILURE) {
    printf ("Can't write SR mask_index for SDS %s", sr->sdsName);
    return FAILURE;
  } 

  /* open cfmask hdf file and get sds_id from given sds_name */  
  if ((cfmask->SD_ID = SDstart(cfmask->fileName, DFACC_READ))<0) {
    printf("Can't open file %s\n",cfmask->fileName);
    return FAILURE;
  }

  if ((index=SDnametoindex(cfmask->SD_ID, cfmask->sdsName))<0) {
    printf("Unable to access sdsName %s and convert to index\n",
      cfmask->sdsName);
    return FAILURE;
  }
  cfmask->band_id = SDselect(cfmask->SD_ID, index);

  return SUCCESS;
}


int closeUp(GRID_SR *sr, bool close) 
{
  if((SDendaccess(sr->band_id)) == HDF_FAILURE) {
    printf("SDendaccess for %s cfmask band error!\n", sr->fileName);
    return FAILURE;
  }

  if (close) {
    if((SDend(sr->SD_ID)) == HDF_FAILURE) {
      printf("SDend for %s error!\n", sr->fileName);
      return FAILURE;
    }
    sr->SD_ID = 9999;
  }

  return SUCCESS;
}
