
# OpFlashFinder_module.cc is not built - it seems specific to uboone

#art_make_library( LIBRARY_NAME FlashHypothesis
#		  	     SOURCE FlashHypothesis.cxx
#		                           FlashHypothesisCalculator.cxx
#		                           FlashUtilities.cxx
#			     LIBRARIES
#			                             ${ROOT_BASIC_LIB_LIST}
#)

art_make_library( LIBRARY_NAME SimPhotonCounterOpLib
		  	     SOURCE SimPhotonCounterOpLib.cxx
			     LIBRARIES
			                ${ROOT_BASIC_LIB_LIST}
)

#art_make_library( LIBRARY_NAME FlashHypothesisComparison
#		  	     SOURCE FlashHypothesisComparison.cxx
#			     LIBRARIES
#					FlashHypothesis
#					SimPhotonCounter
#			                ${ROOT_BASIC_LIB_LIST}
#)

art_make( BASENAME_ONLY
          EXCLUDES
			 SimPhotonCounter.cxx
          LIBRARY_NAME OpticalDetectorOpLib
          LIB_LIBRARIES 
			SimPhotonCounter
			RecoBase
			MCBase
                        Geometry
                        Geometry_service
                       PhotonVisibilityServiceOpLib_service
#        	        OpDigiProperties_service
		        Utilities
                       ${SIMULATIONBASE}
                        ${ART_FRAMEWORK_CORE}
			${ART_FRAMEWORK_PRINCIPAL}
			${ART_FRAMEWORK_SERVICES_REGISTRY}
			${ART_FRAMEWORK_SERVICES_BASIC}
			${ART_FRAMEWORK_SERVICES_OPTIONAL}
			${ART_FRAMEWORK_SERVICES_OPTIONAL_TFILESERVICE_SERVICE}
                       ${ART_FRAMEWORK_SERVICES_OPTIONAL_RANDOMNUMBERGENERATOR_SERVICE}
			${ART_PERSISTENCY_COMMON}
			${ART_UTILITIES}
			${MF_MESSAGELOGGER}
			${MF_UTILITIES}
	                ${FHICLCPP}
			${CETLIB}
                        ${ROOT_BASIC_LIB_LIST}
          MODULE_LIBRARIES #OpDigiProperties_service
	                OpticalDetector
                        PhotonVisibilityServiceOpLib_service
			BackTracker_service
                        Filters
                        RecoObjects
                       RecoBase
			MCBase
                       RecoAlg
                        Geometry
                       Geometry_service
                        Simulation
#                        TriggerAlgoMicroBoone_service
                        Utilities
                        RawData
                        AnalysisAlg
                        AnalysisBase
                        ${SIMULATIONBASE}
                        ${ART_FRAMEWORK_CORE}
			${ART_FRAMEWORK_PRINCIPAL}
			${ART_FRAMEWORK_SERVICES_REGISTRY}
			${ART_FRAMEWORK_SERVICES_BASIC}
			${ART_FRAMEWORK_SERVICES_OPTIONAL}
			${ART_FRAMEWORK_SERVICES_OPTIONAL_TFILESERVICE_SERVICE}
                        ${ART_FRAMEWORK_SERVICES_OPTIONAL_RANDOMNUMBERGENERATOR_SERVICE}
			${ART_PERSISTENCY_COMMON}
			${ART_UTILITIES}
			${MF_MESSAGELOGGER}
			${MF_UTILITIES}
	                ${FHICLCPP}
			${CETLIB}
                        ${CLHEP}
                        ${ROOT_BASIC_LIB_LIST}
       SERVICE_LIBRARIES
                        Geometry
                        Geometry_service
                        ${ART_FRAMEWORK_CORE}
			${ART_FRAMEWORK_PRINCIPAL}
			${ART_FRAMEWORK_SERVICES_REGISTRY}
			${ART_FRAMEWORK_SERVICES_BASIC}
			${ART_FRAMEWORK_SERVICES_OPTIONAL}
			${ART_FRAMEWORK_SERVICES_OPTIONAL_TFILESERVICE_SERVICE}
			${ART_PERSISTENCY_COMMON}
			${ART_UTILITIES}
			${MF_MESSAGELOGGER}
			${MF_UTILITIES}
	                ${FHICLCPP}
			${CETLIB}
                        ${CLHEP}
                        ${ROOT_BASIC_LIB_LIST}
        )

install_headers()
install_fhicl()
install_source()
#install_fw( LIST toyWaveform.txt
#            SUBDIRNAME OpticalDetector )
