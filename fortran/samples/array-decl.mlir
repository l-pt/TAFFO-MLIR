module attributes {dlti.dl_spec = #dlti.dl_spec<!llvm.ptr<270> = dense<32> : vector<4xi64>, !llvm.ptr<271> = dense<32> : vector<4xi64>, !llvm.ptr<272> = dense<64> : vector<4xi64>, i64 = dense<64> : vector<2xi64>, i128 = dense<128> : vector<2xi64>, f80 = dense<128> : vector<2xi64>, !llvm.ptr = dense<64> : vector<4xi64>, i1 = dense<8> : vector<2xi64>, i8 = dense<8> : vector<2xi64>, i16 = dense<16> : vector<2xi64>, i32 = dense<32> : vector<2xi64>, f16 = dense<16> : vector<2xi64>, f64 = dense<64> : vector<2xi64>, f128 = dense<128> : vector<2xi64>, "dlti.endianness" = "little", "dlti.mangling_mode" = "e", "dlti.stack_alignment" = 128 : i64>, fir.defaultkind = "a1c4d8i4l4r4", fir.kindmap = "", llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.ident = "flang version 21.0.0", llvm.target_triple = "x86_64-unknown-linux-gnu"} {
  func.func @_QParraytest() -> f32 {
    %c3 = arith.constant 3 : index
    %c1_i32 = arith.constant 1 : i32
    %c31_i32 = arith.constant 31 : i32
    %c0_i32 = arith.constant 0 : i32
    %cst = arith.constant 0.000000e+00 : f32
    %c32 = arith.constant 32 : index
    %0 = fir.dummy_scope : !fir.dscope
    %1 = fir.alloca i32 {bindc_name = "i", uniq_name = "_QFarraytestEi"}
    %2 = fir.declare %1 {uniq_name = "_QFarraytestEi"} : (!fir.ref<i32>) -> !fir.ref<i32>
    %3 = fir.address_of(@_QFarraytestEmyarray) : !fir.ref<!fir.array<32xf32>>
    %4 = fir.shape %c32 : (index) -> !fir.shape<1>
    %5 = fir.declare %3(%4) {uniq_name = "_QFarraytestEmyarray"} : (!fir.ref<!fir.array<32xf32>>, !fir.shape<1>) -> !fir.ref<!fir.array<32xf32>>
    %6 = fir.alloca f32 {bindc_name = "rv", uniq_name = "_QFarraytestErv"}
    %7 = fir.declare %6 {uniq_name = "_QFarraytestErv"} : (!fir.ref<f32>) -> !fir.ref<f32>
    fir.store %cst to %7 : !fir.ref<f32>
    %8 = fir.convert %c0_i32 : (i32) -> index
    %9 = fir.convert %c31_i32 : (i32) -> index
    %10 = fir.convert %c1_i32 : (i32) -> index
    %11 = fir.convert %8 : (index) -> i32
    %12:2 = fir.do_loop %arg0 = %8 to %9 step %10 iter_args(%arg1 = %11) -> (index, i32) {
      fir.store %arg1 to %2 : !fir.ref<i32>
      %18 = fir.load %7 : !fir.ref<f32>
      %19 = fir.load %2 : !fir.ref<i32>
      %20 = fir.convert %19 : (i32) -> i64
      %21 = fir.array_coor %5(%4) %20 : (!fir.ref<!fir.array<32xf32>>, !fir.shape<1>, i64) -> !fir.ref<f32>
      %22 = fir.load %21 : !fir.ref<f32>
      %23 = arith.addf %18, %22 fastmath<contract> : f32
      fir.store %23 to %7 : !fir.ref<f32>
      %24 = arith.addi %arg0, %10 overflow<nsw> : index
      %25 = fir.convert %10 : (index) -> i32
      %26 = fir.load %2 : !fir.ref<i32>
      %27 = arith.addi %26, %25 overflow<nsw> : i32
      fir.result %24, %27 : index, i32
    }
    fir.store %12#1 to %2 : !fir.ref<i32>
    %13 = fir.load %7 : !fir.ref<f32>
    %14 = fir.array_coor %5(%4) %c3 : (!fir.ref<!fir.array<32xf32>>, !fir.shape<1>, index) -> !fir.ref<f32>
    %15 = fir.load %14 : !fir.ref<f32>
    %16 = arith.addf %13, %15 fastmath<contract> : f32
    fir.store %16 to %7 : !fir.ref<f32>
    %17 = fir.load %7 : !fir.ref<f32>
    return %17 : f32
  }
  fir.global internal @_QFarraytestEmyarray(dense<1.100000e+00> : tensor<32xf32>) : !fir.array<32xf32>
}
