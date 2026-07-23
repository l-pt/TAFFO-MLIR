#include "flang/Optimizer/Dialect/FIRDialect.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/InitAllDialects.h"
#include "mlir/InitAllPasses.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Support/FileUtilities.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/ToolOutputFile.h"

#include "Taffo/Dialect/Taffo.h"
#include "Taffo/Transforms/Passes.h"

int main(int argc, char **argv) {
  mlir::registerAllPasses();

  mlir::DialectRegistry registry;
  mlir::registerAllDialects(registry);
  registry.insert<mlir::taffo::TaffoDialect>();
  registry.insert<fir::FIROpsDialect>();

  mlir::taffo::registerTaffoPasses();

  return mlir::asMainReturnCode(
      mlir::MlirOptMain(argc, argv, "Taffo optimizer driver\n", registry));
}
