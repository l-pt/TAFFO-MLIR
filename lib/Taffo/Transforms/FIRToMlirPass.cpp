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
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinDialect.h"
#include "mlir/IR/BuiltinTypeInterfaces.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/ImplicitLocOpBuilder.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/IR/TypeRange.h"
#include "mlir/IR/Types.h"
#include "mlir/IR/ValueRange.h"
#include "mlir/Support/LLVM.h"
#include "mlir/Transforms/DialectConversion.h"
#include "llvm/Support/LogicalResult.h"
#include <cstdlib>
#include <iostream>
#include <ostream>

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
      addConversion([](fir::ReferenceType type) {
        //FIXME shape
        return MemRefType::get({1}, type.getElementType()); //1xT
      });

      addSourceMaterialization([](OpBuilder& builder, Type resultType, ValueRange inputs, Location loc) -> Value {
        if (inputs.size() != 1) {
          return {};
        }
        return builder.create<memref::CastOp>(loc, resultType, inputs).getResult();
      });

      addTargetMaterialization([](OpBuilder& builder, Type resultType, ValueRange inputs, Location loc) -> Value {
        if (inputs.size() != 1) {
          return {};
        }
        return builder.create<memref::CastOp>(loc, resultType, inputs).getResult();
      });
    }
  };

  static SmallVector<Type> convTypes(const TypeConverter* converter, TypeRange in) {
    SmallVector<Type> out;
    if (failed(converter->convertTypes(in, out))) {
      std::cerr << "conversion error" << std::endl;
      std::abort();
    }
    return out;
  }

  struct RewriteDeclare : public OpConversionPattern<fir::DeclareOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::DeclareOp firDeclareOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      emitWarning(firDeclareOp.getLoc()) << "fir::declare shape " << firDeclareOp.getShape();
      emitWarning(firDeclareOp.getLoc()) << "fir::declare result " << firDeclareOp.getResult();
      return success();
    }
  };

  struct RewriteAlloca : public OpConversionPattern<fir::AllocaOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::AllocaOp firAllocaOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      ImplicitLocOpBuilder builder(firAllocaOp.getLoc(), rewriter);
      auto convertedType = getTypeConverter()->convertType(firAllocaOp.getAllocatedType());
      auto memrefAllocaOp = builder.create<memref::AllocaOp>(MemRefType::get({1}, convertedType)); //FIXME shape
      rewriter.replaceOp(firAllocaOp, memrefAllocaOp);
      return success();
    }
  };

  struct RewriteLoad : public OpConversionPattern<fir::LoadOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::LoadOp firLoadOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      ImplicitLocOpBuilder builder(firLoadOp.getLoc(), rewriter);
      auto zeroConst = builder.create<arith::ConstantOp>(builder.getI32IntegerAttr(0)); //FIXME correct index
      auto loadOp = builder.create<memref::LoadOp>(adaptor.getMemref(), ValueRange{zeroConst.getResult()});
      rewriter.replaceOp(firLoadOp, loadOp);
      return success();
    }
  };

  struct RewriteStore : public OpConversionPattern<fir::StoreOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::StoreOp firStoreOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      ImplicitLocOpBuilder builder(firStoreOp.getLoc(), rewriter);
      auto storeOp = builder.create<memref::StoreOp>(adaptor.getValue(), adaptor.getMemref());
      rewriter.replaceOp(firStoreOp, storeOp);
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

      Value firLb = adaptor.getLowerBound();
      Value firUb = adaptor.getUpperBound();

      //If the step is negative, swap lower and upper bound and use -step
      auto zeroConst = builder.create<arith::ConstantOp>(builder.getI32IntegerAttr(0)); //TODO check if we need different types
      auto isStepPositive = builder.create<arith::CmpIOp>(arith::CmpIPredicate::sgt, adaptor.getStep(), zeroConst);
      auto selectLowerBound = builder.create<arith::SelectOp>(isStepPositive.getResult(), firLb, firUb);
      auto selectUpperBound = builder.create<arith::SelectOp>(isStepPositive.getResult(), firUb, firLb);
      auto negStep = builder.create<arith::SubIOp>(zeroConst, adaptor.getStep());
      auto selectStep = builder.create<arith::SelectOp>(isStepPositive.getResult(), firDoLoopOp.getStep(), negStep.getResult());

      //NOTE fir.do_loop has inclusive upper bound, scf.for does not
      auto realUpperBound = builder.create<arith::AddIOp>(selectUpperBound.getResult(), selectStep.getResult());
      auto forOp = builder.create<scf::ForOp>(selectLowerBound.getResult(),
          realUpperBound.getResult(),
          selectStep.getResult(),
          adaptor.getInitArgs());

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

      bool hasElse = !adaptor.getElseRegion().empty();
      auto ifOp = builder.create<scf::IfOp>(convTypes(getTypeConverter(), firIfOp.getResultTypes()), adaptor.getCondition(), hasElse);

      //NOTE each block is terminated by a fir.result op, we have to convert it in a scf.yield op.
      //See: https://mlir.llvm.org/docs/Dialects/SCFDialect/#scfyield-scfyieldop
      //See: https://flang.llvm.org/docs/FIRLangRef.html#fir-result-fir-resultop
      Region& thenRegion = ifOp.getThenRegion();
      thenRegion.takeBody(adaptor.getThenRegion());
      Operation *firResultOp = thenRegion.front().getTerminator();
      rewriter.replaceOpWithNewOp<scf::YieldOp>(firResultOp, firResultOp->getOperands());

      if (hasElse) {
        Region& elseRegion = ifOp.getElseRegion();
        elseRegion.takeBody(adaptor.getElseRegion());
        Operation *firResultOp = elseRegion.front().getTerminator();
        rewriter.replaceOpWithNewOp<scf::YieldOp>(firResultOp, firResultOp->getOperands());
      }

      rewriter.replaceOp(firIfOp, ifOp);
      return success();
    }
  };

  //FIXME
  struct RewriteDummyScope : public OpConversionPattern<fir::DummyScopeOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::DummyScopeOp dummyScopeOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      rewriter.eraseOp(dummyScopeOp);
      return success();
    }
  };

  void runOnOperation() override {
    MLIRContext& context = getContext();
    ConversionTarget target(context);

    target.addIllegalDialect<fir::FIROpsDialect>();
    target.addLegalDialect<BuiltinDialect, arith::ArithDialect, func::FuncDialect, scf::SCFDialect, memref::MemRefDialect>();

    Operation *op = getOperation();
    FIRToMlirTypeConverter typeConverter(context, *op);

    RewritePatternSet patternSet(&context);
    patternSet.add<RewriteAlloca>(typeConverter, &context);
    patternSet.add<RewriteDeclare>(typeConverter, &context);
    patternSet.add<RewriteIf>(typeConverter, &context);
    patternSet.add<RewriteDoLoop>(typeConverter, &context);
    patternSet.add<RewriteDummyScope>(typeConverter, &context);

    (void) applyFullConversion(op, target, std::move(patternSet));
  }
};
} // namespace mlir
