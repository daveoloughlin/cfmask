1. cd to data directory where original landsat data and *MTL* data are available
2. run $BIN/lndpm <Landsat_meta_file> first
3, run /home/sguo/Fmask_c_2_1/HDF_cal_th_code/cfmask2/lndcal2/lndcal <lndcal_input_text>
4. cd to cfmask directory
5. ./cfmask $TEST_DATA/<ledaps output metadata file> 
