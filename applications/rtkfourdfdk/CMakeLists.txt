WRAP_GGO(rtkfourdfdk_GGO_C rtkfourdfdk.ggo ../rtkinputprojections_section.ggo ../rtk4Doutputimage_section.ggo ${RTK_BINARY_DIR}/rtkVersion.ggo)
add_executable(rtkfourdfdk rtkfourdfdk.cxx ${rtkfourdfdk_GGO_C})
target_link_libraries(rtkfourdfdk RTK)

# Installation code
if(NOT RTK_INSTALL_NO_EXECUTABLES)
  foreach(EXE_NAME rtkfourdfdk) 
    install(TARGETS ${EXE_NAME}
      RUNTIME DESTINATION ${RTK_INSTALL_RUNTIME_DIR} COMPONENT Runtime
      LIBRARY DESTINATION ${RTK_INSTALL_LIB_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${RTK_INSTALL_ARCHIVE_DIR} COMPONENT Development)
  endforeach() 
endif()

