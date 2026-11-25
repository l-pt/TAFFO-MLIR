#include "Taffo/Transforms/FIRToMlirPass.h"

#include "flang/Optimizer/Dialect/FIRDialect.h"
#include "flang/Optimizer/Dialect/FIROps.h"
#include "flang/Optimizer/Dialect/FIRType.h"
#include "flang/Optimizer/Dialect/Support/FIRContext.h"
#include "flang/Optimizer/Dialect/Support/KindMapping.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Index/IR/IndexDialect.h"
#include "mlir/Dialect/Index/IR/IndexOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/LLVMIR/LLVMTypes.h"
#include "mlir/Dialect/Math/IR/Math.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Shape/IR/Shape.h"
#include "mlir/IR/Attributes.h"
#include "mlir/IR/Block.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinDialect.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypeInterfaces.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/ImplicitLocOpBuilder.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/IR/TypeRange.h"
#include "mlir/IR/Types.h"
#include "mlir/IR/Value.h"
#include "mlir/IR/ValueRange.h"
#include "mlir/Support/LLVM.h"
#include "mlir/Transforms/DialectConversion.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/LogicalResult.h"
#include <cassert>
#include <numeric>
#include <utility>

namespace mlir::taffo {
#define GEN_PASS_DEF_FIRTOMLIRPASS
#include "Taffo/Transforms/Passes.h.inc"
} // namespace mlir::taffo

using namespace ::mlir::taffo;

namespace {
llvm::FailureOr<llvm::SmallVector<mlir::Type>> convTypes(const mlir::TypeConverter& converter, mlir::TypeRange in) {
  auto out = llvm::SmallVector<mlir::Type>{};
  if (failed(converter.convertTypes(in, out))) {
    return llvm::failure();
  }
  return out;
}

llvm::FailureOr<mlir::Type> convType(const mlir::TypeConverter& converter, mlir::Type in) {
  auto out = llvm::SmallVector<mlir::Type>{};
  if (failed(converter.convertTypes(mlir::TypeRange{in}, out))) {
    return llvm::failure();
  }
  return out[0];
}

llvm::FailureOr<mlir::MemRefType> getMemRefToType(mlir::Type type, mlir::MLIRContext& context) { //NOLINT
  if (type.isSignlessInteger(8)) {
    return llvm::failure();
  }
  if (type.isIntOrIndexOrFloat() || llvm::isa<mlir::MemRefType>(type)) {
    return mlir::MemRefType::get({}, type);
  }
  if (auto arrayType = llvm::dyn_cast<fir::SequenceType>(type)) {
    auto strides = llvm::SmallVector<int64_t>{};

    auto arrayShape = arrayType.getShape();
    int64_t stride = 1;
    bool dyn = false;
    for (int64_t e : arrayShape) {
      if (dyn) {
        strides.push_back(mlir::ShapedType::kDynamic);
      } else {
        strides.push_back(stride);
        if (mlir::ShapedType::isDynamic(e)) {
          dyn = true;
        } else {
          stride *= e;
        }
      }
    }
    auto layout = mlir::StridedLayoutAttr::get(&context, 0, strides);
    return mlir::MemRefType::get(arrayType.getShape(), arrayType.getElementType(), layout);
  }
  if (auto charType = llvm::dyn_cast<fir::CharacterType>(type)) {
    if (charType.getFKind() != 1 || charType.hasDynamicLen()) {
      return llvm::failure();
    }
    return mlir::MemRefType::get(charType.getLen(), mlir::IntegerType::get(&context, 8));
  }
  if (auto boxType = llvm::dyn_cast<fir::BoxType>(type)) {
    if (auto heapType = llvm::dyn_cast<fir::HeapType>(boxType.getElementType())) {
      assert(llvm::isa<fir::SequenceType>(heapType.getElementType()));
      auto m = getMemRefToType(heapType.getElementType(), context);
      if (llvm::succeeded(m)) {
        return mlir::MemRefType::get({}, *m);
      }
    }
  }
  return llvm::failure();
}

mlir::Value maybeEmitCastToIndex(mlir::Value val, mlir::ImplicitLocOpBuilder& builder, mlir::MLIRContext& context) {
  assert(val.getType().isIntOrIndex() && "val type not int or index");
  if (val.getType().isIndex()) {
    return val;
  }
  auto convOp = builder.create<mlir::arith::IndexCastOp>(mlir::IndexType::get(&context), val);
  return convOp.getResult();
}

llvm::FailureOr<llvm::SmallVector<mlir::Value>> lowerArrayIndices(fir::ArrayCoorOp arrCoorOp, mlir::ImplicitLocOpBuilder& builder,
    mlir::ConversionPatternRewriter& rewriter) {
  auto indicesShifted = mlir::SmallVector<mlir::Value>{};

  if (auto shapeShiftOp = arrCoorOp.getShape().getDefiningOp<fir::ShapeShiftOp>()) {
    for (auto [index, origin] : llvm::zip_equal(arrCoorOp.getIndices(), shapeShiftOp.getOrigins())) {
      assert(index.getType().isIntOrIndex() && "index type not int or index");
      assert(origin.getType().isIntOrIndex() && "origin type not int or index");

      auto lhs = maybeEmitCastToIndex(index, builder, *rewriter.getContext());
      auto rhs = maybeEmitCastToIndex(origin, builder, *rewriter.getContext());
      auto subOp = builder.create<mlir::arith::SubIOp>(lhs, rhs);
      indicesShifted.push_back(subOp.getResult());
    }
    return indicesShifted;
  }

  if (auto shapeOp = arrCoorOp.getShape().getDefiningOp<fir::ShapeOp>()) {
    auto one = builder.create<mlir::arith::ConstantIndexOp>(1);
    for (auto index : arrCoorOp.getIndices()) {
      assert(index.getType().isIntOrIndex() && "index type not int or index");

      auto lhs = maybeEmitCastToIndex(index, builder, *rewriter.getContext());
      auto subOp = builder.create<mlir::arith::SubIOp>(lhs, one.getResult());
      indicesShifted.push_back(subOp.getResult());
    }
    return indicesShifted;
  }

  return llvm::failure();
}
} // namespace

