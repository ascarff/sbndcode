art_make(
  MODULE_LIBRARIES larcorealg_Geometry
  larcore_Geometry_Geometry_service
  lardata_Utilities
  larevt_Filters
  lardataobj_RawData
  lardataobj_RecoBase
  lardata_ArtDataHelper
  lardataobj_RecoBase
  lardataobj_AnalysisBase
  lardata_Utilities
  larpandora_LArPandoraInterface
  larreco_Calorimetry
  larreco_RecoAlg
  larreco_RecoAlg_ClusterRecoUtil
  larreco_ClusterFinder
  nusimdata_SimulationBase
  ${LARRECO_LIB}
  ${LARDATA_LIB}
  ${ART_FRAMEWORK_CORE}
  ${ART_FRAMEWORK_PRINCIPAL}
  ${ART_FRAMEWORK_SERVICES_REGISTRY}
  ${ART_FRAMEWORK_SERVICES_BASIC}
  ${ART_FRAMEWORK_SERVICES_OPTIONAL}
  ${ART_FRAMEWORK_SERVICES_OPTIONAL_TFILESERVICE_SERVICE}
  ${ART_FRAMEWORK_SERVICES_OPTIONAL_RANDOMNUMBERGENERATOR}
  art_Persistency_Common canvas
  art_Persistency_Provenance canvas
  art_Utilities canvas
  ${MF_MESSAGELOGGER}
  ${MF_UTILITIES}
  ${FHICLCPP}
  cetlib cetlib_except
  ${CLHEP}
  ${ROOT_GEOM}
  ${ROOT_XMLIO}
  ${ROOT_GDML}
  ${ROOT_BASIC_LIB_LIST}
  ${ROOT_SPECTRUM}
  ${ROOT_ROOFIT}
  ${ROOT_ROOFITCORE}
  sbndcode_RecoUtils
  sbndcode_OpDetSim
  )
install_headers()
install_fhicl()
install_source()
