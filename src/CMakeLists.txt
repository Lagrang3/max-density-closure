add_library(clusterlinearize 
        clusterlinearize.cpp)
target_include_directories(clusterlinearize INTERFACE "${CMAKE_SOURCE_DIR}/src")
set_target_properties(clusterlinearize
        PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/src"
)
