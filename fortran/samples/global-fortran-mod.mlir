module attributes {dlti.dl_spec = #dlti.dl_spec<!llvm.ptr<270> = dense<32> : vector<4xi64>, !llvm.ptr<271> = dense<32> : vector<4xi64>, !llvm.ptr<272> = dense<64> : vector<4xi64>, i64 = dense<64> : vector<2xi64>, i128 = dense<128> : vector<2xi64>, f80 = dense<128> : vector<2xi64>, !llvm.ptr = dense<64> : vector<4xi64>, i1 = dense<8> : vector<2xi64>, i8 = dense<8> : vector<2xi64>, i16 = dense<16> : vector<2xi64>, i32 = dense<32> : vector<2xi64>, f16 = dense<16> : vector<2xi64>, f64 = dense<64> : vector<2xi64>, f128 = dense<128> : vector<2xi64>, "dlti.endianness" = "little", "dlti.mangling_mode" = "e", "dlti.stack_alignment" = 128 : i64>, fir.defaultkind = "a1c4d8i4l4r4", fir.kindmap = "", llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.ident = "flang version 21.0.0", llvm.target_triple = "x86_64-unknown-linux-gnu"} {
  func.func @_QPtest() -> i32 {
    %c10_i32 = arith.constant 10 : i32
    %0 = fir.dummy_scope : !fir.dscope
    %1 = fir.address_of(@_QFtestEglobal) : !fir.ref<i32>
    %2 = fir.declare %1 {uniq_name = "_QFtestEglobal"} : (!fir.ref<i32>) -> !fir.ref<i32>
    %3 = fir.alloca i32 {bindc_name = "x", uniq_name = "_QFtestEx"}
    %4 = fir.declare %3 {uniq_name = "_QFtestEx"} : (!fir.ref<i32>) -> !fir.ref<i32>
    %5 = fir.alloca i32 {bindc_name = "y", uniq_name = "_QFtestEy"}
    %6 = fir.declare %5 {uniq_name = "_QFtestEy"} : (!fir.ref<i32>) -> !fir.ref<i32>
    %7 = fir.load %2 : !fir.ref<i32>
    fir.store %7 to %4 : !fir.ref<i32>
    %8 = fir.load %4 : !fir.ref<i32>
    %9 = arith.addi %8, %c10_i32 : i32
    fir.store %9 to %6 : !fir.ref<i32>
    %10 = fir.load %6 : !fir.ref<i32>
    return %10 : i32
  }
  fir.global internal @_QFtestEglobal : i32 {
    %c42_i32 = arith.constant 42 : i32
    fir.has_value %c42_i32 : i32
  }
}
