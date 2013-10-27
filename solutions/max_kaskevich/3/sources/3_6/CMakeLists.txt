find_package( Boost ${SEARCH_PARAMETERS} COMPONENTS
	system
	date_time
	thread
	filesystem
	chrono
	regex
)

compile_project( ${module_name} "*.cpp" "*.h" BINARY binaries Boost message Boost)
