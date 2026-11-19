module attributes {dlti.dl_spec = #dlti.dl_spec<!llvm.ptr<270> = dense<32> : vector<4xi64>, !llvm.ptr<271> = dense<32> : vector<4xi64>, !llvm.ptr<272> = dense<64> : vector<4xi64>, i64 = dense<64> : vector<2xi64>, i128 = dense<128> : vector<2xi64>, f80 = dense<128> : vector<2xi64>, !llvm.ptr = dense<64> : vector<4xi64>, i1 = dense<8> : vector<2xi64>, i8 = dense<8> : vector<2xi64>, i16 = dense<16> : vector<2xi64>, i32 = dense<32> : vector<2xi64>, f16 = dense<16> : vector<2xi64>, f64 = dense<64> : vector<2xi64>, f128 = dense<128> : vector<2xi64>, "dlti.endianness" = "little", "dlti.mangling_mode" = "e", "dlti.stack_alignment" = 128 : i64>, fir.defaultkind = "a1c4d8i4l4r4", fir.kindmap = "", llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.ident = "flang version 21.0.0", llvm.target_triple = "x86_64-unknown-linux-gnu"} {
  func.func @_QParraytest() -> i32 {
    %c23_i32 = arith.constant 23 : i32
    %c1 = arith.constant 1 : index
    %c21_i32 = arith.constant 21 : i32
    %c12_i32 = arith.constant 12 : i32
    %c11_i32 = arith.constant 11 : i32
    %c3 = arith.constant 3 : index
    %c-1 = arith.constant -1 : index
    %c2 = arith.constant 2 : index
    %c0 = arith.constant 0 : index
    %0 = fir.dummy_scope : !fir.dscope
    %1 = fir.alloca !fir.array<2x3xi32> {bindc_name = "myarray", uniq_name = "_QFarraytestEmyarray"}
    %2 = fir.shape_shift %c0, %c2, %c-1, %c3 : (index, index, index, index) -> !fir.shapeshift<2>
    %3 = fir.declare %1(%2) {uniq_name = "_QFarraytestEmyarray"} : (!fir.ref<!fir.array<2x3xi32>>, !fir.shapeshift<2>) -> !fir.ref<!fir.array<2x3xi32>>
    %4 = fir.embox %3(%2) : (!fir.ref<!fir.array<2x3xi32>>, !fir.shapeshift<2>) -> !fir.box<!fir.array<2x3xi32>>
    %5 = fir.alloca i32 {bindc_name = "rv", uniq_name = "_QFarraytestErv"}
    %6 = fir.declare %5 {uniq_name = "_QFarraytestErv"} : (!fir.ref<i32>) -> !fir.ref<i32>
    %7 = fir.array_coor %3(%2) %c0, %c-1 : (!fir.ref<!fir.array<2x3xi32>>, !fir.shapeshift<2>, index, index) -> !fir.ref<i32>
    fir.store %c11_i32 to %7 : !fir.ref<i32>
    %8 = fir.array_coor %3(%2) %c0, %c0 : (!fir.ref<!fir.array<2x3xi32>>, !fir.shapeshift<2>, index, index) -> !fir.ref<i32>
    fir.store %c12_i32 to %8 : !fir.ref<i32>
    %9 = fir.array_coor %3(%2) %c1, %c-1 : (!fir.ref<!fir.array<2x3xi32>>, !fir.shapeshift<2>, index, index) -> !fir.ref<i32>
    fir.store %c21_i32 to %9 : !fir.ref<i32>
    %10 = fir.array_coor %3(%2) %c1, %c1 : (!fir.ref<!fir.array<2x3xi32>>, !fir.shapeshift<2>, index, index) -> !fir.ref<i32>
    fir.store %c23_i32 to %10 : !fir.ref<i32>
    %11 = fir.array_coor %3(%2) %c1, %c-1 : (!fir.ref<!fir.array<2x3xi32>>, !fir.shapeshift<2>, index, index) -> !fir.ref<i32>
    %12 = fir.load %11 : !fir.ref<i32>
    fir.store %12 to %6 : !fir.ref<i32>
    %13 = fir.load %6 : !fir.ref<i32>
    return %13 : i32
  }
}
