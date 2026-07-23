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

  class FIRToMlirTypeConverter : public TypeConverter {
    static IntegerType::SignednessSemantics getSignednessSemantic(Type type) {
      if (type.isSignedInteger()) {
        return IntegerType::SignednessSemantics::Signed;
      }
      if (type.isUnsignedInteger()) {
        return IntegerType::SignednessSemantics::Unsigned;
      }
      return IntegerType::SignednessSemantics::Signless;
    }

  public:
    FIRToMlirTypeConverter(MLIRContext& ctx, Operation& op) {
      addConversion([](Type type) { return type; });

      addConversion([&ctx](fir::IntegerType type) {
          return IntegerType::get(&ctx, type.getIntOrFloatBitWidth(), getSignednessSemantic(type));
      });
      addConversion([&ctx, &op](fir::LogicalType type) {
          return IntegerType::get(&ctx, fir::getKindMapping(&op).getLogicalBitsize(type.getFKind()));
      });
      //TODO convert memref types
    }
  };

  struct RewriteAlloca : public OpConversionPattern<fir::AllocaOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::AllocaOp firAllocaOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      ImplicitLocOpBuilder builder(firAllocaOp.getLoc(), rewriter);
      //TODO convert memref types
      auto memrefAllocaOp = builder.create<memref::AllocaOp>(MemRefType::get({}, firAllocaOp.getAllocatedType()));
      rewriter.replaceOp(firAllocaOp, memrefAllocaOp);
      return success();
    }
  };

  //Convert fir.do_loop in scf.for
  struct RewriteDoLoop : public OpConversionPattern<fir::DoLoopOp> {
    using OpConversionPattern::OpConversionPattern;

    //See: https://mlir.llvm.org/docs/Dialects/SCFDialect/#scffor-scfforop
    //See: https://flang.llvm.org/docs/FIRLangRef.html#fir-do-loop-fir-doloopop
    LogicalResult matchAndRewrite(fir::DoLoopOp firDoLoopOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      ImplicitLocOpBuilder builder(firDoLoopOp.getLoc(), rewriter);

      Value firLb = firDoLoopOp.getLowerBound();
      Value firUb = firDoLoopOp.getUpperBound();

      //If the step is negative, swap lower and upper bound and use -step
      auto zeroConst = builder.create<arith::ConstantOp>(builder.getI32IntegerAttr(0)); //TODO check if we need different types
      auto isStepPositive = builder.create<arith::CmpIOp>(arith::CmpIPredicate::sgt, firDoLoopOp.getStep(), zeroConst);
      auto selectLowerBound = builder.create<arith::SelectOp>(isStepPositive.getResult(), firLb, firUb);
      auto selectUpperBound = builder.create<arith::SelectOp>(isStepPositive.getResult(), firUb, firLb);
      auto negStep = builder.create<arith::SubIOp>(zeroConst, firDoLoopOp.getStep());
      auto selectStep = builder.create<arith::SelectOp>(isStepPositive.getResult(), firDoLoopOp.getStep(), negStep.getResult());

      //NOTE: fir.do_loop has inclusive upper bound, scf.for does not
      auto realUpperBound = builder.create<arith::AddIOp>(selectUpperBound.getResult(), selectStep.getResult());
      auto forOp = builder.create<scf::ForOp>(selectLowerBound.getResult(),
          realUpperBound.getResult(),
          selectStep.getResult(),
          firDoLoopOp.getInitArgs());

      //Move the loop body, replace induction variable if necessary
      //delta = for_iv - original_lb
      //new_iv = original_ub - delta
      auto delta = builder.create<arith::SubIOp>(forOp.getInductionVar(), firLb);
      auto newIv = builder.create<arith::SubIOp>(firUb, delta.getResult());
      auto selectNewIv = builder.create<arith::SelectOp>(isStepPositive.getResult(), forOp.getInductionVar(), newIv.getResult());
      rewriter.mergeBlocks(firDoLoopOp.getBody(), forOp.getBody(), {selectNewIv.getResult()});

      //Replace fir.result with scf.yield
      Operation* firResultOp = forOp.getBody()->getTerminator();
      rewriter.replaceOpWithNewOp<scf::YieldOp>(firResultOp, firResultOp->getOperands());

      rewriter.replaceOp(firDoLoopOp, forOp);
      return success();
    }
  };

  struct RewriteIf : public OpConversionPattern<fir::IfOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::IfOp firIfOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      ImplicitLocOpBuilder builder(firIfOp.getLoc(), rewriter);

      bool hasElse = !firIfOp.getElseRegion().empty();
      auto ifOp = builder.create<scf::IfOp>(firIfOp.getResultTypes(), firIfOp.getCondition(), hasElse);

      //NOTE each block is terminated by a fir.result op, we have to convert it in a scf.yield op.
      //See: https://mlir.llvm.org/docs/Dialects/SCFDialect/#scfyield-scfyieldop
      //See: https://flang.llvm.org/docs/FIRLangRef.html#fir-result-fir-resultop
      Region& thenRegion = ifOp.getThenRegion();
      thenRegion.takeBody(firIfOp.getThenRegion());
      Operation *firResultOp = thenRegion.front().getTerminator();
      rewriter.replaceOpWithNewOp<scf::YieldOp>(firResultOp, firResultOp->getOperands());

      if (hasElse) {
        Region& elseRegion = ifOp.getElseRegion();
        elseRegion.takeBody(firIfOp.getElseRegion());
        Operation *firResultOp = elseRegion.front().getTerminator();
        rewriter.replaceOpWithNewOp<scf::YieldOp>(firResultOp, firResultOp->getOperands());
      }

      rewriter.replaceOp(firIfOp, ifOp);
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
    patternSet.add<RewriteAlloca>(typeConverter, &context);
    patternSet.add<RewriteIf>(typeConverter, &context);
    patternSet.add<RewriteDoLoop>(typeConverter, &context);

    (void) applyFullConversion(op, target, std::move(patternSet));
  }
};
} // namespace mlir
