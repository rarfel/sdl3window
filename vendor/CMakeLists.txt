add_subdirectory(SDL)
add_subdirectory(glm)

find_package(Vulkan REQUIRED COMPONENTS volk shaderc_combined)

add_library(vendor INTERFACE)
target_link_libraries(vendor INTERFACE 
                              Vulkan::Vulkan
                              Vulkan::volk
                              Vulkan::shaderc_combined
                              SDL3::SDL3 
                              glm::glm
                            )
