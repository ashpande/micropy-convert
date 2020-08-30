set(LLVM_LINK_COMPONENTS support)

add_clang_executable(micropy-convert
	micropyconvert.cpp
	)
target_link_libraries(micropy-convert
	PRIVATE
	clangTooling
	clangBasic
	clangASTMatchers
	)