namespace mlir {
struct FIRToMlirPass : public mlir::taffo::impl::FIRToMlirPassBase<FIRToMlirPass> {
  using FIRToMlirPassBase::FIRToMlirPassBase;

  class FIRToMlirTypeConverter : public TypeConverter {
    static IntegerType::SignednessSemantics getSignednessSemantic(fir::IntegerType type) {
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
      addConversion([&ctx](fir::ReferenceType type) -> FailureOr<Type> {
        // FIXME: pointers are !fir.ref<i8>
        if (type.getElementType().isSignlessInteger(8)) {
          return mlir::LLVM::LLVMPointerType::get(&ctx);
        }
        return failure();
      });
      addConversion([&ctx](fir::ReferenceType type) {
        return getMemRefToType(type.getElementType(), ctx);
      });
      addConversion([&ctx](fir::ReferenceType type) -> FailureOr<Type> {
        // FIXME: _FortranAProgramStart
        if (auto tupleType = llvm::dyn_cast<TupleType>(type.getElementType())) {
          return mlir::LLVM::LLVMPointerType::get(&ctx);
        }
        return failure();
      });
      addConversion([&ctx](fir::BoxType type) -> FailureOr<Type> {
        // FIXME: _FortranAioBeginExternalFormattedOutput
        if (auto noneType = llvm::dyn_cast<NoneType>(type.getElementType())) {
          return mlir::LLVM::LLVMPointerType::get(&ctx);
        }

        // box<heap<array<x>>> is converted to memref<x>
        if (auto heapType = llvm::dyn_cast<fir::HeapType>(type.getElementType())) {
          if (auto arrayType = llvm::dyn_cast<fir::SequenceType>(heapType.getElementType())) {
            // array<xyz> -> memref<xyz>
            return getMemRefToType(arrayType, ctx);
          }
        }
        return failure();
      });
      addConversion([&ctx](fir::HeapType type) -> FailureOr<Type> {
        // heap<array<x>> is converted to memref<x>
        if (auto arrayType = llvm::dyn_cast<fir::SequenceType>(type.getElementType())) {
          return getMemRefToType(arrayType, ctx);
        }
        return failure();
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
        return {castToRealOp.getResult(0)};
      });
    }
  };

