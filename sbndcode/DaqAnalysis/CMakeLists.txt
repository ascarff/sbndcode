set (trace_product_version v3_06_06)
find_ups_product( TRACE v3_06_06 )
find_ups_product( lardataobj v1_17_00 )


# setup external non-ups libraries
include_directories($ENV{HIREDIS_INC})
link_directories($ENV{HIREDIS_LIB})
include_directories($ENV{JSONCPP_INC})
link_directories($ENV{JSONCPP_LIB})

# include Lariat Geo
add_subdirectory(LariatGeo)

# include Lariat Data PRoducts
add_subdirectory(LariatDataProducts)

# add in VST Calibration
add_subdirectory(VSTCalibration)

art_dictionary(DICTIONARY_LIBRARIES daqAnalysis_VST)

cet_make_library( LIBRARY_NAME daqAnalysis_MODE
	SOURCE
		Mode.cc
)

cet_make_library( LIBRARY_NAME daqAnalysis_VST
	SOURCE  Analysis.cc
		FFT.cc
		Noise.cc
		PeakFinder.cc
		Redis.cc
		ChannelData.cc
	LIBRARIES
		daqAnalysis_MODE
		jsoncpp
		hiredis
		sbnddaq-datatypes_Overlays
		sbnddaq-datatypes_NevisTPC
		${LARDATAOBJ} 
		lardataobj_RawData
		lardata_Utilities
        	${ART_FRAMEWORK_IO_SOURCES}
       		${ART_FRAMEWORK_PRINCIPAL}
       		canvas
	        ${ART_FRAMEWORK_SERVICES_REGISTRY}
		${ART_FRAMEWORK_SERVICES_OPTIONAL_TFILESERVICE_SERVICE}
		art_Framework_Services_Optional_TFileService_service
		art_Framework_Services_FileServiceInterfaces
		${ART_UTILITIES}
		${FHICLCPP}
		${ART_FRAMEWORK_CORE}
		${ART_FRAMEWORK_PRINCIPAL}
		${ROOT_BASIC_LIB_LIST} 
		fftw3
                           larcore_Geometry_Geometry_service
                           lardataobj_Simulation
                           lardata_Utilities
                           lardataobj_RawData
                           lardata_DetectorInfoServices_DetectorClocksServiceStandard_service
                           sbndcode_Utilities_SignalShapingServiceSBND_service
                           nutools_RandomUtils_NuRandomService_service
                           ${ART_FRAMEWORK_CORE}
                           ${ART_FRAMEWORK_PRINCIPAL}
                           ${ART_FRAMEWORK_SERVICES_OPTIONAL_TFILESERVICE_SERVICE}
                           ${ART_FRAMEWORK_SERVICES_OPTIONAL_RANDOMNUMBERGENERATOR_SERVICE}
                           canvas
                           ${MF_MESSAGELOGGER}
                           ${MF_UTILITIES}
                           ${FHICLCPP}
                           cetlib cetlib_except
                           ${CLHEP}
                           ${ROOT_BASIC_LIB_LIST}
		)

simple_plugin( SimpleDaqAnalysis module
  daqAnalysis_VST
  ${LARDATAOBJ}
  lardataobj_RawData
  ${ART_UTILITIES}
  ${FHICLCPP}
  ${ART_FRAMEWORK_CORE}
  ${ROOT_BASIC_LIB_LIST}
  ${ART_FRAMEWORK_SERVICES_OPTIONAL_TFILESERVICE_SERVICE}
)

simple_plugin( DaqDecoder module
  daqAnalysis_MODE
  sbnddaq-datatypes_Overlays 
  sbnddaq-datatypes_NevisTPC
  ${LARDATAOBJ}
  lardataobj_RawData
  ${ART_UTILITIES}
  ${FHICLCPP}
  ${ART_FRAMEWORK_CORE}
  ${ROOT_BASIC_LIB_LIST}
)

install_headers()
install_fhicl()
install_source()

