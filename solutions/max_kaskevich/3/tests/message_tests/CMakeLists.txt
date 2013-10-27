
compile_project( ${tests_name} "*.cpp" "*.h" BINARY tests ${module_name} )
register_test( ${tests_name} 0.1 0.2 )