  struct RewriteDeclare : public OpConversionPattern<fir::DeclareOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::DeclareOp firDeclareOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      rewriter.replaceOp(firDeclareOp, adaptor.getMemref());
      return success();
    }
  };

  struct RewriteAlloca : public OpConversionPattern<fir::AllocaOp> {
    using OpConversionPattern::OpConversionPattern;

    static Value emitSizeCalculation(ValueRange vals, ImplicitLocOpBuilder& builder) {
      assert(!vals.empty());
      return std::accumulate(vals.begin() + 1, vals.end(), vals.front(), [&builder](const Value& a, const Value& b) {
        auto mulOp = builder.create<index::MulOp>(a, b);
        return mulOp.getResult();
      });
    }

    static SmallVector<OpFoldResult> emitStrides(ValueRange shape, ImplicitLocOpBuilder& builder) {
      auto strides = SmallVector<OpFoldResult>{builder.getIndexAttr(1)};
      auto val = shape.front();
      for (auto e : shape.drop_front().drop_back()) {
        strides.push_back(val);
        auto mulOp = builder.create<arith::MulIOp>(val, e);
        val = mulOp.getResult();
      }
      strides.push_back(val);
      return strides;
    }

    LogicalResult matchAndRewrite(fir::AllocaOp firAllocaOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      auto builder = ImplicitLocOpBuilder{firAllocaOp.getLoc(), rewriter};

      auto convertedType = getTypeConverter()->convertType(firAllocaOp.getAllocatedType());
      auto memRefType = getMemRefToType(convertedType, *getContext());
      if (failed(memRefType)) {
        return failure();
      }

      // Dynamic array handling, for example in:
      // integer, intent(in) :: n, m
      // integer :: myarray(n, m)
      if (auto arrayType = llvm::dyn_cast<fir::SequenceType>(convertedType); arrayType && arrayType.hasDynamicExtents()) {
        LLVM_DEBUG(llvm::dbgs() << "Array with dynamic shape allocated on the stack in " << firAllocaOp << "\n");
        auto nElems = emitSizeCalculation(adaptor.getShape(), builder);
        auto memrefAllocaOp = builder.create<memref::AllocaOp>(
            mlir::MemRefType::get({ShapedType::kDynamic}, arrayType.getElementType()), // ?xT
            SmallVector<Value>{nElems});

        auto sizes = SmallVector<OpFoldResult>{adaptor.getShape()};
        auto castOp = builder.create<memref::ReinterpretCastOp>(
            *memRefType, //resultType
            memrefAllocaOp.getResult(), //source
            builder.getIndexAttr(0), //offset
            sizes, //sizes
            emitStrides(adaptor.getShape(), builder)); //strides
        rewriter.replaceOp(firAllocaOp, castOp);
        return success();
      }

      auto memrefAllocaOp = builder.create<memref::AllocaOp>(*memRefType);
      rewriter.replaceOp(firAllocaOp, memrefAllocaOp);
      return success();
    }
  };

  struct RewriteAllocMem : public OpConversionPattern<fir::AllocMemOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::AllocMemOp allocOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      auto builder = ImplicitLocOpBuilder{allocOp.getLoc(), rewriter};

      auto convertedType = getTypeConverter()->convertType(allocOp.getAllocatedType());
      auto memRefType = getMemRefToType(convertedType, *getContext());
      if (failed(memRefType)) {
        return failure();
      }

      // We only support dynamic arrays with allocmem
      if (auto arrayType = llvm::dyn_cast<fir::SequenceType>(convertedType); arrayType && arrayType.hasDynamicExtents()) {
        LLVM_DEBUG(llvm::dbgs() << "Array with dynamic shape allocated on the heap in " << allocOp << "\n");
        auto nElems = RewriteAlloca::emitSizeCalculation(adaptor.getShape(), builder);
        auto memrefAllocOp = builder.create<memref::AllocOp>(
            mlir::MemRefType::get({ShapedType::kDynamic}, arrayType.getElementType()), // ?xT
            SmallVector<Value>{nElems});

        auto sizes = SmallVector<OpFoldResult>{adaptor.getShape()};
        auto castOp = builder.create<memref::ReinterpretCastOp>(
            *memRefType, //resultType
            memrefAllocOp.getResult(), //source
            builder.getIndexAttr(0), //offset
            sizes, //sizes
            RewriteAlloca::emitStrides(adaptor.getShape(), builder)); //strides
        rewriter.replaceOp(allocOp, castOp);
        return success();
      }

      return failure();
    }
  };

  struct RewriteLoad : public OpConversionPattern<fir::LoadOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::LoadOp firLoadOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      auto builder = ImplicitLocOpBuilder{firLoadOp.getLoc(), rewriter};

      // Stack arrays
      if (auto arrCoorOp = firLoadOp.getOperand().getDefiningOp<fir::ArrayCoorOp>()) {
        LLVM_DEBUG(llvm::dbgs() << "fir.load operand was defined with array_coor\n");

        // Array we are trying to load from
        auto array = rewriter.getRemappedValue(arrCoorOp.getMemref());
        auto indicesShifted = lowerArrayIndices(arrCoorOp, builder, rewriter);
        if (failed(indicesShifted)) {
          firLoadOp.emitError("index shift failed");
          return failure();
        }

        auto loadOp = builder.create<memref::LoadOp>(array, *indicesShifted);
        rewriter.replaceOp(firLoadOp, loadOp);
        return success();
      }

      // Scalar values
      auto loadOp = builder.create<memref::LoadOp>(adaptor.getMemref(), ValueRange{});
      rewriter.replaceOp(firLoadOp, loadOp);
      return success();
    }
  };

  struct RewriteStore : public OpConversionPattern<fir::StoreOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::StoreOp firStoreOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      auto builder = ImplicitLocOpBuilder{firStoreOp.getLoc(), rewriter};

      if (auto arrCoorOp = firStoreOp.getMemref().getDefiningOp<fir::ArrayCoorOp>()) {
        LLVM_DEBUG(llvm::dbgs() << "fir.load operand was defined with array_coor\n");

        // Array we are trying to store in
        auto array = rewriter.getRemappedValue(arrCoorOp.getMemref());
        auto arr = arrCoorOp.getMemref();
        LLVM_DEBUG(llvm::dbgs() << "RewriteStore array is " << array << "\n");
        LLVM_DEBUG(llvm::dbgs() << "RewriteStore arr is " << arr << "\n");
        auto indicesShifted = lowerArrayIndices(arrCoorOp, builder, rewriter);
        if (failed(indicesShifted)) {
          firStoreOp.emitError("index shift failed");
          return failure();
        }

        auto storeOp = builder.create<memref::StoreOp>(adaptor.getValue(), array, *indicesShifted);
        rewriter.replaceOp(firStoreOp, storeOp);
        return success();
      }

      auto storeOp = builder.create<memref::StoreOp>(adaptor.getValue(), adaptor.getMemref(), ValueRange{});
      rewriter.replaceOp(firStoreOp, storeOp);
      return success();
    }
  };

  struct RewriteDoLoop : public OpConversionPattern<fir::DoLoopOp> {
    using OpConversionPattern::OpConversionPattern;

    // See: https://mlir.llvm.org/docs/Dialects/SCFDialect/#scffor-scfforop
    // See: https://flang.llvm.org/docs/FIRLangRef.html#fir-do-loop-fir-doloopop
    LogicalResult matchAndRewrite(fir::DoLoopOp firDoLoopOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      auto builder = ImplicitLocOpBuilder{firDoLoopOp.getLoc(), rewriter};

      auto firLb = adaptor.getLowerBound();
      auto firUb = adaptor.getUpperBound();
      auto firStep = adaptor.getStep();

      // If the step is negative, swap lower and upper bound and use -step
      auto zeroConst = builder.create<arith::ConstantIndexOp>(0);
      auto isStepPositive = builder.create<arith::CmpIOp>(arith::CmpIPredicate::sgt, firStep, zeroConst);
      auto selectLowerBound = builder.create<arith::SelectOp>(isStepPositive.getResult(), firLb, firUb);
      auto selectUpperBound = builder.create<arith::SelectOp>(isStepPositive.getResult(), firUb, firLb);
      auto negStep = builder.create<arith::SubIOp>(zeroConst, firStep);
      auto selectStep = builder.create<arith::SelectOp>(isStepPositive.getResult(), firStep, negStep.getResult());

      // NOTE: fir.do_loop has inclusive upper bound, scf.for does not
      auto realUpperBound = builder.create<arith::AddIOp>(selectUpperBound.getResult(), selectStep.getResult());

      auto iterArgs = SmallVector<Value>{selectLowerBound.getResult()};
      llvm::append_range(iterArgs, adaptor.getInitArgs());
      auto forOp = builder.create<scf::ForOp>(selectLowerBound.getResult(),
          realUpperBound.getResult(),
          selectStep.getResult(),
          iterArgs);

      // Replace induction variable
      {
        auto guard = OpBuilder::InsertionGuard{builder};

        builder.setInsertionPointToStart(forOp.getBody());
        auto iv = forOp.getBody()->getArgument(0);
        auto delta = builder.create<arith::SubIOp>(iv, firLb);
        auto newIv = builder.create<arith::SubIOp>(firUb, delta.getResult());
        auto selectNewIv = builder.create<arith::SelectOp>(isStepPositive.getResult(), iv, newIv.getResult());

        auto argValues = SmallVector<Value>{selectNewIv.getResult()};
        // NOTE: drop_front is used as the first block argument is the IV which we already mapped
        llvm::append_range(argValues, forOp.getBody()->getArguments().drop_front(2));
        rewriter.mergeBlocks(firDoLoopOp.getBody(), forOp.getBody(), argValues);
      }

      // Replace fir.result with scf.yield
      {
        auto guard = OpBuilder::InsertionGuard{builder};

        assert(forOp.getBody()->mightHaveTerminator() && "scf.for body might have no terminator");
        auto* firResultOp = forOp.getBody()->getTerminator();
        assert(llvm::isa<fir::ResultOp>(firResultOp) && "fir.do_loop body terminator is not a fir.result");

        builder.setInsertionPointToEnd(forOp.getBody());
        auto yieldOp = builder.create<scf::YieldOp>(firResultOp->getOperands());
        rewriter.replaceOp(firResultOp, yieldOp);
        assert(llvm::isa<scf::YieldOp>(forOp.getBody()->getTerminator()) && "scf.for body terminator is not a scf.yield");
      }

      rewriter.replaceOp(firDoLoopOp, forOp);
      return success();
    }
  };

  struct RewriteIf : public OpConversionPattern<fir::IfOp> {
    using OpConversionPattern::OpConversionPattern;

    static void regionTakeBodyAndChangeTerminator(Region& from, Region& to, ImplicitLocOpBuilder& builder, ConversionPatternRewriter& rewriter) {
      auto guard = OpBuilder::InsertionGuard{builder};

      auto& fromBody = from.front();
      auto& toBody = to.front();

      // erase scf.yield in toBody, it is created by default
      assert(fromBody.mightHaveTerminator() && "newly created scf.if has no terminator");
      assert(llvm::isa<scf::YieldOp>(toBody.getTerminator()) && "scf.if body terminator is not a scf.yield");
      rewriter.eraseOp(toBody.getTerminator());

      toBody.getOperations().splice(toBody.end(), fromBody.getOperations());

      assert(toBody.mightHaveTerminator() && "fir.if body does not have a terminator");
      auto* terminator = toBody.getTerminator();
      assert(llvm::isa<fir::ResultOp>(terminator) && "fir.if body terminator is not a fir.result");

      builder.setInsertionPointToEnd(&toBody);
      auto yieldOp = builder.create<scf::YieldOp>(terminator->getOperands());
      rewriter.replaceOp(terminator, yieldOp);
    }

    LogicalResult matchAndRewrite(fir::IfOp firIfOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      auto builder = ImplicitLocOpBuilder{firIfOp.getLoc(), rewriter};

      bool hasElse = !adaptor.getElseRegion().empty();
      auto resultTypes = convTypes(*getTypeConverter(), firIfOp.getResultTypes());
      if (failed(resultTypes)) {
        return failure();
      }
      auto ifOp = builder.create<scf::IfOp>(
          *resultTypes,
          adaptor.getCondition(),
          hasElse);

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

    LogicalResult matchAndRewrite(fir::DummyScopeOp dummyScopeOp, OpAdaptor, ConversionPatternRewriter& rewriter) const override {
      rewriter.eraseOp(dummyScopeOp);
      return success();
    }
  };

  struct RewriteConvert : public OpConversionPattern<fir::ConvertOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::ConvertOp convertOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      auto builder = ImplicitLocOpBuilder{convertOp.getLoc(), rewriter};
      auto srcType = adaptor.getValue().getType();
      auto dstType = convertOp.getResult().getType();

      if ((srcType.isIndex() && dstType.isInteger()) || (srcType.isInteger() && dstType.isIndex())){
        auto indexCastOp = builder.create<arith::IndexCastOp>(dstType, adaptor.getValue());
        rewriter.replaceOp(convertOp, indexCastOp);
        return success();
      }

      if (((srcType.isSignlessInteger() && dstType.isSignlessInteger()) || (srcType.isSignedInteger() && dstType.isSignedInteger()))
          && srcType.getIntOrFloatBitWidth() < dstType.getIntOrFloatBitWidth()) {
        auto extOp = builder.create<arith::ExtSIOp>(dstType, adaptor.getValue());
        rewriter.replaceOp(convertOp, extOp);
        return success();
      }

      if (srcType.isUnsignedInteger() && dstType.isUnsignedInteger() &&
          srcType.getIntOrFloatBitWidth() < dstType.getIntOrFloatBitWidth()) {
        auto extOp = builder.create<arith::ExtUIOp>(dstType, adaptor.getValue());
        rewriter.replaceOp(convertOp, extOp);
        return success();
      }

      if (dstType.isFloat()) {
        if (srcType.isSignlessInteger() || srcType.isSignedInteger()) {
          auto toFpOp = builder.create<arith::SIToFPOp>(dstType, adaptor.getValue());
          rewriter.replaceOp(convertOp, toFpOp);
          return success();
        }
        if (srcType.isUnsignedInteger()) {
          auto toFpOp = builder.create<arith::UIToFPOp>(dstType, adaptor.getValue());
          rewriter.replaceOp(convertOp, toFpOp);
          return success();
        }
      }

      auto convDstType = getTypeConverter()->convertType(dstType);
      {
        auto srcMemRefType = llvm::dyn_cast<MemRefType>(srcType);
        auto dstMemRefType = llvm::dyn_cast<MemRefType>(convDstType);
        if (srcMemRefType && dstMemRefType && memref::CastOp::areCastCompatible(srcMemRefType, dstMemRefType)) {
          auto castOp = builder.create<memref::CastOp>(dstMemRefType, adaptor.getValue());
          rewriter.replaceOp(convertOp, castOp);
          return success();
        }
      }

      {
        auto srcMemRefType = llvm::dyn_cast<MemRefType>(srcType);
        auto dstPtrType = llvm::dyn_cast<LLVM::LLVMPointerType>(convDstType);
        if (srcMemRefType && dstPtrType) {
          // See: https://mlir.llvm.org/docs/Dialects/MemRef/#memrefextract_aligned_pointer_as_index-memrefextractalignedpointerasindexop
          auto extractPtrOp = builder.create<memref::ExtractAlignedPointerAsIndexOp>(adaptor.getValue());
          auto castOp = builder.create<arith::IndexCastOp>(builder.getI64Type(), extractPtrOp.getResult());
          auto intToPtrOp = builder.create<LLVM::IntToPtrOp>(LLVM::LLVMPointerType::get(getContext()), castOp.getResult());
          rewriter.replaceOp(convertOp, intToPtrOp);
          return success();
        }
      }

      convertOp.emitError("Unsupported fir.convert. srcType = ") << srcType << ", dstType = " << dstType;
      return failure();
    }
  };

  struct RewriteGlobal : public OpConversionPattern<fir::GlobalOp> {
    using OpConversionPattern::OpConversionPattern;

    static FailureOr<Attribute> getInitialValueFromInitializerBlock(Type globalType, Block& block, ImplicitLocOpBuilder& builder) {
      auto hasValueOp = llvm::dyn_cast<fir::HasValueOp>(block.getTerminator());
      if (!hasValueOp) {
        return failure();
      }
      auto resVal = hasValueOp.getResval();
      // Scalar constant
      if (auto constantOp = resVal.getDefiningOp<arith::ConstantOp>()) {
        auto val = constantOp.getValue();
        return mlir::DenseElementsAttr::get(
            RankedTensorType::get({}, globalType),
            SmallVector<Attribute>{val});
      }
      // String literal
      if (auto litOp = resVal.getDefiningOp<fir::StringLitOp>()) {
        auto stringAttr = llvm::cast<StringAttr>(litOp.getValue());
        auto sizeAttr = llvm::cast<IntegerAttr>(litOp.getSize());
        auto chars = SmallVector<Attribute>{};
        for (char c : stringAttr) {
          chars.push_back(builder.getI8IntegerAttr(c));
        }
        return mlir::DenseElementsAttr::get(
            RankedTensorType::get({sizeAttr.getInt()}, builder.getIntegerType(8)),
            chars);
      }
      // Zero initialized static array
      if (auto zeroOp = resVal.getDefiningOp<fir::ZeroOp>()) {
        if (auto arrayType = llvm::dyn_cast<fir::SequenceType>(globalType)) {
          auto zero = builder.getZeroAttr(arrayType.getElementType());
          return mlir::DenseElementsAttr::get(
              RankedTensorType::get(arrayType.getShape(), arrayType.getElementType()),
              zero);
        }
      }
      return failure();
    }

    LogicalResult matchAndRewrite(fir::GlobalOp globalOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      auto builder = ImplicitLocOpBuilder{globalOp.getLoc(), rewriter};

      auto firGlobalType = convType(*getTypeConverter(), adaptor.getType());
      if (failed(firGlobalType)) {
        globalOp.emitError("Failed to convert global type ") << adaptor.getType();
        return failure();
      }
      LLVM_DEBUG(llvm::dbgs() << "fir.global converted type is " << *firGlobalType << "\n");

      // Heap arrays
      if (auto firGlobalMemRefType = llvm::dyn_cast<MemRefType>(*firGlobalType)) {
        // Global type was box<heap<array<x>>> and was converted to memref<x>
        // by the type converter.
        auto wrapperType = MemRefType::get({}, firGlobalMemRefType);
        auto attributes = SmallVector<NamedAttribute>{
          NamedAttribute{"type", TypeAttr::get(wrapperType)},
          NamedAttribute{"sym_name", adaptor.getSymNameAttr()},
          NamedAttribute{"sym_visibility", StringAttr::get(getContext(), "private")},
        };
        if (globalOp.getConstant()) {
          attributes.push_back(NamedAttribute{"constant", UnitAttr::get(getContext())});
        }
        auto memrefGlobalOp = builder.create<memref::GlobalOp>(TypeRange{}, ValueRange{}, attributes); 
        rewriter.replaceOp(globalOp, memrefGlobalOp);
        LLVM_DEBUG(llvm::dbgs() << "RewriteGlobal replaced with " << memrefGlobalOp << "\n");
        return success();
      }

      // Scalars, static arrays, string literals
      auto initialValue = ([&]() -> FailureOr<Attribute> {
        if (globalOp.hasInitializationBody()) {
          auto initVal = getInitialValueFromInitializerBlock(*firGlobalType, globalOp.getBlock(), builder);
          if (failed(initVal)) {
            emitError(globalOp.getLoc()) << "unable to get initial value of initializer block";
            return failure();
          }
          return initVal;
        }
        if (auto initValAttr = adaptor.getInitVal(); initValAttr.has_value()) {
          return *initValAttr;
        }
        return {};
      })();
      if (failed(initialValue)) {
        return failure();
      }

      auto memrefType = getMemRefToType(*firGlobalType, *getContext());
      if (failed(memrefType)) {
        globalOp.emitError("unsupported global type ") << *firGlobalType;
        return failure();
      }

      auto t = MemRefType::get(memrefType->getShape(), memrefType->getElementType());
      auto attributes = SmallVector<NamedAttribute>{
        NamedAttribute{"type", TypeAttr::get(t)},
        NamedAttribute{"sym_name", adaptor.getSymNameAttr()},
        NamedAttribute{"sym_visibility", StringAttr::get(getContext(), "private")},
        NamedAttribute{"initial_value", *initialValue},
      };
      if (globalOp.getConstant()) {
        attributes.push_back(NamedAttribute{"constant", UnitAttr::get(getContext())});
      }
      if (auto align = globalOp.getAlignmentAttr()) {
        attributes.push_back(NamedAttribute{"alignment", align});
      }
      auto memrefGlobalOp = builder.create<memref::GlobalOp>(TypeRange{}, ValueRange{}, attributes);
      rewriter.replaceOp(globalOp, memrefGlobalOp);
      return success();
    }
  };

  struct RewriteAddressOf : public OpConversionPattern<fir::AddrOfOp> {
    using OpConversionPattern::OpConversionPattern;

    static SmallVector<OpFoldResult> toAttr(ArrayRef<int64_t> v, ImplicitLocOpBuilder& builder) {
      auto r = SmallVector<OpFoldResult>{};
      for (auto i : v) {
        r.push_back(builder.getIndexAttr(i));
      }
      return r;
    }

    static SmallVector<OpFoldResult> getStrides(ArrayRef<int64_t> shape, ImplicitLocOpBuilder& builder) {
      auto strides = SmallVector<OpFoldResult>{builder.getIndexAttr(1)};
      auto val = shape.front();
      for (auto e : shape.drop_front().drop_back()) {
        strides.push_back(builder.getIndexAttr(val));
        val = val * e;
      }
      strides.push_back(builder.getIndexAttr(val));
      return strides;
    }

    LogicalResult matchAndRewrite(fir::AddrOfOp addrOfOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      auto builder = ImplicitLocOpBuilder{addrOfOp.getLoc(), rewriter};
      auto sym = addrOfOp.getSymbol();
      auto symRefAttr = FlatSymbolRefAttr::get(rewriter.getContext(), sym.getRootReference().getValue());

      auto firRefType = llvm::cast<fir::ReferenceType>(addrOfOp.getType());
      LLVM_DEBUG(llvm::dbgs() << "fir.address_of global type is " << addrOfOp.getType() << "\n");

      if (auto type = llvm::dyn_cast<MemRefType>(getTypeConverter()->convertType(firRefType))) {
        auto gt = MemRefType::get(type.getShape(), type.getElementType());
        auto memrefGetGlobalOp = builder.create<memref::GetGlobalOp>(gt, symRefAttr);

        auto t = getMemRefToType(
            fir::SequenceType::get(type.getShape(), type.getElementType()),
            *getContext());
        if (type.getShape().size() > 1) {
          auto sizes = toAttr(type.getShape(), builder);
          auto castOp = builder.create<memref::ReinterpretCastOp>(
              *t, //resultType
              memrefGetGlobalOp.getResult(), //source
              builder.getIndexAttr(0), //offset
              sizes, //sizes
              getStrides(type.getShape(), builder)); //strides
          rewriter.replaceOp(addrOfOp, castOp);
        } else {
          rewriter.replaceOp(addrOfOp, memrefGetGlobalOp);
        }

        return success();
      }

      return failure();
    }
  };

  struct RewriteShape : public OpConversionPattern<fir::ShapeOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::ShapeOp shapeOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      auto builder = ImplicitLocOpBuilder{shapeOp.getLoc(), rewriter};

      auto fromExtentsOp = builder.create<shape::FromExtentsOp>(shape::ShapeType::get(getContext()), adaptor.getExtents());
      rewriter.replaceOp(shapeOp, fromExtentsOp);
      return success();
    }
  };

  struct RewriteShapeShift : public OpConversionPattern<fir::ShapeShiftOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::ShapeShiftOp shapeShiftOp, OpAdaptor, ConversionPatternRewriter& rewriter) const override {
      rewriter.eraseOp(shapeShiftOp);
      return success();
    }
  };

  struct RewriteArrayCoor : public OpConversionPattern<fir::ArrayCoorOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::ArrayCoorOp arrCoorOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      rewriter.eraseOp(arrCoorOp); // FIXME
      return success();
    }
  };

  struct RewriteCall : public OpConversionPattern<fir::CallOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::CallOp firCallOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      auto builder = ImplicitLocOpBuilder{firCallOp.getLoc(), rewriter};
      auto resultTypes = convTypes(*getTypeConverter(), firCallOp.getResultTypes());
      if (failed(resultTypes)) {
        return failure();
      }
      auto callOp = builder.create<func::CallOp>(*adaptor.getCallee(), *resultTypes, adaptor.getOperands());
      rewriter.replaceOp(firCallOp, callOp);
      return success();
    }
  };

  struct RewriteZeroBits : public OpConversionPattern<fir::ZeroOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::ZeroOp zeroBitsOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      auto builder = ImplicitLocOpBuilder{zeroBitsOp.getLoc(), rewriter};

      auto res = zeroBitsOp.getResult();
      if (!res.hasOneUse()) {
        zeroBitsOp.emitError("Unsupported");
        return failure();
      }
      if (auto firCallOp = llvm::dyn_cast<fir::CallOp>(*res.getUsers().begin())) {
        auto callee = firCallOp.getCallee()->getRootReference().getValue();
        if (!callee.starts_with("_Fortran")) {
          zeroBitsOp.emitError("Unsupported");
          return failure();
        }

        auto llvmPtrType = LLVM::LLVMPointerType::get(getContext());
        auto llvmZeroOp = builder.create<LLVM::ZeroOp>(llvmPtrType);
        rewriter.replaceOp(zeroBitsOp, llvmZeroOp);
        return success();
      }
      if (auto emboxOp = llvm::dyn_cast<fir::EmboxOp>(*res.getUsers().begin())) {
        if (!emboxOp.getResult().hasOneUse()) {
          zeroBitsOp.emitError("Unsupported");
          return failure();
        }
        if (auto storeOp = llvm::dyn_cast<fir::StoreOp>(*emboxOp.getResult().getUsers().begin())) {
          rewriter.eraseOp(storeOp);
          rewriter.eraseOp(emboxOp);
          rewriter.eraseOp(zeroBitsOp);
          return success();
        }
      }
      return failure();
    }
  };

  struct RewriteUnreachable : public OpConversionPattern<fir::UnreachableOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::UnreachableOp unreachableOp, OpAdaptor adaptor,
        ConversionPatternRewriter& rewriter) const override {
      auto builder = ImplicitLocOpBuilder{unreachableOp.getLoc(), rewriter};
      auto llvmUnreachableOp = builder.create<LLVM::UnreachableOp>();
      rewriter.replaceOp(unreachableOp, llvmUnreachableOp);
      return success();
    }
  };

  struct RewriteNoReassoc : public OpConversionPattern<fir::NoReassocOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::NoReassocOp noReassocOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      rewriter.replaceOp(noReassocOp, adaptor.getVal());
      return success();
    }
  };

  struct RewriteEmbox : public OpConversionPattern<fir::EmboxOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::EmboxOp emboxOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      rewriter.replaceOp(emboxOp, adaptor.getOperands()[0]);
      return success();
    }
  };

  struct RewriteBoxAddr : public OpConversionPattern<fir::BoxAddrOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::BoxAddrOp boxAddrOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      rewriter.replaceOp(boxAddrOp, adaptor.getOperands()[0]);
      return success();
    }
  };

  struct RewriteBoxDims : public OpConversionPattern<fir::BoxDimsOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::BoxDimsOp boxDimsOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      rewriter.eraseOp(boxDimsOp);
      return success();
    }
  };

  struct RewriteFreeMem : public OpConversionPattern<fir::FreeMemOp> {
    using OpConversionPattern::OpConversionPattern;

    LogicalResult matchAndRewrite(fir::FreeMemOp freeMemOp, OpAdaptor adaptor, ConversionPatternRewriter& rewriter) const override {
      auto builder = ImplicitLocOpBuilder{freeMemOp.getLoc(), rewriter};
      if (auto reinterpretOp = adaptor.getHeapref().getDefiningOp<memref::ReinterpretCastOp>()) {
        if (auto allocOp = reinterpretOp.getSource().getDefiningOp<memref::AllocOp>()) {
          auto deallocOp = builder.create<memref::DeallocOp>(allocOp.getMemref());
          rewriter.replaceOp(freeMemOp, deallocOp);
          return success();
        }
      } else if (auto allocOp = adaptor.getHeapref().getDefiningOp<memref::AllocOp>()) {
          auto deallocOp = builder.create<memref::DeallocOp>(allocOp.getMemref());
          rewriter.replaceOp(freeMemOp, deallocOp);
          return success();
      }
      return failure();
    }
  };

  void runOnOperation() override {
    auto& context = getContext();
    auto target = ConversionTarget{context};

    target.addIllegalDialect<fir::FIROpsDialect>();
    target.addLegalDialect<BuiltinDialect, arith::ArithDialect, index::IndexDialect, scf::SCFDialect, cf::ControlFlowDialect,
        memref::MemRefDialect, shape::ShapeDialect, LLVM::LLVMDialect, math::MathDialect>();
    target.addLegalOp<func::ReturnOp, func::CallOp>();

    auto* op = getOperation();
    auto typeConverter = FIRToMlirTypeConverter{context, *op};

    target.addDynamicallyLegalOp<func::FuncOp>([&typeConverter](func::FuncOp funcOp) {
      auto& body = funcOp.getRegion();
      return typeConverter.isSignatureLegal(funcOp.getFunctionType()) && typeConverter.isLegal(&body);
    });

    auto patternSet = RewritePatternSet{&context};
    patternSet.add<RewriteDummyScope>(typeConverter, &context);
    patternSet.add<RewriteAlloca>(typeConverter, &context);
    patternSet.add<RewriteAllocMem>(typeConverter, &context);
    patternSet.add<RewriteLoad>(typeConverter, &context);
    patternSet.add<RewriteStore>(typeConverter, &context);
    patternSet.add<RewriteDeclare>(typeConverter, &context);
    patternSet.add<RewriteConvert>(typeConverter, &context);
    patternSet.add<RewriteIf>(typeConverter, &context);
    patternSet.add<RewriteDoLoop>(typeConverter, &context);
    patternSet.add<RewriteGlobal>(typeConverter, &context);
    patternSet.add<RewriteAddressOf>(typeConverter, &context);
    patternSet.add<RewriteShape>(typeConverter, &context);
    patternSet.add<RewriteShapeShift>(typeConverter, &context);
    patternSet.add<RewriteArrayCoor>(typeConverter, &context);
    patternSet.add<RewriteCall>(typeConverter, &context);
    patternSet.add<RewriteZeroBits>(typeConverter, &context);
    patternSet.add<RewriteUnreachable>(typeConverter, &context);
    patternSet.add<RewriteNoReassoc>(typeConverter, &context);
    patternSet.add<RewriteEmbox>(typeConverter, &context);
    patternSet.add<RewriteBoxAddr>(typeConverter, &context);
    patternSet.add<RewriteBoxDims>(typeConverter, &context);
    patternSet.add<RewriteFreeMem>(typeConverter, &context);
    populateFunctionOpInterfaceTypeConversionPattern<func::FuncOp>(patternSet, typeConverter);

    if (failed(applyFullConversion(op, target, std::move(patternSet)))) {
      signalPassFailure();
    }
  }
};
} // namespace mlir
