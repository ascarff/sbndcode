################################################################################
###  CMakeLists.txt for gallery analysis executables
################################################################################
###
###  Usage:
###  1) edit the EXECUTABLE section adding executable names ("targets"),
###     the header paths they need, and the libraries they link to
###  2) create a build directory and run `cmake` from there:
###      
###      mkdir -p build
###      cd build
###      cmake ..
###      make
###      
###
################################################################################
cmake_minimum_required(VERSION 2.8)
project(galleryAnalysis CXX)

# install in the same directory as the source
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR})

#
# include directories
#
include_directories($ENV{BOOST_INC})
include_directories($ENV{ROOT_INC} $ENV{CLHEP_INC})
include_directories($ENV{CETLIB_INC} $ENV{CETLIB_EXCEPT_INC} $ENV{CANVAS_INC})
include_directories($ENV{FHICLCPP_INC} $ENV{MESSAGEFACILITY_INC})
include_directories($ENV{GALLERY_INC})
include_directories($ENV{NUSIMDATA_INC} $ENV{LARCOREOBJ_INC} $ENV{LARDATAOBJ_INC})
include_directories($ENV{LARCOREALG_INC} $ENV{LARDATA_INC})

include_directories($ENV{SBNDCODE_INC})

#
# link directories and libraries
#
set(ROOT_LIBDIRS $ENV{ROOTSYS}/lib )
set(GALLERY_LIBDIRS ${ROOT_LIBDIRS} $ENV{CANVAS_LIB} $ENV{CETLIB_EXCEPT_LIB} $ENV{CETLIB_LIB} $ENV{GALLERY_LIB})
set(LARSOFTOBJ_LIBDIRS $ENV{NUSIMDATA_LIB} $ENV{LARCOREOBJ_LIB} $ENV{LARDATAOBJ_LIB})
set(LARSOFT_LIBDIRS $ENV{CLHEP_LIB_DIR} $ENV{LARCOREALG_LIB} $ENV{LARDATA_LIB})
set(SBNDCODE_LIBDIRS $ENV{SBNDCODE_LIB})

link_directories(${GALLERY_LIBDIRS} ${LARSOFTOBJ_LIBDIRS} ${LARSOFT_LIBDIRS} ${SBNDCODE_LIBDIRS})

set(GALLERY_LIBS canvas cetlib_except cetlib gallery)
set(ROOT_CORELIBS Core RIO Net Hist Graf Graf3d Gpad Tree Rint Postscript Matrix Physics MathCore Thread MultiProc pthread)
set(LARSOFTOBJ_LIBS nusimdata_SimulationBase larcoreobj_SummaryData lardataobj_RawData lardataobj_OpticalDetectorData lardataobj_RecoBase lardataobj_AnalysisBase lardataobj_MCBase lardataobj_Simulation)

set(SBNDGEOMETRY_LIBS larcorealg_Geometry)
set(SBNDDETINFO_LIBS lardata_DetectorInfo)

################################################################################
###  EXECUTABLE DIRECTIVES
################################################################################

add_executable(galleryAnalysis galleryAnalysis.cpp)
set_property(TARGET galleryAnalysis PROPERTY CXX_STANDARD 14)
target_link_libraries(galleryAnalysis
  ${ROOT_CORELIBS} ${GALLERY_LIBS} ${LARSOFTOBJ_LIBS}
  ${SBNDGEOMETRY_LIBS} # if using geometry
  ${SBNDDETINFO_LIBS}  # if using LAr properties, detector clocks or detector properties
  )
install(TARGETS galleryAnalysis DESTINATION .)