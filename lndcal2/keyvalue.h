#ifndef KEYVALUE_H
#define KEYVALUE_H

#include "lndcal.h"

/* Data provider type values */

const Key_string_t Provider_string[PROVIDER_MAX] = {
  {(int)PROVIDER_UMD,  "UMD"},
  {(int)PROVIDER_EDC,  "EDC"},
  {(int)PROVIDER_CCRS, "CCRS"},
  {(int)PROVIDER_MRLC01, "MRLC01"},
  {(int)PROVIDER_MRLC92, "MRLC92"},
  {(int)PROVIDER_USDA,   "USDA"},
  {(int)PROVIDER_EROS,   "USGS/EROS"},
  {(int)PROVIDER_CI,     "CI"},
  {(int)PROVIDER_Scott,  "Scott"} 
};

/* Satellite type values */

const Key_string_t Sat_string[SAT_MAX] = {
  {(int)SAT_LANDSAT_1, "LANDSAT_1"},
  {(int)SAT_LANDSAT_2, "LANDSAT_2"},
  {(int)SAT_LANDSAT_3, "LANDSAT_3"},
  {(int)SAT_LANDSAT_4, "LANDSAT_4"},
  {(int)SAT_LANDSAT_5, "LANDSAT_5"},
  {(int)SAT_LANDSAT_7, "LANDSAT_7"}
};

/* Instrument type values */

const Key_string_t Inst_string[INST_MAX] = {
  {(int)INST_MSS, "MSS"},
  {(int)INST_TM,  "TM"},
  {(int)INST_ETM, "ETM"},
};

/* World Reference System (WRS) type values */

const Key_string_t Wrs_string[WRS_MAX] = {
  {(int)WRS_1, "1"},
  {(int)WRS_2, "2"},
};

/* Band gain settings (ETM+ only) values */

const Key_string_t Gain_string[GAIN_MAX] = {
  {(int)GAIN_HIGH, "HIGH"},
  {(int)GAIN_LOW,  "LOW"},
};

#endif
