#ifndef TAFFO_TRANSFORMS_FIRALLOCATABLESANALYSIS_PASS_H
#define TAFFO_TRANSFORMS_FIRALLOCATABLESANALYSIS_PASS_H

#define DEBUG_TYPE "fir-allocatables-analysis"

#include "mlir/Pass/Pass.h"
namespace mlir::taffo {
#define GEN_PASS_DECL_FIRALLOCATABLESANALYSISPASS
#include "Taffo/Transforms/Passes.h.inc"
} // namespace mlir::taffo

#endif // TAFFO_TRANSFORMS_FIRALLOCATABLESANALYSIS_PASS_H
