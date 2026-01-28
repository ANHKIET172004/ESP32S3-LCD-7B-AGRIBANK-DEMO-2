file(GLOB_RECURSE srcs *.c
                   ui/*.c
                   ui/components/*.c
                   ui/screens/*.c
                   user/wifi/*.c
                   esp_mqtt_client/*.c
                   init_handle/*.c
                   )
set(include_dirs 
   . 
   ui
   user/wifi
   esp_mqtt_client
   init_handle
                     
   )
idf_component_register( SRCS ${srcs} INCLUDE_DIRS ${include_dirs})

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)
#target_include_directories(${COMPONENT_LIB} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})//


