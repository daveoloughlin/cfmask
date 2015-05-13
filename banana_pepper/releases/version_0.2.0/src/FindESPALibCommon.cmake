
find_path ( LibESPA_INCLUDE_DIR
  NAMES
    espa_common.h
  PATHS
    ENV ESPAINC
)

find_library ( LibEspaRawBinary_LIBRARY
  NAMES
    lib_espa_raw_binary.a
  PATHS
    ENV ESPALIB
)

find_library ( LibEspaCommon_LIBRARY
  NAMES
    lib_espa_common.a
  PATHS
    ENV ESPALIB
)

find_library ( LibEspaFormatConversion_LIBRARY
  NAMES
    lib_espa_format_conversion.a
  PATHS
    ENV ESPALIB
)

# TODO TODO TODO - Needs to be made smarter to report not finding the library

set ( LibESPA_INCLUDES
        ${LibESPA_INCLUDE_DIR} )
set ( LibESPA_LIBRARIES
        ${LibEspaRawBinary_LIBRARY}
        ${LibEspaCommon_LIBRARY}
        ${LibEspaFormatConversion_LIBRARY} )

if ( LibESPA_INCLUDES )
  message( STATUS "Found ESPA Common Include Directory: ${LibESPA_INCLUDES}" )
endif ( LibESPA_INCLUDES )

if ( LibESPA_LIBRARIES )
  message( STATUS "Found ESPA Common Libraries: "
        ${LibEspaRawBinary_LIBRARY} " "
        ${LibEspaCommon_LIBRARY} " "
        ${LibEspaFormatConversion_LIBRARY} )
endif ( LibESPA_LIBRARIES )

