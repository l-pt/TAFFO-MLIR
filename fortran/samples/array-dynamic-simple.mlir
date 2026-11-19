module attributes {dlti.dl_spec = #dlti.dl_spec<!llvm.ptr<270> = dense<32> : vector<4xi64>, !llvm.ptr<271> = dense<32> : vector<4xi64>, !llvm.ptr<272> = dense<64> : vector<4xi64>, i64 = dense<64> : vector<2xi64>, i128 = dense<128> : vector<2xi64>, f80 = dense<128> : vector<2xi64>, !llvm.ptr = dense<64> : vector<4xi64>, i1 = dense<8> : vector<2xi64>, i8 = dense<8> : vector<2xi64>, i16 = dense<16> : vector<2xi64>, i32 = dense<32> : vector<2xi64>, f16 = dense<16> : vector<2xi64>, f64 = dense<64> : vector<2xi64>, f128 = dense<128> : vector<2xi64>, "dlti.endianness" = "little", "dlti.mangling_mode" = "e", "dlti.stack_alignment" = 128 : i64>, fir.defaultkind = "a1c4d8i4l4r4", fir.kindmap = "", llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.ident = "flang version 21.0.0", llvm.target_triple = "x86_64-unknown-linux-gnu"} {
  func.func @_QParraytest(%arg0: !fir.ref<i32> {fir.bindc_name = "n"}, %arg1: !fir.ref<i32> {fir.bindc_name = "m"}) -> i32 {
    %c2 = arith.constant 2 : index
    %c1 = arith.constant 1 : index
    %c12_i32 = arith.constant 12 : i32
    %c0 = arith.constant 0 : index
    %0 = fir.dummy_scope : !fir.dscope
    %1 = fir.declare %arg1 dummy_scope %0 {fortran_attrs = #fir.var_attrs<intent_in>, uniq_name = "_QFarraytestEm"} : (!fir.ref<i32>, !fir.dscope) -> !fir.ref<i32>
    %2 = fir.declare %arg0 dummy_scope %0 {fortran_attrs = #fir.var_attrs<intent_in>, uniq_name = "_QFarraytestEn"} : (!fir.ref<i32>, !fir.dscope) -> !fir.ref<i32>
    %3 = fir.alloca i32 {bindc_name = "rv", uniq_name = "_QFarraytestErv"}
    %4 = fir.declare %3 {uniq_name = "_QFarraytestErv"} : (!fir.ref<i32>) -> !fir.ref<i32>
    %5 = fir.load %2 : !fir.ref<i32>
    %6 = fir.convert %5 : (i32) -> i64
    %7 = fir.convert %6 : (i64) -> index
    %8 = arith.cmpi sgt, %7, %c0 : index
    %9 = arith.select %8, %7, %c0 : index
    %10 = fir.load %1 : !fir.ref<i32>
    %11 = fir.convert %10 : (i32) -> i64
    %12 = fir.convert %11 : (i64) -> index
    %13 = arith.cmpi sgt, %12, %c0 : index
    %14 = arith.select %13, %12, %c0 : index
    %15 = fir.alloca !fir.array<?x?xi32>, %9, %14 {bindc_name = "myarray", uniq_name = "_QFarraytestEmyarray"}
    %16 = fir.shape %9, %14 : (index, index) -> !fir.shape<2>
    %17 = fir.declare %15(%16) {uniq_name = "_QFarraytestEmyarray"} : (!fir.ref<!fir.array<?x?xi32>>, !fir.shape<2>) -> !fir.ref<!fir.array<?x?xi32>>
    %18 = fir.embox %17(%16) : (!fir.ref<!fir.array<?x?xi32>>, !fir.shape<2>) -> !fir.box<!fir.array<?x?xi32>>
    %19 = fir.array_coor %17(%16) %c1, %c2 : (!fir.ref<!fir.array<?x?xi32>>, !fir.shape<2>, index, index) -> !fir.ref<i32>
    fir.store %c12_i32 to %19 : !fir.ref<i32>
    %20 = fir.array_coor %17(%16) %c1, %c2 : (!fir.ref<!fir.array<?x?xi32>>, !fir.shape<2>, index, index) -> !fir.ref<i32>
    %21 = fir.load %20 : !fir.ref<i32>
    fir.store %21 to %4 : !fir.ref<i32>
    %22 = fir.load %4 : !fir.ref<i32>
    return %22 : i32
  }
}
