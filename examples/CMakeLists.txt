add_executable(maxfeerate-fp maxfeerate-fp.cpp)
target_link_libraries(maxfeerate-fp clusterlinearize)

add_executable(maxfeerate-bf maxfeerate-bf.cpp)
target_link_libraries(maxfeerate-bf clusterlinearize)

add_executable(maxfeerate-validate maxfeerate-validate.cpp)
target_link_libraries(maxfeerate-validate clusterlinearize)

add_executable(kattis-maxflow kattis-maxflow.cpp)
