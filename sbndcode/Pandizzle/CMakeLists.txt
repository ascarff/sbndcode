#include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
#include_directories( $ENV{NUTOOLS_INC} )

art_make(
    EXCLUDE
        PandizzleTreeMaker_module.cc
    LIB_LIBRARIES
        lardataobj_RawData
        lardataobj_RecoBase
        lardata_RecoObjects  
        larpandora_LArPandoraInterface
        ${ART_FRAMEWORK_CORE}
        ${ART_FRAMEWORK_PRINCIPAL}
        ${ART_FRAMEWORK_SERVICES_REGISTRY}
        ${ART_FRAMEWORK_SERVICES_OPTIONAL}
        ${ART_FRAMEWORK_SERVICES_OPTIONAL_TFILESERVICE_SERVICE}
        art_Persistency_Common canvas
        art_Persistency_Provenance canvas
        art_Utilities canvas
        ${MF_MESSAGELOGGER}
        ${MF_UTILITIES}
        ${FHICLCPP}
        ${ROOT_GEOM}
        ${ROOT_XMLIO}
        ${ROOT_GDML}
        ${ROOT_BASIC_LIB_LIST}
        sbndcode_RecoUtils

)



simple_plugin( PandizzleTreeMaker module
        larcore_Geometry_Geometry_service
        larsim_Simulation lardataobj_Simulation
        larsim_MCCheater_BackTrackerService_service
        larsim_MCCheater_ParticleInventoryService_service
        lardata_Utilities
        larevt_Filters
        lardataobj_RawData
        lardataobj_RecoBase
        lardata_RecoObjects  
        larpandora_LArPandoraInterface
        nusimdata_SimulationBase
        ${ART_FRAMEWORK_CORE}
        ${ART_FRAMEWORK_PRINCIPAL}
        ${ART_FRAMEWORK_SERVICES_REGISTRY}
        ${ART_FRAMEWORK_SERVICES_OPTIONAL}
        ${ART_FRAMEWORK_SERVICES_OPTIONAL_TFILESERVICE_SERVICE}
        art_Persistency_Common canvas
        art_Persistency_Provenance canvas
        art_Utilities canvas
        ${MF_MESSAGELOGGER}
        ${MF_UTILITIES}
        ${FHICLCPP}
        ${ROOT_GEOM}
        ${ROOT_XMLIO}
        ${ROOT_GDML}
        ${ROOT_BASIC_LIB_LIST}
        sbndcode_RecoUtils
        sbndcode_Pandizzle
)

install_headers()
install_fhicl()
install_source()

