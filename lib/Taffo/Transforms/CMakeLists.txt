add_mlir_dialect_library(MLIRTaffoTransforms
        TaffoRangeCommon.cpp
        NtvRangeAnalysis.cpp
        AffineRangeAnalysis.cpp
        ValueRangeAnalysisPass.cpp
        LowerToArithPass.cpp
        DatatypeOptimizationPass.cpp
        libaffine.cpp
        RaiseToTaffoPass.cpp
        FIRToMlirPass.cpp
        #ADDITIONAL_HEADER_DIRS
        #${PROJECT_SOURCE_DIR}/include/Taffo

        DEPENDS
        MLIRTaffoTransformsIncGen

        LINK_COMPONENTS
        Core

        LINK_LIBS PUBLIC
        MLIRTaffo
        MLIRIR
        FIRDialect
        FIRSupport
)
