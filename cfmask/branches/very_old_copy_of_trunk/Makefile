# Set the HDF4 environment variables
HDFINC=/home/gschmidt/hdf-4.2.5/include
HDFLIB=/home/gschmidt/hdf-4.2.5/lib

# Set the HDF_EOS environment variables
HDFEOS_INC=/home/gschmidt/hdfeos2.17v1.00/hdfeos/include
HDFEOS_LIB=/home/gschmidt/hdfeos2.17v1.00/hdfeos/lib

# Set the GCTP environment variables (as delivered with HDF-EOS)
GCTPINC=/home/gschmidt/hdfeos2.17v1.00/hdfeos/gctp/include
GCTPLIB=/home/gschmidt/hdfeos2.17v1.00/hdfeos/lib

# Set the SZIP environment variables
SZIPINC=/home/gschmidt/szip-2.1/include
SZIPLIB=/home/gschmidt/szip-2.1/lib

# Set the ZLIB environment variables
ZLIBINC=/home/gschmidt/zlib-1.2.7/include
ZLIBLIB=/home/gschmidt/zlib-1.2.7/lib

# Set the JPEG environment variables
JPEGINC=/home/gschmidt/jpeg-6b/include
JPEGLIB=/home/gschmidt/jpeg-6b/lib

# Set the openCV environment variables
OPENCVLIB=/opt/cots/opencv-2.4.3/lib
OPENCVINC=/opt/cots/opencv-2.4.3/include/opencv

# Set the cvblob environment variables
CVBLOBLIB=/opt/cots/cvblob-0.10.4/lib
CVBLOBINC=/opt/cots/cvblob-0.10.4/include

#Add openCV cvBlobslib
CVBLOBSLIB=/opt/cots/cvblobs-8.3/lib
CVBLOBSINC=/opt/cots/cvblobs-8.3/include

# Set up compile options
CC	= gcc
RM	= rm -f
MV	= mv
EXTRA = -g -O -Wall 

# Define the include files
INC = const.h date.h error.h mystring.h input.h myhdf.h ias_misc_2d_array.h ias_const.h ias_logging.h ias_types.h cfmask.h
INCDIR  = -I. -I$(HDFINC) -I$(HDFEOS_INC) -I$(GCTPINC) -I$(JPEGINC) -I$(SZIPINC) -I$(ZLIBINC) -I$(OPENCVINC) -I$(CVBLOBINC) -I$(CVBLOBSINC)
NCFLAGS = $(EXTRA) $(INCDIR)

# Define the source code and object files
SRC = date.c error.c input.c myhdf.c mystring.c ias_misc_2d_array.c cfmask.c ias_logging.c potential_cloud_shadow_snow_mask.c ias_misc_write_envi_header.c ias_misc_split_filename.c object_cloud_shadow_match.c ias_misc.c
OBJ = $(SRC:.c=.o)

# Define the object libraries
LIB   = -L$(HDFLIB) -lmfhdf -ldf -L$(ZLIBLIB) -lz \
        -L$(JPEGLIB) -ljpeg -L$(HDFEOS_LIB) \
	-lxdr -lm -lstdc++
EOSLIB = -L$(HDFEOS_LIB) -lhdfeos -L$(GCTPLIB) -lGctp -L$(CVBLOBSLIB) -lblob

CVLIB = -L$(OPENCVLIB) -L$(CVBLOBLIB) -lopencv_core -lopencv_imgproc \
	-lopencv_calib3d -lopencv_contrib -lopencv_features2d -lopencv_flann \
	-lopencv_gpu -lopencv_highgui -lopencv_legacy -lopencv_ml \
	-lopencv_nonfree -lopencv_objdetect -lopencv_photo -lopencv_ts \
	-lcvblob

# Define the executable
EXE = cfmask

# Target for the executable
all: $(EXE)

cfmask: $(OBJ) $(INC)
	$(CC) -o cfmask $(OBJ) $(CVLIB) $(LIB) $(EOSLIB) 

install:
#	$(MV) $(EXE) $(BINDIR)/

clean:
	$(RM) *.o

$(OBJ): $(INC)

.c.o:
	$(CC) $(NCFLAGS) $(INCDIR) -c $<

