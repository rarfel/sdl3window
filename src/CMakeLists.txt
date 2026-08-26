add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE 
                              main.cpp
                              window.cpp
                              )

add_subdirectory(headers)
target_link_libraries(${PROJECT_NAME} PRIVATE headers)
target_link_libraries(${PROJECT_NAME} PRIVATE vendor)
