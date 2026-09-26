add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE 
                              main.cpp
                              window.cpp
                              imguiMenu.cpp
                              # Adding Imgui Dependencies
                              ../vendor/imgui/imgui.cpp
                              ../vendor/imgui/imgui_draw.cpp
                              ../vendor/imgui/imgui_demo.cpp

                              ../vendor/imgui/imgui_tables.cpp
                              ../vendor/imgui/imgui_widgets.cpp

                              ../vendor/imgui/imgui_impl_sdl3.cpp
                              # SdlRenderer specific
                              ../vendor/imgui/imgui_impl_sdlrenderer3.cpp
                              # Vulkan specific
                              #../vendor/imgui/imgui_impl_vulkan.cpp
                              )

add_subdirectory(headers)
target_link_libraries(${PROJECT_NAME} PRIVATE headers)
target_link_libraries(${PROJECT_NAME} PRIVATE vendor)
