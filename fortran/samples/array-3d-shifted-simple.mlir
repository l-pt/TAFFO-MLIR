module attributes {dlti.dl_spec = #dlti.dl_spec<!llvm.ptr<270> = dense<32> : vector<4xi64>, !llvm.ptr<271> = dense<32> : vector<4xi64>, !llvm.ptr<272> = dense<64> : vector<4xi64>, i64 = dense<64> : vector<2xi64>, i128 = dense<128> : vector<2xi64>, f80 = dense<128> : vector<2xi64>, !llvm.ptr = dense<64> : vector<4xi64>, i1 = dense<8> : vector<2xi64>, i8 = dense<8> : vector<2xi64>, i16 = dense<16> : vector<2xi64>, i32 = dense<32> : vector<2xi64>, f16 = dense<16> : vector<2xi64>, f64 = dense<64> : vector<2xi64>, f128 = dense<128> : vector<2xi64>, "dlti.endianness" = "little", "dlti.mangling_mode" = "e", "dlti.stack_alignment" = 128 : i64>, fir.defaultkind = "a1c4d8i4l4r4", fir.kindmap = "", llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.ident = "flang version 21.0.0", llvm.target_triple = "x86_64-unknown-linux-gnu"} {
  func.func @_QParraytest() -> i32 {
    %c212_i32 = arith.constant 212 : i32
    %c9 = arith.constant 9 : index
    %c235_i32 = arith.constant 235 : i32
    %c7 = arith.constant 7 : index
    %c1 = arith.constant 1 : index
    %c213_i32 = arith.constant 213 : i32
    %c6 = arith.constant 6 : index
    %c122_i32 = arith.constant 122 : i32
    %c111_i32 = arith.constant 111 : i32
    %c5 = arith.constant 5 : index
    %c3 = arith.constant 3 : index
    %c-1 = arith.constant -1 : index
    %c2 = arith.constant 2 : index
    %c0 = arith.constant 0 : index
    %0 = fir.dummy_scope : !fir.dscope
    %1 = fir.alloca !fir.array<2x3x5xi32> {bindc_name = "myarray", uniq_name = "_QFarraytestEmyarray"}
    %2 = fir.shape_shift %c0, %c2, %c-1, %c3, %c5, %c5 : (index, index, index, index, index, index) -> !fir.shapeshift<3>
    %3 = fir.declare %1(%2) {uniq_name = "_QFarraytestEmyarray"} : (!fir.ref<!fir.array<2x3x5xi32>>, !fir.shapeshift<3>) -> !fir.ref<!fir.array<2x3x5xi32>>
    %4 = fir.embox %3(%2) : (!fir.ref<!fir.array<2x3x5xi32>>, !fir.shapeshift<3>) -> !fir.box<!fir.array<2x3x5xi32>>
    %5 = fir.alloca i32 {bindc_name = "rv", uniq_name = "_QFarraytestErv"}
    %6 = fir.declare %5 {uniq_name = "_QFarraytestErv"} : (!fir.ref<i32>) -> !fir.ref<i32>
    %7 = fir.array_coor %3(%2) %c0, %c-1, %c5 : (!fir.ref<!fir.array<2x3x5xi32>>, !fir.shapeshift<3>, index, index, index) -> !fir.ref<i32>
    fir.store %c111_i32 to %7 : !fir.ref<i32>
    %8 = fir.array_coor %3(%2) %c0, %c0, %c6 : (!fir.ref<!fir.array<2x3x5xi32>>, !fir.shapeshift<3>, index, index, index) -> !fir.ref<i32>
    fir.store %c122_i32 to %8 : !fir.ref<i32>
    %9 = fir.array_coor %3(%2) %c1, %c-1, %c7 : (!fir.ref<!fir.array<2x3x5xi32>>, !fir.shapeshift<3>, index, index, index) -> !fir.ref<i32>
    fir.store %c213_i32 to %9 : !fir.ref<i32>
    %10 = fir.array_coor %3(%2) %c1, %c1, %c9 : (!fir.ref<!fir.array<2x3x5xi32>>, !fir.shapeshift<3>, index, index, index) -> !fir.ref<i32>
    fir.store %c235_i32 to %10 : !fir.ref<i32>
    %11 = fir.array_coor %3(%2) %c1, %c-1, %c6 : (!fir.ref<!fir.array<2x3x5xi32>>, !fir.shapeshift<3>, index, index, index) -> !fir.ref<i32>
    fir.store %c212_i32 to %11 : !fir.ref<i32>
    %12 = fir.array_coor %3(%2) %c1, %c-1, %c6 : (!fir.ref<!fir.array<2x3x5xi32>>, !fir.shapeshift<3>, index, index, index) -> !fir.ref<i32>
    %13 = fir.load %12 : !fir.ref<i32>
    fir.store %13 to %6 : !fir.ref<i32>
    %14 = fir.load %6 : !fir.ref<i32>
    return %14 : i32
  }
}
