#ifndef TAFFO_TRANSFORMS_FIRTOMLIR_PASS_H
#define TAFFO_TRANSFORMS_FIRTOMLIR_PASS_H

#define DEBUG_TYPE "fir-to-mlir"

#include "mlir/Pass/Pass.h"
namespace mlir::taffo {
#define GEN_PASS_DECL_FIRTOMLIRPASS
#include "Taffo/Transforms/Passes.h.inc"
} // namespace mlir::taffo

#endif // TAFFO_TRANSFORMS_FIRTOMLIR_PASS_H
