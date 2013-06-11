#! /usr/bin/env python

import sys
from do_append_cfmask import AppendCfmask

ERROR = 1
SUCCESS = 0

if __name__ == "__main__":
    status = AppendCfmask().runAppend (sr_infile="/media/sf_Software_SandBox/Landsat/cfmask/lndsr.LT50700131994252XXX02.hdf", cfmask_infile="/media/sf_Software_SandBox/Landsat/cfmask/fmask.LT50700131994252XXX02.hdf", usebin=True)

    if status != SUCCESS:
        print "Error running append cfmask"
        sys.exit(ERROR)

    print "Success running append cfmask"
    sys.exit(SUCCESS)

