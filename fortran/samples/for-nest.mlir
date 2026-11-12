module attributes {dlti.dl_spec = #dlti.dl_spec<!llvm.ptr<270> = dense<32> : vector<4xi64>, !llvm.ptr<271> = dense<32> : vector<4xi64>, !llvm.ptr<272> = dense<64> : vector<4xi64>, i64 = dense<64> : vector<2xi64>, i128 = dense<128> : vector<2xi64>, f80 = dense<128> : vector<2xi64>, !llvm.ptr = dense<64> : vector<4xi64>, i1 = dense<8> : vector<2xi64>, i8 = dense<8> : vector<2xi64>, i16 = dense<16> : vector<2xi64>, i32 = dense<32> : vector<2xi64>, f16 = dense<16> : vector<2xi64>, f64 = dense<64> : vector<2xi64>, f128 = dense<128> : vector<2xi64>, "dlti.endianness" = "little", "dlti.mangling_mode" = "e", "dlti.stack_alignment" = 128 : i64>, fir.defaultkind = "a1c4d8i4l4r4", fir.kindmap = "", llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.ident = "flang version 21.0.0", llvm.target_triple = "x86_64-unknown-linux-gnu"} {
  func.func @_QPtestfor() -> i32 {
    %c30_i32 = arith.constant 30 : i32
    %c3_i32 = arith.constant 3 : i32
    %c10_i32 = arith.constant 10 : i32
    %c1_i32 = arith.constant 1 : i32
    %c0_i32 = arith.constant 0 : i32
    %0 = fir.dummy_scope : !fir.dscope
    %1 = fir.alloca i32 {bindc_name = "i", uniq_name = "_QFtestforEi"}
    %2 = fir.declare %1 {uniq_name = "_QFtestforEi"} : (!fir.ref<i32>) -> !fir.ref<i32>
    %3 = fir.alloca i32 {bindc_name = "j", uniq_name = "_QFtestforEj"}
    %4 = fir.declare %3 {uniq_name = "_QFtestforEj"} : (!fir.ref<i32>) -> !fir.ref<i32>
    %5 = fir.alloca i32 {bindc_name = "rv", uniq_name = "_QFtestforErv"}
    %6 = fir.declare %5 {uniq_name = "_QFtestforErv"} : (!fir.ref<i32>) -> !fir.ref<i32>
    fir.store %c0_i32 to %6 : !fir.ref<i32>
    %7 = fir.convert %c1_i32 : (i32) -> index
    %8 = fir.convert %c10_i32 : (i32) -> index
    %9 = fir.convert %c1_i32 : (i32) -> index
    %10 = fir.convert %7 : (index) -> i32
    %11:2 = fir.do_loop %arg0 = %7 to %8 step %9 iter_args(%arg1 = %10) -> (index, i32) {
      fir.store %arg1 to %2 : !fir.ref<i32>
      %13 = fir.convert %c3_i32 : (i32) -> index
      %14 = fir.convert %c30_i32 : (i32) -> index
      %15 = fir.convert %c3_i32 : (i32) -> index
      %16 = fir.convert %13 : (index) -> i32
      %17:2 = fir.do_loop %arg2 = %13 to %14 step %15 iter_args(%arg3 = %16) -> (index, i32) {
        fir.store %arg3 to %4 : !fir.ref<i32>
        %22 = fir.load %6 : !fir.ref<i32>
        %23 = fir.load %2 : !fir.ref<i32>
        %24 = arith.addi %22, %23 : i32
        %25 = fir.load %4 : !fir.ref<i32>
        %26 = arith.addi %24, %25 : i32
        fir.store %26 to %6 : !fir.ref<i32>
        %27 = arith.addi %arg2, %15 overflow<nsw> : index
        %28 = fir.convert %15 : (index) -> i32
        %29 = fir.load %4 : !fir.ref<i32>
        %30 = arith.addi %29, %28 overflow<nsw> : i32
        fir.result %27, %30 : index, i32
      }
      fir.store %17#1 to %4 : !fir.ref<i32>
      %18 = arith.addi %arg0, %9 overflow<nsw> : index
      %19 = fir.convert %9 : (index) -> i32
      %20 = fir.load %2 : !fir.ref<i32>
      %21 = arith.addi %20, %19 overflow<nsw> : i32
      fir.result %18, %21 : index, i32
    }
    fir.store %11#1 to %2 : !fir.ref<i32>
    %12 = fir.load %6 : !fir.ref<i32>
    return %12 : i32
  }
}
