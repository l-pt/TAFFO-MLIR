module attributes {dlti.dl_spec = #dlti.dl_spec<!llvm.ptr<270> = dense<32> : vector<4xi64>, !llvm.ptr<271> = dense<32> : vector<4xi64>, !llvm.ptr<272> = dense<64> : vector<4xi64>, i64 = dense<64> : vector<2xi64>, i128 = dense<128> : vector<2xi64>, f80 = dense<128> : vector<2xi64>, !llvm.ptr = dense<64> : vector<4xi64>, i1 = dense<8> : vector<2xi64>, i8 = dense<8> : vector<2xi64>, i16 = dense<16> : vector<2xi64>, i32 = dense<32> : vector<2xi64>, f16 = dense<16> : vector<2xi64>, f64 = dense<64> : vector<2xi64>, f128 = dense<128> : vector<2xi64>, "dlti.endianness" = "little", "dlti.mangling_mode" = "e", "dlti.stack_alignment" = 128 : i64>, fir.defaultkind = "a1c4d8i4l4r4", fir.kindmap = "", llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.ident = "flang version 21.0.0", llvm.target_triple = "x86_64-unknown-linux-gnu"} {
  func.func @_QPtestfor() -> i32 {
    %c10_i32 = arith.constant 10 : i32
    %c1_i32 = arith.constant 1 : i32
    %c44_i32 = arith.constant 44 : i32
    %0 = fir.dummy_scope : !fir.dscope
    %1 = fir.alloca i32 {bindc_name = "i", uniq_name = "_QFtestforEi"}
    %2 = fir.declare %1 {uniq_name = "_QFtestforEi"} : (!fir.ref<i32>) -> !fir.ref<i32>
    %3 = fir.alloca i32 {bindc_name = "j", uniq_name = "_QFtestforEj"}
    %4 = fir.declare %3 {uniq_name = "_QFtestforEj"} : (!fir.ref<i32>) -> !fir.ref<i32>
    %5 = fir.alloca i32 {bindc_name = "rv", uniq_name = "_QFtestforErv"}
    %6 = fir.declare %5 {uniq_name = "_QFtestforErv"} : (!fir.ref<i32>) -> !fir.ref<i32>
    fir.store %c44_i32 to %6 : !fir.ref<i32>
    %7 = fir.convert %c1_i32 : (i32) -> index
    %8 = fir.convert %c10_i32 : (i32) -> index
    %9 = fir.convert %c1_i32 : (i32) -> index
    %10 = fir.convert %7 : (index) -> i32
    %11:2 = fir.do_loop %arg0 = %7 to %8 step %9 iter_args(%arg1 = %10) -> (index, i32) {
      fir.store %arg1 to %2 : !fir.ref<i32>
      %13 = arith.addi %arg0, %9 overflow<nsw> : index
      %14 = fir.convert %9 : (index) -> i32
      %15 = fir.load %2 : !fir.ref<i32>
      %16 = arith.addi %15, %14 overflow<nsw> : i32
      fir.result %13, %16 : index, i32
    }
    fir.store %11#1 to %2 : !fir.ref<i32>
    %12 = fir.load %6 : !fir.ref<i32>
    return %12 : i32
  }
}
