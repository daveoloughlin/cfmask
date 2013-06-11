#include "input.h"
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "cv.h"
#include "ml.h"
#include "cxcore.h"
#include "highgui.h"
//#include "cvblob.h"
#include "ias_misc_2d_array.h"
#include "input.h"

//using namespace cvb;

#define MINSIGMA 1e-5
#define MAX_CLOUD_TYPE 500000
#define MIN_CLOUD_OBJ 9

static int num_clouds = 0;

#if 0
bool cmpY(const pair<CvLabel, CvBlob*>  &p1, const pair<CvLabel, CvBlob*> &p2) 
{   return p1.second->centroid.y < p2.second->centroid.y; }  
#endif

int viewgeo
(
   int x_ul,
   int y_ul,
   int x_ur,
   int y_ur,
   int x_ll,
   int y_ll,
   int x_lr,
   int y_lr,
   float *a,
   float *b,
   float *c,
   float *omiga_par,
   float *omiga_per
)
{
 int x_u, x_l, y_u, y_l;
 float k_ulr = 1.0;
 float k_llr = 1.0;
 float k_aver; 
 char errstr[MAX_STR_LEN];

  x_u=(x_ul+x_ur)/2;
  x_l=(x_ll+x_lr)/2;
  y_u=(y_ul+y_ur)/2;
  y_l=(y_ll+y_lr)/2;

  if (x_ul != x_ur)
      k_ulr=(y_ul-y_ur)/(x_ul-x_ur); /* get k of upper left and right points */
  else
    {
        sprintf (errstr, "calc view geometry");
        ERROR (errstr, "cloud/shadow match");
    }
  if (x_ll != x_lr)      
      k_llr=(y_ll-y_lr)/(x_ll-x_lr); /* get k of lower left and right points */
  else
    {
        sprintf (errstr, "calc view geometry");
        ERROR (errstr, "cloud/shadow match");
    }
  k_aver=(k_ulr+k_llr)/2;
  *omiga_par=atan(k_aver); /* get the angle of parallel lines k (in pi) */

  /* AX(j)+BY(i)+C=0 */
  *a=y_u-y_l;
  *b=x_l-x_u;
  *c=y_l*x_u-x_l*y_u;

  *omiga_per=atan((*b)/(*a)); /* get the angle which is perpendicular to the 
                                 trace line */

  printf("omiga_par, omiga_per =%f, %f\n",*omiga_par,*omiga_per);

  return 0;
}


  // mat_truecloud function
  int mat_truecloud
  (
    unsigned char *x,
    unsigned char *y,
    int array_length,
    float *h,
    float a,
    float b,
    float c,
    float omiga_par,
    float omiga_per,
    unsigned char *x_new,
    unsigned char *y_new
)
{
 float dist;
 float dist_par;
 float dist_move;
 float delt_x;
 float delt_y;

   int height=705000; // average Landsat 4,5,&7 height (m)
   int i;

   for (i = 0; i < array_length; i++)
   {
       dist = (a*x[i]+b*y[i]+c)/(sqrt(a*a+b*b)); 
                /* from the cetral perpendicular (unit: pixel) */
       dist_par = dist/cos(omiga_per-omiga_par);
       dist_move = (dist_par*h[i])/height; /* cloud move distance (m) */
       delt_x = dist_move*cos(omiga_par);
       delt_y = dist_move*sin(omiga_par);

       x_new[i] = x[i] + delt_x; /* new x, j */
       y_new[i] = y[i] + delt_y; /* new y, i */
   }

   return 0;
}

int find_min(int *array, int nums)
{
    int i;
    int min = 0;

     for (i = 0; i < nums; i++)
     {
      if (array[i] != 0)
      {
         min = array[i];
         break;
      }
     }
     for (i = 0; i < nums; i++)
     {
      if (array[i] != 0 && array[i] < min)
       min = array[i];
     }

     return min;

}

