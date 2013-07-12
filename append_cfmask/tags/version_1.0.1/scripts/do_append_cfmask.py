#! /usr/bin/env python
import sys
import os
import re
import commands
import datetime
from optparse import OptionParser

ERROR = 1
SUCCESS = 0

############################################################################
# Description: logIt logs the information to the logfile (if valid) or to
# stdout if the logfile is None.
#
# Inputs:
#   msg - message to be printed/logged
#   log_handler - log file handler; if None then print to stdout
#
# Returns: nothing
#
# Notes:
############################################################################
def logIt (msg, log_handler):
    if log_handler == None:
        print msg
    else:
        log_handler.write (msg + '\n')


#############################################################################
# Created on June 6, 2013 by Gail Schmidt, USGS/EROS
# Created Python script to run the append cfmask algorithm.
#
# History:
#
# Usage: do_append_cfmask.py --help prints the help message
############################################################################
class AppendCfmask():

    def __init__(self):
        pass


    ########################################################################
    # Description: runAppend will use the parameters passed for the input
    # files, logfile, and usebin.  If input files are None (i.e. not
    # specified) then the command-line parameters will be parsed for this
    # information.  The append cfmask application is then executed to append
    # the cfmask band to the specified input surface reflectance file.  If a
    # log file was specified, then the output from this application will be
    # logged to that file.
    #
    # Inputs:
    #   sr_infile - name of the input reflectance HDF file to be processed
    #   cfmask_infile - name of the input cfmask HDF file to be processed
    #   logfile - name of the logfile for logging information; if None then
    #       the output will be written to stdout
    #   usebin - this specifies if the spectral indices exe resides in the
    #       $BIN directory; if None then the spectral indices exe is
    #       expected to be in the PATH
    #
    # Returns:
    #     ERROR - error running the append cfmask application
    #     SUCCESS - successful processing
    #
    # Notes:
    #   1. If the reflectance file is not specified and the information is
    #      going to be grabbed from the command line, then it's assumed all
    #      the parameters will be pulled from the command line.
    #######################################################################
    def runAppend (self, sr_infile=None, cfmask_infile=None, logfile=None, \
        usebin=None):
        # if no parameters were passed then get the info from the
        # command line
        if sr_infile == None:
            # get the command line argument for the reflectance file
            parser = OptionParser()
            parser.add_option ("-s", "--sr_infile", type="string",
                dest="sr_infile",
                help="name of surface reflectance HDF file", metavar="FILE")
            parser.add_option ("-c", "--cfmask_infile", type="string",
                dest="cfmask_infile",
                help="name of cfmask HDF file", metavar="FILE")
            parser.add_option ("--usebin", dest="usebin", default=False,
                action="store_true",
                help="use BIN environment variable as the location of append cfmask application")
            parser.add_option ("-l", "--logfile", type="string", dest="logfile",
                help="name of optional log file", metavar="FILE")
            (options, args) = parser.parse_args()
    
            # validate the command-line options
            usebin = options.usebin          # should $BIN directory be used
            logfile = options.logfile        # name of the log file

            # surface reflectance file
            sr_infile = options.sr_infile
            if sr_infile == None:
                parser.error ("missing reflectance input file command-line argument");
                return ERROR

            # cfmask file
            cfmask_infile = options.cfmask_infile
            if sr_infile == None:
                parser.error ("missing cfmask input file command-line argument");
                return ERROR

        # open the log file if it exists; use line buffering for the output
        log_handler = None
        if logfile != None:
            log_handler = open (logfile, 'w', buffering=1)
        msg = 'Appending cfmask band from %s to Landsat surface reflectance ' \
            'file: %s' % (cfmask_infile, sr_infile)
        logIt (msg, log_handler)
        
        # should we expect the append cfmask application to be in the PATH
        # or in the BIN directory?
        if usebin:
            # get the BIN dir environment variable
            bin_dir = os.environ.get('BIN')
            bin_dir = bin_dir + '/'
            msg = 'BIN environment variable: %s' % bin_dir
            logIt (msg, log_handler)
        else:
            # don't use a path to the append cfmask application
            bin_dir = ""
            msg = 'append cfmask executable expected to be in the PATH'
            logIt (msg, log_handler)
        
        # make sure the reflectance file exists
        if not os.path.isfile(sr_infile):
            msg = "Error: reflectance file does not exist or is not " \
                "accessible: " + sr_infile
            logIt (msg, log_handler)
            return ERROR

        # make sure the cfmask file exists
        if not os.path.isfile(cfmask_infile):
            msg = "Error: cfmask file does not exist or is not accessible: "  \
                + cfmask_infile
            logIt (msg, log_handler)
            return ERROR

        # run append cfmask algorithm, checking the return status.  exit
        # if any errors occur.
        cmdstr = "%sappend_cfmask -sr %s -cfmask %s" %   \
            (bin_dir, sr_infile, cfmask_infile)
        print 'DEBUG: append_cfmask command: %s' % cmdstr
        (status, output) = commands.getstatusoutput (cmdstr)
        logIt (output, log_handler)
        exit_code = status >> 8
        if exit_code != 0:
            msg = 'Error running cfmask_append.  Processing will terminate.'
            logIt (msg, log_handler)
            return ERROR
        
        # successful completion.
        msg = 'Completion of append cfmask.'
        logIt (msg, log_handler)
        if logfile != None:
            log_handler.close()
        return SUCCESS

######end of AppendCfmask class######

if __name__ == "__main__":
    sys.exit (AppendCfmask().runAppend())
