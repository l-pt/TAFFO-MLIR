#include "Taffo/Transforms/FIRAllocatablesAnalysis.h"

#include "flang/Optimizer/Dialect/FIRDialect.h"
#include "flang/Optimizer/Dialect/FIROps.h"
#include "flang/Optimizer/Dialect/FIRType.h"
#include "flang/Optimizer/Dialect/Support/FIRContext.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/ImplicitLocOpBuilder.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Support/LLVM.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"

namespace mlir::taffo {
#define GEN_PASS_DEF_FIRALLOCATABLESANALYSISPASS
#include "Taffo/Transforms/Passes.h.inc"
} // namespace mlir::taffo

using namespace ::mlir::taffo;

namespace mlir {
struct FIRAllocatablesAnalysisPass
    : public mlir::taffo::impl::FIRAllocatablesAnalysisPassBase<FIRAllocatablesAnalysisPass> {
  Operation *getOriginalShapeOp(fir::ShapeShiftOp shapeOp, const DenseMap<Value, Value>& loadMap) {
    auto candidates = DenseSet<Operation*>{};
    for (const auto& operand : shapeOp.getOperands()) {
      if (auto boxDimsOp = operand.getDefiningOp<fir::BoxDimsOp>()) {
        auto box = boxDimsOp.getVal();
        if (const auto& found = loadMap.find(box); found != loadMap.end()) {
          box = found->second;
        }
        if (auto emboxOp = box.getDefiningOp<fir::EmboxOp>()) {
          auto shape = emboxOp.getShape();
          if (auto shapeDefOp = shape.getDefiningOp<fir::ShapeOp>()) {
            LLVM_DEBUG(llvm::dbgs() << "Found original fir.shape " << shapeDefOp << "\n");
            candidates.insert(shapeDefOp);
          } else if (auto shapeDefOp = shape.getDefiningOp<fir::ShapeShiftOp>()) {
            LLVM_DEBUG(llvm::dbgs() << "Found original fir.shape_shift " << shapeDefOp << "\n");
            candidates.insert(shapeDefOp);
          }
        }
      }
    }

    if (candidates.size() == 0) {
      LLVM_DEBUG(llvm::dbgs() << "Original shape not found\n");
      return nullptr;
    }
    if (candidates.size() > 1) {
      LLVM_DEBUG(llvm::dbgs() << "Found multiple original shapes\n");
      return nullptr;
    }
    return *candidates.begin();
  }

  Operation *createShapeShift(Operation& op, Type resType, Location loc, ImplicitLocOpBuilder& builder) {
    auto guard = OpBuilder::InsertionGuard{builder};
    builder.setInsertionPoint(&op);

    if (auto shapeOp = llvm::dyn_cast<fir::ShapeOp>(op)) {
      auto one = builder.create<arith::ConstantIndexOp>(1);
      auto pairs = SmallVector<Value>{};
      for (const auto& extent : shapeOp.getOperands()) {
        pairs.push_back(one.getResult());
        pairs.push_back(extent);
      }
      return builder.create<fir::ShapeShiftOp>(resType, pairs);
    }
    if (auto shapeShiftOp = llvm::dyn_cast<fir::ShapeShiftOp>(op)) {
      return builder.create<fir::ShapeShiftOp>(loc, resType, shapeShiftOp.getPairs());
    }

    assert(false && "unreachable");
  }
 
  void runOnOperation() override {
    auto funcOp = getOperation();
    auto rewriter = IRRewriter{&getContext()};
    auto builder = ImplicitLocOpBuilder{funcOp.getLoc(), rewriter};

    for (auto& block : funcOp.getBody()) {
      auto storeMap = DenseMap<Value, Value>{};
      auto loadMap = DenseMap<Value, Value>{};

      for (auto it = block.begin(); it != block.end();) {
        auto* op = &(*(it++));
        if (auto storeOp = llvm::dyn_cast<fir::StoreOp>(op)) {
          auto addr = storeOp.getMemref();
          auto val = storeOp.getValue();
          storeMap[addr] = val;
          LLVM_DEBUG(llvm::dbgs() << "Added " << val << " to the store map\n");
          continue;
        }

        if (auto loadOp = llvm::dyn_cast<fir::LoadOp>(op)) {
          auto addr = loadOp.getMemref();
          if (const auto& found = storeMap.find(addr); found != storeMap.end()) {
            loadMap[loadOp.getResult()] = found->second;
            LLVM_DEBUG(llvm::dbgs() << "Added " << found->second << " to the load map\n");
          }
          continue;
        }

        if (auto boxAddrOp = llvm::dyn_cast<fir::BoxAddrOp>(op)) {
          auto box = boxAddrOp.getVal();
          if (const auto& found = loadMap.find(box); found != loadMap.end()) {
            box = found->second;
          }
          if (auto emboxOp = box.getDefiningOp<fir::EmboxOp>()) {
            auto addr = emboxOp.getMemref();
            rewriter.replaceOp(boxAddrOp, addr);
            LLVM_DEBUG(llvm::dbgs() << "Replaced fir.box_addr with address from fir.embox\n");
            continue;
          }
        }

        if (auto shapeShiftOp = llvm::dyn_cast<fir::ShapeShiftOp>(op)) {
          if (auto* origShapeOp = getOriginalShapeOp(shapeShiftOp, loadMap)) {
            auto* newOp = createShapeShift(*origShapeOp, shapeShiftOp.getResult().getType(), shapeShiftOp.getLoc(), builder);
            rewriter.replaceOp(shapeShiftOp, newOp);
            LLVM_DEBUG(llvm::dbgs() << "Replaced fir.shape_shift\n");
            continue;
          }
        }
      }
    }
  }
};
} // namespace mlir
