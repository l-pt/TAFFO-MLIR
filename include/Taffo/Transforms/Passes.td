#ifndef TAFFO_TRANSFORMS_PASSES_TD
#define TAFFO_TRANSFORMS_PASSES_TD

include "mlir/Pass/PassBase.td"
include "flang/Optimizer/Dialect/FIRDialect.td"

def FIRToMlirPass
    : Pass<"fir-to-mlir">
{
    let summary = "Convert FIR to intree MLIR dialects";

    let description = [{
        Raise scf and Arith ops generated from Polygeist to Taffo dialect.
    }];

    let dependentDialects = [
        "fir::FIROpsDialect",
        "mlir::scf::SCFDialect",
        "mlir::memref::MemRefDialect"
    ];
}

def RaiseToTaffoPass
    : Pass<"raise-to-taffo">
{
    let summary = "Raise scf and Arith ops to Taffo dialect.";

    let description = [{
        Raise scf and Arith ops generated from Polygeist to Taffo dialect.
    }];

    let dependentDialects = [
        "mlir::scf::SCFDialect",
        "mlir::arith::ArithDialect",
        "mlir::taffo::TaffoDialect"
    ];

}
def ValueRangeAnalysisPass
    : Pass<"value-range-analysis">
{
    let summary = "Infer the value range of every SSA value.";

    let description = [{
        Infer the value range of every SSA value.
        In the future this will rely on a yet to be implemented trip count
        estimation pass, and will be used for data type allocation.

        Currently only works with interval arithmetic, but it's planned to
        use affine arithmetic as well in the future.

        Attaches a "DatatypeInfo" attribute to each op to be used by other
        passes.
    }];

    let dependentDialects = [
        "mlir::taffo::TaffoDialect"
    ];

}

def LowerToArithPass
    : Pass<"lower-to-arith">
{
    let summary = "Lower Taffo ops to Arith dialect.";

    let description = [{
        Lower Taffo ops to Arith dialect.
        Depends on ValueRangeAnalysisPass and will fail if not correctly
        sequenced with it.
        Currently only the following ops are supported:
        add, mult, cast (float to fixp) and convert (fixp to float)
    }];

    let dependentDialects = [
        "mlir::taffo::TaffoDialect",
        "mlir::arith::ArithDialect",
        "mlir::scf::SCFDialect"
    ];

}

def DatatypeOptimizationPass
    : Pass<"dt-optimization">
{
    let summary = "Optimize datatypes by modifying the dtInfo attribute.";

    let description = [{
        Currently, this pass only sets the bitwidth of all values to 32 bit.
        In the future, this can be used as a hook to do all sorts of optimizations:
        shift minimization (datatype merging), overflow prevention, precision-driven
        datatype tuning etc...
    }];

    let dependentDialects = [
        "mlir::taffo::TaffoDialect",
    ];

}


#endif // TAFFO_TRANSFORMS_PASSES_TD
