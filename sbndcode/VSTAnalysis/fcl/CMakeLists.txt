install_fhicl()

FILE(GLOB fcl_files *.fcl)
install_source(EXTRAS  ${fcl_files} )

# Uncomment these lines if you want 
# the various fcl configurations available globally

add_subdirectory(DAQ)
add_subdirectory(analysis_example)
add_subdirectory(pipeline_example)
add_subdirectory(redis_example)
