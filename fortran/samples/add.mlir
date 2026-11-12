module attributes {dlti.dl_spec = #dlti.dl_spec<!llvm.ptr<270> = dense<32> : vector<4xi64>, !llvm.ptr<271> = dense<32> : vector<4xi64>, !llvm.ptr<272> = dense<64> : vector<4xi64>, i64 = dense<64> : vector<2xi64>, i128 = dense<128> : vector<2xi64>, f80 = dense<128> : vector<2xi64>, !llvm.ptr = dense<64> : vector<4xi64>, i1 = dense<8> : vector<2xi64>, i8 = dense<8> : vector<2xi64>, i16 = dense<16> : vector<2xi64>, i32 = dense<32> : vector<2xi64>, f16 = dense<16> : vector<2xi64>, f64 = dense<64> : vector<2xi64>, f128 = dense<128> : vector<2xi64>, "dlti.endianness" = "little", "dlti.mangling_mode" = "e", "dlti.stack_alignment" = 128 : i64>, fir.defaultkind = "a1c4d8i4l4r4", fir.kindmap = "", llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.ident = "flang version 21.0.0", llvm.target_triple = "x86_64-unknown-linux-gnu"} {
  func.func @_QPadd(%arg0: !fir.ref<i32> {fir.bindc_name = "a"}, %arg1: !fir.ref<i32> {fir.bindc_name = "b"}) -> i32 {
    %0 = fir.dummy_scope : !fir.dscope
    %1 = fir.declare %arg0 dummy_scope %0 {fortran_attrs = #fir.var_attrs<intent_in>, uniq_name = "_QFaddEa"} : (!fir.ref<i32>, !fir.dscope) -> !fir.ref<i32>
    %2 = fir.alloca i32 {bindc_name = "add", uniq_name = "_QFaddEadd"}
    %3 = fir.declare %2 {uniq_name = "_QFaddEadd"} : (!fir.ref<i32>) -> !fir.ref<i32>
    %4 = fir.declare %arg1 dummy_scope %0 {fortran_attrs = #fir.var_attrs<intent_in>, uniq_name = "_QFaddEb"} : (!fir.ref<i32>, !fir.dscope) -> !fir.ref<i32>
    %5 = fir.load %1 : !fir.ref<i32>
    %6 = fir.load %4 : !fir.ref<i32>
    %7 = arith.addi %5, %6 : i32
    fir.store %7 to %3 : !fir.ref<i32>
    %8 = fir.load %3 : !fir.ref<i32>
    return %8 : i32
  }
}
