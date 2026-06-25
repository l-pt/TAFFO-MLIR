#include "Taffo/Transforms/FIRToMlirPass.h"

#include "flang/Optimizer/Dialect/FIRDialect.h"
#include "flang/Optimizer/Dialect/FIROps.h"
#include "flang/Optimizer/Dialect/FIRType.h"
#include "flang/Optimizer/Dialect/Support/FIRContext.h"
#include "flang/Optimizer/Dialect/Support/KindMapping.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/BuiltinTypeInterfaces.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/ImplicitLocOpBuilder.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/IR/Types.h"
#include "mlir/Support/LLVM.h"
#include "llvm/Support/LogicalResult.h"
#include "mlir/Transforms/DialectConversion.h"

namespace mlir::taffo {
#define GEN_PASS_DEF_FIRTOMLIRPASS
#include "Taffo/Transforms/Passes.h.inc"
} // namespace mlir::taffo

using namespace ::mlir::taffo;

namespace mlir {
class FIRToMlirPass
    : public mlir::taffo::impl::FIRToMlirPassBase<FIRToMlirPass> {
public:
  using FIRToMlirPassBase::FIRToMlirPassBase;

  class FIRToMlirTypeConverter : public mlir::TypeConverter {
    static mlir::IntegerType::SignednessSemantics getSignednessSemantic(mlir::Type type) {
      if (type.isSignedInteger()) {
        return mlir::IntegerType::SignednessSemantics::Signed;
      }
      if (type.isUnsignedInteger()) {
        return mlir::IntegerType::SignednessSemantics::Unsigned;
      }
      return mlir::IntegerType::SignednessSemantics::Signless;
    }

  public:
    FIRToMlirTypeConverter(MLIRContext& ctx, Operation& op) {
      addConversion([](Type type) { return type; });

      addConversion([&ctx](fir::IntegerType type) {
          return mlir::IntegerType::get(&ctx, type.getIntOrFloatBitWidth(), getSignednessSemantic(type));
      });
      addConversion([&ctx, &op](fir::LogicalType type) {
          return mlir::IntegerType::get(&ctx, fir::getKindMapping(&op).getLogicalBitsize(type.getFKind()));
      });
      //TODO convert memref types
    }
  };

  struct RewriteAlloca : public OpConversionPattern<fir::AllocaOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::AllocaOp firAllocaOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      rewriter.startOpModification(firAllocaOp);

      ImplicitLocOpBuilder builder(firAllocaOp.getLoc(), rewriter);
      //TODO convert memref types
      auto memrefAllocaOp = builder.create<memref::AllocaOp>();

      rewriter.replaceOp(firAllocaOp, memrefAllocaOp);
      return success();
    }
  };

  void runOnOperation() override {
    MLIRContext& context = getContext();
    ConversionTarget target(context);

    target.addIllegalDialect<fir::FIROpsDialect>();
    target.addLegalDialect<arith::ArithDialect, func::FuncDialect, scf::SCFDialect, memref::MemRefDialect>();

    Operation *op = getOperation();
    FIRToMlirTypeConverter typeConverter(context, *op);

    RewritePatternSet patternSet(&context);
    patternSet.add<RewriteAlloca>(typeConverter, context);

    (void) applyFullConversion(op, target, std::move(patternSet));
  }
};
} // namespace mlir
