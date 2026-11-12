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
#include "mlir/IR/Block.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributeInterfaces.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinDialect.h"
#include "mlir/IR/BuiltinOps.h"
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
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/LogicalResult.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <cstdlib>
#include <optional>
#include <utility>

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
        return MemRefType::get({}, type.getElementType());
      });

      addSourceMaterialization([](OpBuilder& builder, Type resultType, ValueRange inputs, Location loc) -> Value {
        if (inputs.size() != 1) {
          return {};
        }
        auto castToRealOp = builder.create<mlir::UnrealizedConversionCastOp>(loc, resultType, inputs);
        return castToRealOp.getResult(0);
      });

      addTargetMaterialization([](OpBuilder& builder, TypeRange resultType, ValueRange inputs, Location loc) -> SmallVector<Value> {
        if (inputs.size() != 1) {
          return {};
        }
        auto castToRealOp = builder.create<mlir::UnrealizedConversionCastOp>(loc, resultType, inputs);
        SmallVector<Value> result;
        result.push_back(castToRealOp.getResult(0));
        return result;
      });
    }
  };

  static SmallVector<Type> convTypes(const TypeConverter& converter, TypeRange in) {
    SmallVector<Type> out;
    if (failed(converter.convertTypes(in, out))) {
      llvm::errs() << "Conversion error\n";
      std::abort();
    }
    return out;
  }

  struct RewriteDeclare : public OpConversionPattern<fir::DeclareOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::DeclareOp firDeclareOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      emitWarning(firDeclareOp.getLoc()) << "fir::declare shape " << firDeclareOp.getShape();
      emitWarning(firDeclareOp.getLoc()) << "fir::declare result " << firDeclareOp.getResult();
      rewriter.replaceOp(firDeclareOp, adaptor.getMemref());
      return success();
    }
  };

  struct RewriteAlloca : public OpConversionPattern<fir::AllocaOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::AllocaOp firAllocaOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      ImplicitLocOpBuilder builder(firAllocaOp.getLoc(), rewriter);
      auto convertedType = getTypeConverter()->convertType(firAllocaOp.getAllocatedType());
      auto memrefAllocaOp = builder.create<memref::AllocaOp>(MemRefType::get({}, convertedType)); //FIXME shape
      rewriter.replaceOp(firAllocaOp, memrefAllocaOp);
      return success();
    }
  };

  struct RewriteLoad : public OpConversionPattern<fir::LoadOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::LoadOp firLoadOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      ImplicitLocOpBuilder builder(firLoadOp.getLoc(), rewriter);
      auto zeroConst = builder.create<arith::ConstantIndexOp>(0); //FIXME correct index
      auto loadOp = builder.create<memref::LoadOp>(adaptor.getMemref(), ValueRange{});
      rewriter.replaceOp(firLoadOp, loadOp);
      return success();
    }
  };

  struct RewriteStore : public OpConversionPattern<fir::StoreOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::StoreOp firStoreOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      ImplicitLocOpBuilder builder(firStoreOp.getLoc(), rewriter);
      auto zeroConst = builder.create<arith::ConstantIndexOp>(0); //FIXME correct index
      auto storeOp = builder.create<memref::StoreOp>(adaptor.getValue(), adaptor.getMemref(), ValueRange{});
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
      Value firStep = adaptor.getStep();

      //If the step is negative, swap lower and upper bound and use -step
      auto zeroConst = builder.create<arith::ConstantIndexOp>(0); //TODO check if we need different types
      auto isStepPositive = builder.create<arith::CmpIOp>(arith::CmpIPredicate::sgt, firStep, zeroConst);
      auto selectLowerBound = builder.create<arith::SelectOp>(isStepPositive.getResult(), firLb, firUb);
      auto selectUpperBound = builder.create<arith::SelectOp>(isStepPositive.getResult(), firUb, firLb);
      auto negStep = builder.create<arith::SubIOp>(zeroConst, firStep);
      auto selectStep = builder.create<arith::SelectOp>(isStepPositive.getResult(), firStep, negStep.getResult());

      //NOTE fir.do_loop has inclusive upper bound, scf.for does not
      auto realUpperBound = builder.create<arith::AddIOp>(selectUpperBound.getResult(), selectStep.getResult());
      auto forOp = builder.create<scf::ForOp>(selectLowerBound.getResult(),
          realUpperBound.getResult(),
          selectStep.getResult(),
          adaptor.getInitArgs());

      //Replace induction variable
      {
        OpBuilder::InsertionGuard guard(builder);

        builder.setInsertionPointToStart(forOp.getBody());
        auto iv = forOp.getBody()->getArgument(0);
        auto delta = builder.create<arith::SubIOp>(iv, firLb);
        auto newIv = builder.create<arith::SubIOp>(firUb, delta.getResult());
        auto selectNewIv = builder.create<arith::SelectOp>(isStepPositive.getResult(), iv, newIv.getResult());

        SmallVector<Value> argValues;
        argValues.push_back(selectNewIv.getResult());
        //NOTE drop_front is used as the first block argument is the IV which we already mapped
        llvm::append_range(argValues, forOp.getBody()->getArguments().drop_front());
        rewriter.mergeBlocks(firDoLoopOp.getBody(), forOp.getBody(), argValues);
      }

      //Replace fir.result with scf.yield
      {
        OpBuilder::InsertionGuard guard(builder);
        assert(forOp.getBody()->mightHaveTerminator() && "scf.for body might have no terminator");
        Operation* firResultOp = forOp.getBody()->getTerminator();
        assert(llvm::isa<fir::ResultOp>(firResultOp) && "fir.do_loop body terminator is not a fir.result");
        builder.setInsertionPointToEnd(forOp.getBody());
        auto yieldOp = builder.create<scf::YieldOp>(forOp.getRegionIterArgs());
        rewriter.replaceOp(firResultOp, yieldOp);
        assert(llvm::isa<scf::YieldOp>(forOp.getBody()->getTerminator()) && "scf.for body terminator is not a scf.yield");
      }

      SmallVector<Value> results;
      auto toIdx = builder.create<arith::IndexCastOp>(IndexType::get(getContext()), forOp.getResults().front());
      results.push_back(toIdx.getResult()); //FIXME
      llvm::append_range(results, forOp.getResults());
      rewriter.replaceOp(firDoLoopOp, results);
      return success();
    }
  };

  struct RewriteIf : public OpConversionPattern<fir::IfOp> {
    using OpConversionPattern::OpConversionPattern;

    static void regionTakeBodyAndChangeTerminator(Region& from, Region& to, ImplicitLocOpBuilder& builder, ConversionPatternRewriter& rewriter) {
      OpBuilder::InsertionGuard guard(builder);
      to.takeBody(from);
      Block& body = to.front();
      assert(body.mightHaveTerminator() && "fir.if body does not have a terminator");
      Operation* terminator = body.getTerminator();
      assert(llvm::isa<fir::ResultOp>(terminator) && "fir.if body terminator is not a fir.result");
      builder.setInsertionPointToEnd(&body);
      auto yieldOp = builder.create<scf::YieldOp>(terminator->getOperands());
      rewriter.replaceOp(terminator, yieldOp);
    }

    LogicalResult matchAndRewrite(fir::IfOp firIfOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      ImplicitLocOpBuilder builder(firIfOp.getLoc(), rewriter);

      bool hasElse = !adaptor.getElseRegion().empty();
      auto ifOp = builder.create<scf::IfOp>(convTypes(*getTypeConverter(), firIfOp.getResultTypes()), adaptor.getCondition(), hasElse);

      //NOTE each block is terminated by a fir.result op, we have to convert it in a scf.yield op.
      //See: https://mlir.llvm.org/docs/Dialects/SCFDialect/#scfyield-scfyieldop
      //See: https://flang.llvm.org/docs/FIRLangRef.html#fir-result-fir-resultop
      regionTakeBodyAndChangeTerminator(adaptor.getThenRegion(), ifOp.getThenRegion(), builder, rewriter);
      if (hasElse) {
        regionTakeBodyAndChangeTerminator(adaptor.getElseRegion(), ifOp.getElseRegion(), builder, rewriter);
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

  struct RewriteConvert : public OpConversionPattern<fir::ConvertOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::ConvertOp convertOp, OpAdaptor adaptor, ConversionPatternRewriter &rewriter) const override {
      ImplicitLocOpBuilder builder(convertOp.getLoc(), rewriter);
      Type srcType = adaptor.getValue().getType();
      Type dstType = convertOp.getResult().getType();

      if ((srcType.isIndex() && dstType.isInteger()) || (srcType.isInteger() && dstType.isIndex())){
        auto indexCastOp = builder.create<arith::IndexCastOp>(dstType, adaptor.getValue());
        rewriter.replaceOp(convertOp, indexCastOp);
        return success();
      }

      convertOp.emitError("Unsupported fir.convert");
      return failure();
    }
  };

  struct RewriteGlobal : public OpConversionPattern<fir::GlobalOp> {
    using OpConversionPattern::OpConversionPattern;

    // Even simple i32 globals are defined with an initializer block, so we have
    // to support it at least for simple cases (scalars, not arrays)
    static std::optional<TypedAttr> getInitialValueFromInitializerBlock(Type globalType, Block& block) {
      auto hasValueOp = llvm::dyn_cast<fir::HasValueOp>(block.getTerminator());
      if (!hasValueOp) {
        return std::nullopt;
      }
      auto resVal = hasValueOp.getResval();
      auto definingOp = resVal.getDefiningOp<mlir::arith::ConstantOp>();
      if (!definingOp) {
        return std::nullopt;
      }
      auto val = definingOp.getValue();
      return mlir::DenseElementsAttr::get(
          RankedTensorType::get({}, globalType),
          llvm::ArrayRef<Attribute>{val});
    }

    LogicalResult matchAndRewrite(fir::GlobalOp globalOp, OpAdaptor adaptor, ConversionPatternRewriter &rewriter) const override {
      ImplicitLocOpBuilder builder(globalOp.getLoc(), rewriter);

      Type globalType = adaptor.getType();
      emitWarning(globalOp.getLoc()) << "globalType " << globalType;

      std::optional<TypedAttr> initialValue = std::nullopt;
      if (globalOp.hasInitializationBody()) {
        if (!globalType.isIntOrIndexOrFloat()) {
          emitError(globalOp.getLoc()) << "initializer block on a non-scalar global";
          return failure();
        }
        initialValue = getInitialValueFromInitializerBlock(globalType, globalOp.getBlock());
        if (!initialValue) {
          emitError(globalOp.getLoc()) << "unable to get initial value of initializer block";
          return failure();
        }
      }

      auto memrefTypeOpt = ([&]() -> std::optional<MemRefType> {
        if (globalType.isIntOrIndexOrFloat()) {
          return MemRefType::get({}, globalType);
        }
        if (auto arrayType = mlir::dyn_cast<fir::SequenceType>(globalType)) {
          return MemRefType::get(arrayType.getShape(), arrayType.getElementType());
        }
        return std::nullopt;
      })();
      if (!memrefTypeOpt) {
        emitError(globalOp.getLoc()) << "unsupported global type " << globalType;
        return failure();
      }
      auto memrefType = *memrefTypeOpt;

      auto memrefGlobalOp = builder.create<memref::GlobalOp>(
          adaptor.getSymNameAttr(),
          StringAttr::get(getContext(), "private"),
          TypeAttr::get(memrefType),
          initialValue ? *initialValue : Attribute{},
          UnitAttr{}, // TODO constant?
          IntegerAttr{}); // TODO alignment

      emitWarning(globalOp.getLoc()) << "symName " << adaptor.getSymName();
      emitWarning(globalOp.getLoc()) << "memreftype " << memrefType;
      emitWarning(globalOp.getLoc()) << "memrefGlobalOp " << memrefGlobalOp;

      rewriter.replaceOp(globalOp, memrefGlobalOp);
      return success();
    }
  };

  struct RewriteAddressOf : public OpConversionPattern<fir::AddrOfOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::AddrOfOp addrOfOp, OpAdaptor adaptor, ConversionPatternRewriter &rewriter) const override {
      ImplicitLocOpBuilder builder(addrOfOp.getLoc(), rewriter);

      assert(llvm::isa<fir::ReferenceType>(addrOfOp.getType()));

      auto type = getTypeConverter()->convertType(addrOfOp.getType());
      assert(llvm::isa<MemRefType>(type));

      auto sym = addrOfOp.getSymbol();
      auto symRefAttr = FlatSymbolRefAttr::get(rewriter.getContext(), sym.getRootReference().getValue());

      auto memrefGetGlobalOp = builder.create<memref::GetGlobalOp>(type, symRefAttr);
      rewriter.replaceOp(addrOfOp, memrefGetGlobalOp);
      return success();
    }
  };

  void runOnOperation() override {
    MLIRContext& context = getContext();
    ConversionTarget target(context);

    target.addIllegalDialect<fir::FIROpsDialect>();
    target.addLegalDialect<BuiltinDialect, arith::ArithDialect, func::FuncDialect, scf::SCFDialect, memref::MemRefDialect>();

    Operation* op = getOperation();
    FIRToMlirTypeConverter typeConverter(context, *op);

    RewritePatternSet patternSet(&context);
    patternSet.add<RewriteDummyScope>(typeConverter, &context);
    patternSet.add<RewriteAlloca>(typeConverter, &context);
    patternSet.add<RewriteLoad>(typeConverter, &context);
    patternSet.add<RewriteStore>(typeConverter, &context);
    patternSet.add<RewriteDeclare>(typeConverter, &context);
    patternSet.add<RewriteConvert>(typeConverter, &context);
    patternSet.add<RewriteIf>(typeConverter, &context);
    patternSet.add<RewriteDoLoop>(typeConverter, &context);
    patternSet.add<RewriteGlobal>(typeConverter, &context);
    patternSet.add<RewriteAddressOf>(typeConverter, &context);

    if (failed(applyFullConversion(op, target, std::move(patternSet)))) {
      signalPassFailure();
    }
  }
};
} // namespace mlir
