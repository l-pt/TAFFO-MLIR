module attributes {dlti.dl_spec = #dlti.dl_spec<!llvm.ptr<270> = dense<32> : vector<4xi64>, !llvm.ptr<271> = dense<32> : vector<4xi64>, !llvm.ptr<272> = dense<64> : vector<4xi64>, i64 = dense<64> : vector<2xi64>, i128 = dense<128> : vector<2xi64>, f80 = dense<128> : vector<2xi64>, !llvm.ptr = dense<64> : vector<4xi64>, i1 = dense<8> : vector<2xi64>, i8 = dense<8> : vector<2xi64>, i16 = dense<16> : vector<2xi64>, i32 = dense<32> : vector<2xi64>, f16 = dense<16> : vector<2xi64>, f64 = dense<64> : vector<2xi64>, f128 = dense<128> : vector<2xi64>, "dlti.endianness" = "little", "dlti.mangling_mode" = "e", "dlti.stack_alignment" = 128 : i64>, fir.defaultkind = "a1c4d8i4l4r4", fir.kindmap = "", llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.ident = "flang version 21.0.0", llvm.target_triple = "x86_64-unknown-linux-gnu"} {
  func.func @_QParraytest() -> i32 {
    %c1 = arith.constant 1 : index
    %c3_i32 = arith.constant 3 : i32
    %c11 = arith.constant 11 : index
    %c-5 = arith.constant -5 : index
    %0 = fir.dummy_scope : !fir.dscope
    %1 = fir.alloca !fir.array<11xi32> {bindc_name = "myarray", uniq_name = "_QFarraytestEmyarray"}
    %2 = fir.shape_shift %c-5, %c11 : (index, index) -> !fir.shapeshift<1>
    %3 = fir.declare %1(%2) {uniq_name = "_QFarraytestEmyarray"} : (!fir.ref<!fir.array<11xi32>>, !fir.shapeshift<1>) -> !fir.ref<!fir.array<11xi32>>
    %4 = fir.embox %3(%2) : (!fir.ref<!fir.array<11xi32>>, !fir.shapeshift<1>) -> !fir.box<!fir.array<11xi32>>
    %5 = fir.alloca i32 {bindc_name = "rv", uniq_name = "_QFarraytestErv"}
    %6 = fir.declare %5 {uniq_name = "_QFarraytestErv"} : (!fir.ref<i32>) -> !fir.ref<i32>
    %7 = fir.array_coor %3(%2) %c1 : (!fir.ref<!fir.array<11xi32>>, !fir.shapeshift<1>, index) -> !fir.ref<i32>
    fir.store %c3_i32 to %7 : !fir.ref<i32>
    %8 = fir.array_coor %3(%2) %c1 : (!fir.ref<!fir.array<11xi32>>, !fir.shapeshift<1>, index) -> !fir.ref<i32>
    %9 = fir.load %8 : !fir.ref<i32>
    fir.store %9 to %6 : !fir.ref<i32>
    %10 = fir.load %6 : !fir.ref<i32>
    return %10 : i32
  }
}
