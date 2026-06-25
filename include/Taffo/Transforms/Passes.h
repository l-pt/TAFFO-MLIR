#ifndef TAFFO_TRANSFORMS_PASSES_H
#define TAFFO_TRANSFORMS_PASSES_H

#include "Taffo/Transforms/DatatypeOptimizationPass.h"
#include "Taffo/Transforms/LowerToArithPass.h"
#include "Taffo/Transforms/RaiseToTaffoPass.h"
#include "Taffo/Transforms/FIRToMlirPass.h"
#include "Taffo/Transforms/ValueRangeAnalysisPass.h"

namespace mlir::taffo {
#define GEN_PASS_REGISTRATION
#include "Taffo/Transforms/Passes.h.inc"
} // namespace mlir::taffo

#endif // TAFFO_TRANSFORMS_PASSES_H