int label(unsigned char **cloud_mask, int srow, int scol, int nrows, int ncols, 
          int **xys_list, int *obj_num)
{ 
    int row, col;
    int i,j;
    int array[4];
    int min;

    for (row = srow; row < nrows; row++)
    {
     for (col = scol; col <ncols; col++)
     {
      if (cloud_mask[row][col] == 1)
      {
          if (row > 0 && col > 0 && cloud_mask[row-1][col-1] == 1)
              array[0] = xys_list[row-1][col-1];
          else
              array[0] = 0;
          if ( row > 0 && cloud_mask[row-1][col] == 1)
              array[1] = xys_list[row-1][col];
          else
              array[1] = 0;
          if (row > 0 && (col < ncols-1) && cloud_mask[row-1][col+1] == 1)
              array[2] = xys_list[row-1][col+1];
          else
              array[2] = 0;
          if (col > 0 && cloud_mask[row][col-1] == 1)
              array[3] = xys_list[row][col-1];
          else
              array[3] = 0;
              
          min = find_min(array, 4);
          if (min == 0)
          {
            num_clouds++;    
            xys_list[row][col] = num_clouds;
            obj_num[num_clouds]++;
          }
          else
          {
             if ((row > 0 && col > 0 && cloud_mask[row-1][col-1] == 1) && 
                 (xys_list[row-1][col-1] != min))
             {
                 xys_list[row][col] = min;
                 obj_num[min]++; 
                 for (i = 0; i < row; i++)
                 {
                     for (j = 0; j < ncols; j++)
                     {
                         if (xys_list[i][j] == xys_list[row-1][col-1])
                         {
                             obj_num[xys_list[i][j]]--;
                             xys_list[i][j] = min;
                             obj_num[min]++;
                         }
                     }
                 } 
                 for (j = 0; j < col; j++)
                 {
                     if (xys_list[row][j] == xys_list[row-1][col-1])
                     {
                         obj_num[xys_list[row][j]]--;
                         xys_list[row][j] = min;
                         obj_num[min]++;
                     }
                 }
             }
             if ((row > 0 && cloud_mask[row-1][col] == 1) && 
                 (xys_list[row-1][col] != min))
             {
                 xys_list[row][col] = min;
                 obj_num[min]++; 
                 for (i = 0; i < row; i++)
                 {
                     for (j = 0; j < ncols; j++)
                     {
                         if (xys_list[i][j] == xys_list[row-1][col])
                         {
                             obj_num[xys_list[i][j]]--;
                             xys_list[i][j] = min;
                             obj_num[min]++;
                         }
                     }
                 } 
                 for (j = 0; j < col; j++)
                 {
                     if (xys_list[row][j] == xys_list[row-1][col])
                     {
                         obj_num[xys_list[row][j]]--;
                         xys_list[row][j] = min;
                         obj_num[min]++;
                     }
                 }
             }
             if ((row > 0 && (col < ncols-1) && cloud_mask[row-1][col+1] == 1) 
                 && (xys_list[row-1][col+1] != min))
             {
                 xys_list[row][col] = min;
                 obj_num[min]++; 
                 for (i = 0; i < row; i++)
                 {
                     for (j = 0; j < ncols; j++)
                     {
                         if (xys_list[i][j] == xys_list[row-1][col+1])
                         {
                             obj_num[xys_list[i][j]]--;
                             xys_list[i][j] = min;
                             obj_num[min]++;
                         }
                     }
                 } 
                 for (j = 0; j < col; j++)
                 {
                     if (xys_list[row][j] == xys_list[row-1][col+1])
                     {
                         obj_num[xys_list[row][j]]--;
                         xys_list[row][j] = min;
                         obj_num[min]++;
                     }
                 }
             }
             if ((col > 0 && cloud_mask[row][col-1] == 1) && 
                 (xys_list[row][col-1] != min))
             {
                 xys_list[row][col] = min;
                 obj_num[min]++; 
                 for (i = 0; i < row; i++)
                 {
                     for (j = 0; j < ncols; j++)
                     {
                         if (xys_list[i][j] == xys_list[row][col-1])
                         {
                             obj_num[xys_list[i][j]]--;
                             xys_list[i][j] = min;
                             obj_num[min]++;
                         }
                     }
                 } 
                 for (j = 0; j < col; j++)
                 {
                     if (xys_list[row][j] == xys_list[row][col-1])
                     {
                         obj_num[xys_list[row][j]]--;
                         xys_list[row][j] = min;
                         obj_num[min]++;
                     }
                 }
             }
          }
       }
     }
    }

    printf("first pass labeling passed\n");

    return 0;
}      

