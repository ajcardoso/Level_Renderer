cmake_minimum_required(VERSION 3.16)

project(Assignment_2_D3D11)

# CMake FXC shader compilation, add any shaders you want compiled here
set(VERTEX_SHADERS 
	# add vertex shader (.hlsl) files here
	Shaders/VertexShader.hlsl
)

set(PIXEL_SHADERS 
	# add pixel shader (.hlsl) files here
	Shaders/PixelShader.hlsl
)

# Add any new C/C++ source code here
set(SOURCE_CODE
	# Header & CPP files go here
	main.cpp
	h2bParser.h
	load_object_oriented.h
	renderer.h
	FileIntoString.h
	#TODO: Part 1B (optional)
)

if(WIN32)
# by default CMake selects "ALL_BUILD" as the startup project 
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
	PROPERTY VS_STARTUP_PROJECT Assignment_2_D3D11)

	#this will tell Visual Studio to put the shaders under their own group in the solution explorer 
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
   	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${VERTEX_SHADERS})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PIXEL_SHADERS})	
endif()

# currently using unicode in some libraries on win32 but will change soon
ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)



add_executable (Assignment_2_D3D11 
	main.cpp
	h2bParser.h
	load_object_oriented.h
	renderer.h
	${SOURCE_CODE}
	${VERTEX_SHADERS}
	${PIXEL_SHADERS}

)

set_source_files_properties( ${VERTEX_SHADERS} PROPERTIES 
        VS_SHADER_TYPE Vertex 
        VS_SHADER_MODEL 5.0
        VS_SHADER_ENTRYPOINT main
        VS_TOOL_OVERRIDE "FXCompile" 
)
set_source_files_properties( ${PIXEL_SHADERS} PROPERTIES 
	    VS_SHADER_TYPE Pixel 
        VS_SHADER_MODEL 5.0
        VS_SHADER_ENTRYPOINT main
        VS_TOOL_OVERRIDE "FXCompile"
)