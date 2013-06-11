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
 *  Gail Schmidt, USGS EROS LSRD project
 *
 * !History 
 *  Used lndapp application from LEDAPS as the baseline for this application.
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
#define FAILURE     -1
#define SUCCESS     1

typedef struct {

  char  fileName[Max_StrLen];
  int32 nrows;           
  int32 ncols;
  float ulx;
  float uly;

  int32 SD_ID;            
  int32 band_id; 

  char    sdsName[Max_StrLen];
  char    longName[Max_StrLen];
  char    units[Max_StrLen];
  int16   range[2];
  int16   fillValue;
  int16   satValue;
  float64 scale;

} GRID_SR;    

void usage(char *);
int  getMetaInfo(GRID_SR *);
int  openForWrite(GRID_SR *sr, GRID_SR *cfmask);
int  closeUp(GRID_SR *sr, bool close);

#define DEBUG 1

int main(int argc, char *argv[])
{
  int i, irow, ret;
  int32 start[2];
  int32 length[2];
  uint8 *buf = NULL;
  GRID_SR sr, cfmask;

  if(argc != 5) {
    usage(argv[0]);
    exit(1);
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
	exit(1);
      }    
  }

  strcpy(sr.sdsName, "band1");
  strcpy(cfmask.sdsName, "fmask_band");

  /* get metadata from lndsr* file */
  if((ret=getMetaInfo(&sr))==FAILURE) {
    printf("Retrieve %s metadata error\n", sr.fileName);
    exit(1);
  }

  /* get metadata from cfmask* file */
  if((ret=getMetaInfo(&cfmask))==FAILURE) {
    printf("Retrieve %s metadata error\n", cfmask.fileName);
    exit(1);
  }
  
  if(sr.nrows!=cfmask.nrows || sr.ncols!=cfmask.ncols || sr.ulx!=cfmask.ulx ||
     sr.uly!=cfmask.uly)
    printf("Warning: lndsr and cfmask images do not match!  The number of "
        "lines and samples, along with the UL x and y pixel locations need "
        "to be the same.\n");

  /* allocate memory */
  buf = calloc(cfmask.ncols, sizeof(uint8));
  if(!buf) {
    printf("Memory allocation error\n");
    exit(1);
  }
  
  /* create the cfmask band */
  printf("Updating lndsr file for creating/modifying cfmask SDS ...\n");
  if((ret=openForWrite(&sr, &cfmask))==FAILURE) {
    printf("Open for write error\n");
    exit(1);
  }

  if(ret == 0)
    printf("Appending cfmask for line --- ");
  else
    printf("Updating cfmask for line --- ");

  /* loop through each row of data */
  for(irow=0; irow<sr.nrows; irow++) {
    printf("%4d\b\b\b\b",irow);
   
    start[0] = irow;
    start[1] = 0;
    length[0] = 1;
    length[1] = cfmask.ncols;

    if((SDreaddata(cfmask.band_id, start, NULL, length, buf)) == FAILURE) {
      printf("Error: reading cfmask error irow: %d\n", irow);
      exit(1);
    }
    
    if((SDwritedata(sr.band_id, start, NULL, length, buf)) == FAILURE) {
      printf("Error: writing to combined file error irow: %d\n", irow);
      exit(1);
    }
  } /* end of irow */
  printf ("\n");

  /* close HDF files */
  closeUp(&sr, true);
  closeUp(&cfmask, true);

  /* Free buffers */
  free(buf);
  return EXIT_SUCCESS;      /* success */
}


/* display usage */
void usage(char *command)
{
  printf("Usage: %s -sr lndsr_HDF_file -cfmask cfmask_HDF_file\n", command);
  printf("  where ...\n");
  printf("  lndsr_HDF_file is the surface reflectance file\n");
  printf("  cfmask_HDF_file is the cfmask cloud mask file\n");
}