int object_cloud_shadow_match
(
  Input_t *input,
  float ptm,
  float t_templ,
  float t_temph,
  int cldpix,
  int sdpix,
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
    int row;
    int col = 0;
    float sun_ele;
    float sun_ele_rad;
    float sun_tazi;
    float sun_tazi_rad;
    int sub_size = 30;
    int status;
    int cloud_counter = 0;
    int boundary_counter = 0;
    float revised_ptm;

    /* Dynamic memory allocation */
    unsigned char **cloud_cal;
    unsigned char **shadow_cal;
    unsigned char **boundary_test;

    cloud_cal = (unsigned char **)ias_misc_allocate_2d_array(input->size.l, 
                 input->size.s, sizeof(unsigned char)); 
    shadow_cal = (unsigned char **)ias_misc_allocate_2d_array(
                 input->size.l, input->size.s, sizeof(unsigned char)); 
    boundary_test = (unsigned char **)ias_misc_allocate_2d_array(
                 input->size.l, input->size.s, sizeof(unsigned char)); 
    if (cloud_cal == NULL || shadow_cal == NULL ||boundary_test == NULL)
    {
        sprintf (errstr, "Allocating mask memory");
        ERROR (errstr, "cloud/shadow match");
    }

    /* Read in potential mask ... */
    /* Solar elevation angle */
    sun_ele = 90 - input->meta.sun_zen;
    sun_ele_rad = (PI / 180.0) * sun_ele;
    /* Solar azimuth angle */
    sun_tazi = input->meta.sun_az - 90;
    sun_tazi_rad = (PI / 180.0) * sun_tazi;

    for (row = 0; row < nrows; row++)
    {
      for (col = 0; col < ncols; col++)
      {
        if (cloud_mask[row][col] == 1)
          cloud_counter++;
 
        /* Boundary layer includes both cloud_mask equals 0 and 1 */
        if (cloud_mask[row][col] < 255)     
        {
          boundary_test[row][col] = 1;
          boundary_counter++;
        }
        else
          boundary_test[row][col] = 0;
      }
    }

     /* Revised percent of cloud on the scene after plcloud */
     revised_ptm = (float)cloud_counter / (float)boundary_counter;

     printf("Revised percent of cloud = %f\n", revised_ptm);

     /* no t test  => more than 98 % clouds and partly cloud over land
         => no match => rest are definite shadows

        cloud covers more than 90% of the scene
         => no match => rest are definite shadows
         fprintf('Cloud and cloud shadow matching ...\n');  */
     if (ptm <= 0.1 || revised_ptm >= 0.90)
     {
        for (row = 0; row < nrows; row++)
        {
          for (col = 0; col < ncols; col++)
          {
             /* No Shadow Match due to too much cloud (>90 percent) */
             if (cloud_mask[row][col] == 1) 
                 cloud_cal[row][col] = 1;
             else
                shadow_cal[row][col] = 1;
             if (cloud_mask[row][col] == 0)
             {
                cloud_cal[row][col] = 0;
                shadow_cal[row][col] = 0;
             } 
          }
        }
     }
     else
     {
        printf("Shadow Match in processing\n");

        /* define constants */
        float t_similar=0.30;
        float t_buffer=0.98; /* threshold for matching buffering */
        int num_cldoj=9; /* minimum matched cloud object (pixels) */
        int num_pix=8; /* number of inward pixes (240m) for cloud base 
                          temperature */

        printf("Set cloud similarity = %.3f\n", t_similar);
        printf("Set matching buffer = %.3f\n", t_buffer);
        printf("Shadow match for cloud object >= %d pixels\n", num_cldoj);

        float i_step;
        i_step=2*sub_size*tan(sun_ele_rad); /* move 2 pixel at a time */

       /* Get moving direction, the idea is to get the corner rows/cols */
        int x_ul = 0;
        int y_ul = 0;
        int x_lr = 0;
        int y_lr = 0;
        int x_ll = 0;
        int y_ll = 0;
        int x_ur = 0;
        int y_ur = 0;
        printf("nrows,ncols=%d,%d\n",nrows,ncols);
        for (row = 0; row < nrows; row++)
        {
          for (col = 0; col < ncols; col++)
          {
           if (boundary_test[row][col] == 1)
           {
             x_ul = row;
             y_ul = col;
             goto next1;
           }
          }
        }

     next1:        
        for (col = ncols - 1; col >= 0; col--)
        {
          for (row = 0; row < nrows; row++)
          {
           if (boundary_test[row][col] == 1)
           {
             x_ur = row;
             y_ur = col;
             goto next2;
           }
          }
        }

     next2:
        for (col = 0; col < ncols; col++)
        {
          for (row = nrows - 1; row >= 0; row--)
          {
           if (boundary_test[row][col] == 1)
           {
             x_ll = row;
             y_ll = col;
             goto next3;
           }
          }
        }

     next3:
        for (row = nrows - 1; row >= 0; row--)
        {
          for (col = ncols - 1; col >= 0; col--)
          {
           if (boundary_test[row][col] == 1)
           {
             x_lr = row;
             y_lr = col;
             goto next4;
           }
          }
        }

     next4:
        printf("========%d,%d,%d,%d,%d,%d,%d,%d\n",x_ul,y_ul,x_ur,y_ur,x_ll,
               y_ll,x_lr,y_lr);

        /* get view angle geometry */
        float a, b, c, omiga_par, omiga_per;
        status = viewgeo(x_ul,y_ul,x_ur,y_ur,x_ll,y_ll,x_lr,y_lr, &a, &b, &c, 
            &omiga_par, &omiga_per);
        if (status != 0)
        {
            sprintf (errstr, "Calling viewgeo");
            ERROR (errstr, "cloud/shadow match");
           return 0;
        }
#if 0
      IplImage* img = cvCreateImage(cvSize(ncols, nrows), IPL_DEPTH_8U, 1);
      IplImage* imgOut = cvCreateImage(cvSize(ncols, nrows), IPL_DEPTH_DEPTH, 1);
      if (!img)
      {
          sprintf (errstr, "Reading input image data for line %d, "
                   "band %d", row, ib);
          ERROR (errstr, "cloud/shadow match");
      }
       //     cvSet2D(img, ncols, nrows, new_nir);
      for (row = 0; row < nrows; row++)
      {
       for (col = 0; col < ncols; col++)
        {
           img->imageData[row*ncols+col] = cloud_mask[row][col];
        }
      }

      CvBlobs blobs; 
      unsigned int result=cvLabel(img, imgOut, blobs);

for (CvBlobs::const_iterator it=blobs.begin(); it!=blobs.end(); ++it) 
{   cout << "Blob #" << it->second->label << ": Area=" << it->second->area << ", Centroid=(" << it->second->centroid.x << ", " << it->second->centroid.y << ")" << endl; }


vector< pair<CvLabel, CvBlob*> > blobList;   
copy(blobs.begin(), blobs.end(), back_inserter(blobList));    
sort(blobList.begin(), blobList.end(), cmpY);   
for (int i=0; i<blobList.size(); i++)   
 {     cout << "[" << blobList[i].first << "] -> " << (*blobList[i].second) << endl; }

#endif

    /* Allocate memory for segment cloud portion */
    int *obj_num;
    obj_num = (int *)calloc(MAX_CLOUD_TYPE, sizeof(int));
    int **xys_list;
    xys_list = (int **)ias_misc_allocate_2d_array(input->size.l, 
                input->size.s, sizeof(int));
    if (obj_num == NULL || xys_list == NULL)
    {
        printf("Error allcating obj_num/xys_list memory\n");
        return -1;
    }
    for (row = 0; row < nrows; row++)
    {
     for (col = 0; col <ncols; col++)
     {
      xys_list[row][col] = 0;
     }
    }
#if 0
    FILE *cmask = NULL;
    cmask = fopen("cloud_mask.dat", "wb");
    if (cmask==NULL)
    {
        printf("Error from open cfmask\n");
        return -1;
    }
    fwrite(&cloud_mask[0][0], sizeof(unsigned char), nrows*ncols, cmask);
    fclose(cmask);
#endif

    int nl, ns, i, j, srow, scol, rows, cols;
    nl = floor(nrows/4);
    ns = floor(ncols/4);
    printf("nl,ns=%d,%d\n",nl, ns);

    for (i = 0; i < 4; i++)
    {
      for (j = 0; j < 4; j++)
      {
        srow = i * nl;
        scol = j * ns;
        if (i != 3)
          rows = nl - 1;
        else
         rows = nrows - 3 * nl + 1;   
        if (j != 3)
          cols = ns - 1;
        else
          cols = ncols - 3 * ns + 1;   
        status = label(cloud_mask, srow, scol, rows, cols, xys_list, obj_num);
         if (status != 0)
         {
            sprintf (errstr, "Calling label");
            ERROR (errstr, "cloud/shadow match");
            return -1;
        }
      }
    }

    printf("Num of clouds = %d\n", num_clouds);

     /* The cloud pixels are not counted as cloud pixels if the total number of
        cloud pixels is less than 9 within a cloud cluster */
     int num;
     int counter = 0;
     for (num = 0; num < num_clouds; num++)
     {
      if (obj_num[num+1] < MIN_CLOUD_OBJ)
          obj_num[num+1] = 0;
      if (obj_num[num+1] == 0)
          counter++;
     }

     printf("Num of real clouds = %d\n", counter);

     /* Cloud_cal pixels are cloud_mask pixels with < 9 pixels removed */
    for (row = 0; row < nrows; row++)
    {
     for (col = 0; col <ncols; col++)
     {
      if ((cloud_mask[row][col] == 1) && (obj_num[xys_list[row][col]] != 0))
       cloud_cal[row][col] = cloud_mask[row][col];
      else
       cloud_cal[row][col] = 0;
     }
    }

    /* Need to read out whole image brightness temperature for band 6 */
    int16 **temp;
    temp = (int16 **)ias_misc_allocate_2d_array(input->size.l, 
                 input->size.s, sizeof(int16)); 
    if (!temp)
    {
        sprintf (errstr, "Allocating temp memory");
        ERROR (errstr, "cloud/shadow match");
    }

    /* Read out thermal band in 2d */
    for (row = 0; row < nrows; row++)
    {
	if (!GetInputThermLine(input, row))
        {
	  sprintf (errstr, "Reading input thermal data for line %d", row);
	  ERROR (errstr, "cloud/shadow match");
	}
        memcpy(temp[row], input->therm_buf, input->size.s * 
               sizeof(unsigned char));
    }

    /* TODO: Use iteration to get the optimal move distance, Calulate the 
       moving cloud shadow */
    int cloud_type;
    for (cloud_type = 1; cloud_type <= num_clouds; cloud_type++)
    {
     /* printf("Shadow Match of the %d/%d_th cloud with %d pixels\n",
               cloud_type,num,obj_num(cloud_type)); */

     if (obj_num[cloud_type] == 0)
      continue;
     else
     {
       int **xy_type;
       int **tmp_xy_type;
       unsigned char **tmp_xys;
       unsigned char **orin_xys;
       unsigned char *orin_cid;
       unsigned char *tmp_id;
       unsigned char *temp_ii;
       unsigned char *temp_jj;

       /* Note: matlab array index starts with 1 and C starts with 0, also 
          row/col reversed in matlab and C */
       /* moving cloud xys */
       xy_type = (int **)ias_misc_allocate_2d_array(2, 
                obj_num[cloud_type], sizeof(int)); 
       /* record the max threshold moving cloud xys */
       tmp_xy_type = (int **)ias_misc_allocate_2d_array(2, 
               obj_num[cloud_type], sizeof(int)); 
       /* corrected for view angle xys */
       tmp_xys = (unsigned char **)ias_misc_allocate_2d_array(2, 
               obj_num[cloud_type], sizeof(unsigned char)); 
       /* record the original xys */
       orin_xys = (unsigned char **)ias_misc_allocate_2d_array(2, 
               obj_num[cloud_type], sizeof(unsigned char)); 
       orin_cid = malloc(obj_num[cloud_type] * sizeof(unsigned char));
       tmp_id = malloc(obj_num[cloud_type] * sizeof(unsigned char));
       temp_ii = malloc(obj_num[cloud_type] * sizeof(unsigned char));
       temp_jj = malloc(obj_num[cloud_type] * sizeof(unsigned char));
       if (xy_type == NULL || tmp_xy_type == NULL || tmp_xys == NULL || 
          orin_xys == NULL || orin_cid == NULL || tmp_id == NULL || 
          temp_ii == NULL || temp_jj == NULL)
       {
           sprintf (errstr, "Allocating cloud memory");
           ERROR (errstr, "cloud/shadow match");
       }

    /* Get the cloud_type pixels, through orin_cid to map index with row/col 
       of each cloud */
    int index = 0;
    for (row = 0; row < nrows; row++)
    {
     for (col = 0; col <ncols; col++)
     {
       if (xys_list[row][col] == cloud_type)
       {
         orin_cid[index] = row * ncols + col;
         index++;
       }
     }
    }

    /* Temperature of the cloud object */
    int16 *temp_obj; 
    temp_obj = malloc(obj_num[cloud_type] * sizeof(int16));
    if (temp_obj == NULL)
    {
        sprintf (errstr, "Allocating temp_obj memory");
        ERROR (errstr, "cloud/shadow match");
    }
     
    int i;
    int16 temp_obj_max = 0;
    int16 temp_obj_min = 0;
    for (i = 0; i < obj_num[cloud_type]; i++)
    {
     /* Only count pixels within cloud_obj */
     row = floor(orin_cid[i]/ncols);
     col = orin_cid[i] - row * ncols;
     temp_obj[i] = temp[row][col]; 
     if (temp_obj[i] > temp_obj_max)
      temp_obj_max = temp_obj[i];
     if (temp_obj[i] < temp_obj_min)
      temp_obj_min = temp_obj[i];
     orin_xys[0][i] = row;
     orin_xys[1][i] = col;
    }

    /* the base temperature for cloud
       assume object is round r_obj is radium of object */
        float r_obj;
        float pct_obj;
        float t_obj;
        r_obj=sqrt(obj_num[cloud_type]/PI);
        /* number of inward pixes for correct temperature &  num_pix=8 */
        pct_obj=((r_obj-num_pix)*(r_obj-num_pix))/(r_obj * r_obj);
        if (pct_obj >= 1)
         pct_obj = 1; /* pct of edge pixel should be less than 1 */
        // prctile(temp_obj, obj_num[cloud_type], 100. * pct_obj, &t_obj);
        t_obj = pct_obj * (float)(temp_obj_max - temp_obj_min) + 
                (float)temp_obj_min;
        /* put the edge of the cloud the same value as t_obj */
        for (i = 0; i < obj_num[cloud_type]; i++)
        {
         if (temp_obj[i] > t_obj)
            temp_obj[i]=t_obj;
        }
        /* wet adiabatic lapse rate 6.5 degrees/km
           dry adiabatic lapse rate 9.8 degrees/km */
        float rate_elapse=6.5;
        float rate_dlapse=9.8;
        float max_cl_height=12000.0; /* Max cloud base height (m) */
        float min_cl_height=200; /* Min cloud base height (m) */
        /* refine cloud height range (m) */
        if (min_cl_height < 10*(t_templ-400-t_obj)/rate_dlapse)
         min_cl_height = 10*(t_templ-400-t_obj)/rate_dlapse;
        if (max_cl_height > 10*(t_temph+400-t_obj))
         max_cl_height = 10*(t_temph+400-t_obj);
        /* initialize height and similarity info */
        float record_thresh=0.0;

        float base_h = min_cl_height;
        while(base_h < max_cl_height)
        {
            /* Get the true postion of the cloud
               calculate cloud DEM with initial base height */
            float *h;
            h = malloc(obj_num[cloud_type] * sizeof(float));
            if (h == NULL)
            {
                sprintf (errstr, "Allocating h memory");
                 ERROR (errstr, "cloud/shadow match");
            }
            for (i = 0; i < obj_num[cloud_type]; i++)
            {    
              h[i]=(10*(t_obj-temp_obj[i])/rate_elapse+base_h);
            }

            mat_truecloud(orin_xys[0], orin_xys[1], obj_num[cloud_type], 
                 h, a, b, c, omiga_par, omiga_per, tmp_xys[0], tmp_xys[1]);
            /* shadow moved distance (pixel)
               i_xy=h*cos(sun_tazi_rad)/(sub_size*tan(sun_ele_rad)); */
            float i_xy;
            int out_all = 0;
            int in_all = 0;
            for (i = 0; i < obj_num[cloud_type]; i++)
            {
                i_xy=h[i]/(sub_size*tan(sun_ele_rad));
                if ((input->meta.sun_az - 180) < MINSIGMA)
                {
                    xy_type[1][i]=rint(tmp_xys[0][i]-i_xy*cos(sun_tazi_rad)); 
                    xy_type[0][i]=rint(tmp_xys[1][i]-i_xy*sin(sun_tazi_rad)); 
                }
                else
                {
                    xy_type[1][i]=rint(tmp_xys[0][i]+i_xy*cos(sun_tazi_rad)); 
                    xy_type[0][i]=rint(tmp_xys[1][i]+i_xy*sin(sun_tazi_rad)); 
                }

                /* the id that is out of the image */
                if (xy_type[0][i] < 0 || xy_type[0][i] >= nrows 
                    || xy_type[1][i] < 0 || xy_type[1][i] >= ncols)
                    out_all++;
                else
                {
                    temp_ii[in_all] = xy_type[0][i];
                    temp_jj[in_all] = xy_type[1][i];
                    in_all++;
                }
            }

            /* tmp_id=sub2ind(ijDim,tmp_ii,tmp_jj); */
            int match_all = 0;
            int total_all = 0;
            for (i = 0; i < in_all; i++)
            {
              if (boundary_test[temp_ii[i]][temp_jj[i]] == 0 || 
                  (xys_list[temp_ii[i]][temp_jj[i]] != cloud_type 
                 &&(cloud_mask[temp_ii[i]][temp_jj[i]]>0||
                    shadow_mask[temp_ii[i]][temp_jj[i]]==1)))
                  match_all++;
              if (xys_list[temp_ii[i]][temp_jj[i]] != cloud_type)
                  total_all++; 
            }
            match_all += out_all;
            total_all+=out_all;

            float thresh_match;
            thresh_match=match_all/total_all;
            if ((thresh_match >= t_buffer*record_thresh)&&(base_h < 
                 max_cl_height-i_step)&&(record_thresh<0.95))
            {
                if (thresh_match > record_thresh)
                 {
                    record_thresh=thresh_match;
                 }
            }
            else if (record_thresh > t_similar)
            {
                float i_vir;
#if 0
                printf("The similar_num for cloud %d is %f\n",
                       cloud_type, record_thresh);
#endif
                for (i = 0; i < obj_num[cloud_type]; i++)
                {
                   i_vir=h[i]/(sub_size*tan(sun_ele_rad));
                  if ((input->meta.sun_az - 180) < MINSIGMA)
                  {
                     tmp_xy_type[1][i]=rint(tmp_xys[0][i]-
                        i_vir*cos(sun_tazi_rad)); 
                     tmp_xy_type[0][i]=rint(tmp_xys[1][i]-
                        i_vir*sin(sun_tazi_rad)); 
                  }
                  else
                  {
                     tmp_xy_type[1][i]=rint(tmp_xys[0][i]+
                        i_vir*cos(sun_tazi_rad)); 
                     tmp_xy_type[0][i]=rint(tmp_xys[1][i]+
                        i_vir*sin(sun_tazi_rad)); 
                  }

                  /* put data within range */
                  if (tmp_xy_type[0][i]<1)
                    tmp_xy_type[0][i]=1;
                  if (tmp_xy_type[0][i]>nrows)
                   tmp_xy_type[0][i]=nrows;
                  if (tmp_xy_type[1][i]<1)
                   tmp_xy_type[1][i]=1;
                  if (tmp_xy_type[1][i]>ncols)
                   tmp_xy_type[1][i]=ncols;

                  shadow_cal[tmp_xy_type[0][i]][tmp_xy_type[1][i]] = 1; 
                }
                break;
            }
            else
            {
                record_thresh=0.0;
                continue;
            }
            base_h += i_step;
        }
        /* Free all the memory */
        status = ias_misc_free_2d_array((void **)xy_type);
        status = ias_misc_free_2d_array((void **)tmp_xy_type);
        status = ias_misc_free_2d_array((void **)tmp_xys);
        status = ias_misc_free_2d_array((void **)orin_xys);
        free(orin_cid);
        free(tmp_id);
        free(temp_ii);
        free(temp_jj);
        free(temp_obj);
     }
    }      
    free(obj_num); 
    status = ias_misc_free_2d_array((void **)temp);
    status = ias_misc_free_2d_array((void **)xys_list);

      IplImage* src = cvCreateImage(cvSize(ncols, nrows), IPL_DEPTH_8U, 1);
      IplImage* dst = cvCreateImage(cvSize(ncols, nrows), IPL_DEPTH_8U, 1);
      if (!src || !dst)
      {
          sprintf (errstr, "Creating images\n");
          ERROR (errstr, "cloud/shadow match");
      }
 
     for (row = 0; row < nrows; row++)
      {
       for (col = 0; col < ncols; col++)
        {
           src->imageData[row*ncols+col] = cloud_cal[row][col];
           //cvSet2D(src, col, row, cloud_cal[row][col]);
        }
      }

      cvDilate(src, dst, NULL, 1);
      for (row = 0; row < nrows; row++)
      {
       for (col = 0; col < ncols; col++)
        {
           cloud_cal[row][col] = dst->imageData[row*ncols+col];
           //cloud_cal[row][col] = cvGet2D(dst, col, row); 
        }
      }

      for (row = 0; row < nrows; row++)
      {
       for (col = 0; col < ncols; col++)
        {
           src->imageData[row*ncols+col] = shadow_cal[row][col];
           //cvSet2D(src, col, row, shadow_cal[row][col]);
        }
      }

      cvDilate(src, dst, NULL, 1);
      for (row = 0; row < nrows; row++)
      {
       for (col = 0; col < ncols; col++)
        {
           if (shadow_cal[row][col] != 1)
              shadow_cal[row][col] = 0;
           shadow_cal[row][col] = dst->imageData[row*ncols+col];
           //shadow_cal[row][col] = cvGet2D(dst, col, row); 
        }
      }

      for (row = 0; row < nrows; row++)
      {
       for (col = 0; col < ncols; col++)
        {
           src->imageData[row*ncols+col] = snow_mask[row][col];
           //cvSet2D(src, col, row, snow_mask[row][col]);
        }
      }

      cvDilate(src, dst, NULL, 1);
      for (row = 0; row < nrows; row++)
      {
       for (col = 0; col < ncols; col++)
        {
           snow_mask[row][col] = dst->imageData[row*ncols+col];
           //snow_mask[row][col] = cvGet2D(dst, col, row); 
        }
      }

      /* Release image memory */
      cvReleaseImage(&src);
      cvReleaseImage(&dst);

     }


   /* Use cloud mask as the final output mask */  
   int cloud_shadow_counter = 0;    
   for (row = 0; row < nrows; row++)
   {
      for (col = 0; col < ncols; col++)
      {
        if (water_mask[row][col] == 1)
	  final_mask[row][col] = 1;
        if (snow_mask[row][col] == 1)
          final_mask[row][col] = 3;
        if (shadow_cal[row][col] == 1)
        {
	  final_mask[row][col] = 2;
          cloud_shadow_counter++;
        }
        if (cloud_cal[row][col] == 1)
        {
	  final_mask[row][col] = 4;
          cloud_shadow_counter++;
        }
        if (boundary_test[row][col] == 0)
          final_mask[row][col] = 255;

        if ((water_mask[row][col] != 1) && (snow_mask[row][col] != 1) &&
            (shadow_cal[row][col] != 1) && (cloud_cal[row][col] != 1) &&
            boundary_test[row][col] != 255)
         final_mask[row][col] = 0;
      }
   }

   /* Release the memory */
   status = ias_misc_free_2d_array((void **)cloud_cal);
   status = ias_misc_free_2d_array(( void **)shadow_cal);
   status = ias_misc_free_2d_array(( void **)boundary_test);


   printf("cloud_shadow_counter, boundary_counter = %d,%d\n",
          cloud_shadow_counter, boundary_counter);

   /* record cloud and cloud shadow percent; */
   float cloud_shadow_percent;
   cloud_shadow_percent = (float)cloud_shadow_counter / (float)boundary_counter;
printf("The cloud and shadow percentage is %f\n", cloud_shadow_percent);
   
   return 0;
}
