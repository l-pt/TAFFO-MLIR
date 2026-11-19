module attributes {dlti.dl_spec = #dlti.dl_spec<!llvm.ptr<270> = dense<32> : vector<4xi64>, !llvm.ptr<271> = dense<32> : vector<4xi64>, !llvm.ptr<272> = dense<64> : vector<4xi64>, i64 = dense<64> : vector<2xi64>, i128 = dense<128> : vector<2xi64>, f80 = dense<128> : vector<2xi64>, !llvm.ptr = dense<64> : vector<4xi64>, i1 = dense<8> : vector<2xi64>, i8 = dense<8> : vector<2xi64>, i16 = dense<16> : vector<2xi64>, i32 = dense<32> : vector<2xi64>, f16 = dense<16> : vector<2xi64>, f64 = dense<64> : vector<2xi64>, f128 = dense<128> : vector<2xi64>, "dlti.endianness" = "little", "dlti.mangling_mode" = "e", "dlti.stack_alignment" = 128 : i64>, fir.defaultkind = "a1c4d8i4l4r4", fir.kindmap = "", llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.ident = "flang version 21.0.0", llvm.target_triple = "x86_64-unknown-linux-gnu"} {
  func.func @_QParraytest(%arg0: !fir.ref<i32> {fir.bindc_name = "x"}, %arg1: !fir.ref<i32> {fir.bindc_name = "y"}, %arg2: !fir.ref<i32> {fir.bindc_name = "z"}) -> i32 {
    %c3 = arith.constant 3 : index
    %c2 = arith.constant 2 : index
    %c1 = arith.constant 1 : index
    %c123_i32 = arith.constant 123 : i32
    %c0 = arith.constant 0 : index
    %0 = fir.dummy_scope : !fir.dscope
    %1 = fir.declare %arg0 dummy_scope %0 {fortran_attrs = #fir.var_attrs<intent_in>, uniq_name = "_QFarraytestEx"} : (!fir.ref<i32>, !fir.dscope) -> !fir.ref<i32>
    %2 = fir.declare %arg1 dummy_scope %0 {fortran_attrs = #fir.var_attrs<intent_in>, uniq_name = "_QFarraytestEy"} : (!fir.ref<i32>, !fir.dscope) -> !fir.ref<i32>
    %3 = fir.declare %arg2 dummy_scope %0 {fortran_attrs = #fir.var_attrs<intent_in>, uniq_name = "_QFarraytestEz"} : (!fir.ref<i32>, !fir.dscope) -> !fir.ref<i32>
    %4 = fir.alloca i32 {bindc_name = "rv", uniq_name = "_QFarraytestErv"}
    %5 = fir.declare %4 {uniq_name = "_QFarraytestErv"} : (!fir.ref<i32>) -> !fir.ref<i32>
    %6 = fir.load %1 : !fir.ref<i32>
    %7 = fir.convert %6 : (i32) -> i64
    %8 = fir.convert %7 : (i64) -> index
    %9 = arith.cmpi sgt, %8, %c0 : index
    %10 = arith.select %9, %8, %c0 : index
    %11 = fir.load %2 : !fir.ref<i32>
    %12 = fir.convert %11 : (i32) -> i64
    %13 = fir.convert %12 : (i64) -> index
    %14 = arith.cmpi sgt, %13, %c0 : index
    %15 = arith.select %14, %13, %c0 : index
    %16 = fir.load %3 : !fir.ref<i32>
    %17 = fir.convert %16 : (i32) -> i64
    %18 = fir.convert %17 : (i64) -> index
    %19 = arith.cmpi sgt, %18, %c0 : index
    %20 = arith.select %19, %18, %c0 : index
    %21 = fir.alloca !fir.array<?x?x?xi32>, %10, %15, %20 {bindc_name = "myarray", uniq_name = "_QFarraytestEmyarray"}
    %22 = fir.shape %10, %15, %20 : (index, index, index) -> !fir.shape<3>
    %23 = fir.declare %21(%22) {uniq_name = "_QFarraytestEmyarray"} : (!fir.ref<!fir.array<?x?x?xi32>>, !fir.shape<3>) -> !fir.ref<!fir.array<?x?x?xi32>>
    %24 = fir.embox %23(%22) : (!fir.ref<!fir.array<?x?x?xi32>>, !fir.shape<3>) -> !fir.box<!fir.array<?x?x?xi32>>
    %25 = fir.array_coor %23(%22) %c1, %c2, %c3 : (!fir.ref<!fir.array<?x?x?xi32>>, !fir.shape<3>, index, index, index) -> !fir.ref<i32>
    fir.store %c123_i32 to %25 : !fir.ref<i32>
    %26 = fir.array_coor %23(%22) %c1, %c2, %c3 : (!fir.ref<!fir.array<?x?x?xi32>>, !fir.shape<3>, index, index, index) -> !fir.ref<i32>
    %27 = fir.load %26 : !fir.ref<i32>
    fir.store %27 to %5 : !fir.ref<i32>
    %28 = fir.load %5 : !fir.ref<i32>
    return %28 : i32
  }
}
