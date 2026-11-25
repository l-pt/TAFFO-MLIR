


get_property(dialect_libs GLOBAL PROPERTY MLIR_DIALECT_LIBS)
get_property(conversion_libs GLOBAL PROPERTY MLIR_CONVERSION_LIBS)
set(LIBS
        ${dialect_libs}
        ${conversion_libs}
        MLIROptLib
        MLIRTaffo
        FIRTransforms
        )
add_llvm_executable(taffo-opt taffo-opt.cpp)

llvm_update_compile_flags(taffo-opt)
target_link_libraries(taffo-opt PRIVATE ${LIBS})

mlir_check_all_link_libraries(taffo-opt)
