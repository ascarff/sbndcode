set (trace_product_version v3_06_06)
find_ups_product( TRACE v3_06_06 )

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

