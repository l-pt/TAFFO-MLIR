module attributes {dlti.dl_spec = #dlti.dl_spec<!llvm.ptr<270> = dense<32> : vector<4xi64>, !llvm.ptr<271> = dense<32> : vector<4xi64>, !llvm.ptr<272> = dense<64> : vector<4xi64>, i64 = dense<64> : vector<2xi64>, i128 = dense<128> : vector<2xi64>, f80 = dense<128> : vector<2xi64>, !llvm.ptr = dense<64> : vector<4xi64>, i1 = dense<8> : vector<2xi64>, i8 = dense<8> : vector<2xi64>, i16 = dense<16> : vector<2xi64>, i32 = dense<32> : vector<2xi64>, f16 = dense<16> : vector<2xi64>, f64 = dense<64> : vector<2xi64>, f128 = dense<128> : vector<2xi64>, "dlti.endianness" = "little", "dlti.mangling_mode" = "e", "dlti.stack_alignment" = 128 : i64>, fir.defaultkind = "a1c4d8i4l4r4", fir.kindmap = "", llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.ident = "flang version 21.0.0", llvm.target_triple = "x86_64-unknown-linux-gnu"} {
  func.func @_QPtestfor() -> i32 {
    %c10_i32 = arith.constant 10 : i32
    %c1_i32 = arith.constant 1 : i32
    %c0_i32 = arith.constant 0 : i32
    %0 = fir.dummy_scope : !fir.dscope
    %1 = fir.alloca i32 {bindc_name = "i", uniq_name = "_QFtestforEi"}
    %2 = fir.declare %1 {uniq_name = "_QFtestforEi"} : (!fir.ref<i32>) -> !fir.ref<i32>
    %3 = fir.alloca i32 {bindc_name = "rv", uniq_name = "_QFtestforErv"}
    %4 = fir.declare %3 {uniq_name = "_QFtestforErv"} : (!fir.ref<i32>) -> !fir.ref<i32>
    fir.store %c0_i32 to %4 : !fir.ref<i32>
    %5 = fir.convert %c1_i32 : (i32) -> index
    %6 = fir.convert %c10_i32 : (i32) -> index
    %7 = fir.convert %c1_i32 : (i32) -> index
    %8 = fir.convert %5 : (index) -> i32
    %9:2 = fir.do_loop %arg0 = %5 to %6 step %7 iter_args(%arg1 = %8) -> (index, i32) {
      fir.store %arg1 to %2 : !fir.ref<i32>
      %11 = fir.load %4 : !fir.ref<i32>
      %12 = fir.load %2 : !fir.ref<i32>
      %13 = arith.addi %11, %12 : i32
      fir.store %13 to %4 : !fir.ref<i32>
      %14 = arith.addi %arg0, %7 overflow<nsw> : index
      %15 = fir.convert %7 : (index) -> i32
      %16 = fir.load %2 : !fir.ref<i32>
      %17 = arith.addi %16, %15 overflow<nsw> : i32
      fir.result %14, %17 : index, i32
    }
    fir.store %9#1 to %2 : !fir.ref<i32>
    %10 = fir.load %4 : !fir.ref<i32>
    return %10 : i32
  }
}