/* get metadata and open specific sds */
int getMetaInfo(GRID_SR *sr) {

  int index, ret;

  char GD_gridlist[100];
  int32 gfid=0, ngrid=0, gid=0;
  int32 bufsize=100;
  float64 GD_upleft[2];
  float64 GD_lowright[2];
  int32 att_id;

  /* open a hdf file */
  gfid = GDopen(sr->fileName, DFACC_READ);
  if(gfid==FAILURE){
      printf ("Not successful in retrieving grid file ID/open\n");
      return FAILURE;
  }

  /* find out about grid type */
  ngrid=GDinqgrid(sr->fileName, GD_gridlist,&bufsize);
  if(ngrid==FAILURE){
      printf ("Not successful in retrieving grid name list\n");
      return FAILURE;
  }

  /* attach grid */
  gid = GDattach(gfid, GD_gridlist);
  if(gid==FAILURE){
      printf ("Not successful in attaching grid.\n");
      return FAILURE;
  }

  /* get grid info */
  ret = GDgridinfo(gid, &sr->ncols, &sr->nrows, GD_upleft, GD_lowright);
  if(ret==FAILURE){
      printf ("Failed to read grid info.\n");
      return FAILURE;
  }
  sr->ulx = GD_upleft[0];
  sr->uly = GD_upleft[1];

  /* detach grid */
  ret = GDdetach(gid);
  if(ret==FAILURE){
      printf ("Failed to detach grid.\n");
      return FAILURE;
  }

  /* close for grid access */
  ret = GDclose(gfid);
  if(ret==FAILURE){
      printf ("GD-file close failed.\n");
      return FAILURE;
  }

  /* open hdf file and get sds_id from given sds_name */  
  if ((sr->SD_ID = SDstart(sr->fileName, DFACC_READ))<0) {
    printf("Can't open file %s\n",sr->fileName);
    return FAILURE;
  }

  /* open band1 */
  if ((index=SDnametoindex(sr->SD_ID, sr->sdsName))<0) {
    printf("Not successful in convert sdsName %s to index\n", sr->sdsName);
    return FAILURE;
  }
  sr->band_id = SDselect(sr->SD_ID, index);

  /* retrieve SR Fill Value and other attributes */
  if ((att_id = SDfindattr(sr->band_id, "_FillValue")) == FAILURE) {
    printf ("Can't retrieve fill value from SDS %s.\n", sr->sdsName);
    return FAILURE;
  }
  else if (SDreadattr(sr->band_id, att_id, &sr->fillValue) == FAILURE) {
    printf ("Can't retrieve fill value from SDS %s.\n", sr->sdsName);
    return FAILURE;
  }

  if ((att_id = SDfindattr(sr->band_id, "_SaturateValue")) == FAILURE) {
    printf ("Can't retrieve saturation value from SDS %s (expected for "
      "cfmask).\n", sr->sdsName);
  }
  else if (SDreadattr(sr->band_id, att_id, &sr->satValue) == FAILURE) {
    printf ("Can't retrieve saturation value from SDS %s.\n", sr->sdsName);
    return FAILURE;
  }

  if ((att_id = SDfindattr(sr->band_id, "long_name")) == FAILURE) {
    printf ("Can't retrieve long name from SDS %s.\n", sr->sdsName);
    return FAILURE;
  }
  else if (SDreadattr(sr->band_id, att_id, sr->longName) == FAILURE) {
    printf ("Can't retrieve long name from SDS %s.\n", sr->sdsName);
    return FAILURE;
  }

  if ((att_id = SDfindattr(sr->band_id, "units")) == FAILURE) {
    printf ("Can't retrieve units from SDS %s (expected for cfmask).\n",
      sr->sdsName);
  }
  else if (SDreadattr(sr->band_id, att_id, sr->units) == FAILURE) {
    printf ("Can't retrieve units from SDS %s.\n", sr->sdsName);
    return FAILURE;
  }

  if ((att_id = SDfindattr(sr->band_id, "valid_range")) == FAILURE) {
    printf ("Can't retrieve valid range from SDS %s.\n", sr->sdsName);
    return FAILURE;
  }
  else if (SDreadattr(sr->band_id, att_id, sr->range) == FAILURE) {
    printf ("Can't retrieve valid range from SDS %s.\n", sr->sdsName);
    return FAILURE;
  }

  if ((att_id = SDfindattr(sr->band_id, "scale_factor")) == FAILURE) {
    printf ("Can't retrieve scale factor from SDS %s (expected for cfmask).\n",
      sr->sdsName);
  }
  else if (SDreadattr(sr->band_id, att_id, &sr->scale) == FAILURE) {
    printf ("Can't retrieve scale factor from SDS %s.\n", sr->sdsName);
    return FAILURE;
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

  if((SDend(sr->SD_ID)) == FAILURE) {
    printf("SDend for %s error!\n", sr->fileName);
    return FAILURE;
  }
  sr->SD_ID = 9999;

  /* create output use HDF-EOS functions */
  GDfid = GDopen(sr->fileName, DFACC_RDWR);
  if(GDfid == FAILURE){
    printf("Not successful in creating grid file %s", sr->fileName);
    return FAILURE;
  }
  
  GDid = GDattach(GDfid, "Grid");
  if(GDid == FAILURE) {
    printf("Not successful in attaching grid ID/ create for %s", sr->fileName);
    return FAILURE;
  }

  /* Create the new SDS in the Grid, if it doesn't already exist */
  if(exist == 0) {
    ret = GDdeffield(GDid, cfmask->sdsName, "YDim,XDim", DFNT_UINT8,
      HDFE_NOMERGE);
    if(ret==FAILURE){
      printf ("Not successful in defining %s SDS", cfmask->sdsName);
      return FAILURE;
    }
  }
  
  /* detach grid */
  ret = GDdetach(GDid);
  if(ret==FAILURE){
    printf ("Failed to detach grid.");
    return FAILURE;
  }

  /* close for grid access */
  ret = GDclose(GDfid);
  if(ret==FAILURE){
    printf ("GD-file close failed.");
    return FAILURE;
  }
 
  /* open hdf file */
  if ((sr->SD_ID = SDstart(sr->fileName, DFACC_RDWR))<0) {
    printf("Can't open file %s",sr->fileName);
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
  if (ret == FAILURE) {
    printf("Can't write SR long_name for SDS %s\n", cfmask->sdsName);
    return FAILURE;
  } 
    
  ret = SDsetattr(sr->band_id, "valid_range", DFNT_INT16, 2, cfmask->range);
  if (ret == FAILURE) {
    printf ("Can't write SR valid_range for SDS %s", cfmask->sdsName);
    return FAILURE;
  } 
  
  ret = SDsetattr(sr->band_id, "_FillValue", DFNT_INT16, 1,
    &(cfmask->fillValue));
  if (ret == FAILURE) {
    printf ("Can't write SR _FillValue for SDS %s", cfmask->sdsName);
    return FAILURE;
  } 
  
  sprintf (lndsr_index, "0 clear; 1 water; 2 cloud_shadow; 3 snow; 4 cloud");
  ret = SDsetattr(sr->band_id, "mask_index", DFNT_CHAR8, strlen(lndsr_index),
    lndsr_index);
  if (ret == FAILURE) {
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

  return exist;
}


int closeUp(GRID_SR *sr, bool close) 
{
  if((SDendaccess(sr->band_id)) == FAILURE) {
    printf("SDendaccess for %s cfmask band error!\n", sr->fileName);
    return FAILURE;
  }

  if (close) {
    if((SDend(sr->SD_ID)) == FAILURE) {
      printf("SDend for %s error!\n", sr->fileName);
      return FAILURE;
    }
    sr->SD_ID = 9999;
  }

  return SUCCESS;
}
