set(PACKAGES
    google_
    )

foreach(lib_package ${PACKAGES})
  add_subdirectory(${lib_package})
endforeach()
