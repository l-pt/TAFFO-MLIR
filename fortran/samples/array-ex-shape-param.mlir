module attributes {dlti.dl_spec = #dlti.dl_spec<!llvm.ptr<270> = dense<32> : vector<4xi64>, !llvm.ptr<271> = dense<32> : vector<4xi64>, !llvm.ptr<272> = dense<64> : vector<4xi64>, i64 = dense<64> : vector<2xi64>, i128 = dense<128> : vector<2xi64>, f80 = dense<128> : vector<2xi64>, !llvm.ptr = dense<64> : vector<4xi64>, i1 = dense<8> : vector<2xi64>, i8 = dense<8> : vector<2xi64>, i16 = dense<16> : vector<2xi64>, i32 = dense<32> : vector<2xi64>, f16 = dense<16> : vector<2xi64>, f64 = dense<64> : vector<2xi64>, f128 = dense<128> : vector<2xi64>, "dlti.endianness" = "little", "dlti.mangling_mode" = "e", "dlti.stack_alignment" = 128 : i64>, fir.defaultkind = "a1c4d8i4l4r4", fir.kindmap = "", llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.ident = "flang version 21.0.0", llvm.target_triple = "x86_64-unknown-linux-gnu"} {
  func.func @_QPfoo(%arg0: !fir.ref<i32> {fir.bindc_name = "n"}, %arg1: !fir.ref<i32> {fir.bindc_name = "m"}, %arg2: !fir.ref<!fir.array<?x?xi32>> {fir.bindc_name = "a"}) {
    %c1 = arith.constant 1 : index
    %c42_i32 = arith.constant 42 : i32
    %c0 = arith.constant 0 : index
    %0 = fir.dummy_scope : !fir.dscope
    %1 = fir.declare %arg0 dummy_scope %0 {fortran_attrs = #fir.var_attrs<intent_in>, uniq_name = "_QFfooEn"} : (!fir.ref<i32>, !fir.dscope) -> !fir.ref<i32>
    %2 = fir.declare %arg1 dummy_scope %0 {fortran_attrs = #fir.var_attrs<intent_in>, uniq_name = "_QFfooEm"} : (!fir.ref<i32>, !fir.dscope) -> !fir.ref<i32>
    %3 = fir.load %1 : !fir.ref<i32>
    %4 = fir.convert %3 : (i32) -> i64
    %5 = fir.convert %4 : (i64) -> index
    %6 = arith.cmpi sgt, %5, %c0 : index
    %7 = arith.select %6, %5, %c0 : index
    %8 = fir.load %2 : !fir.ref<i32>
    %9 = fir.convert %8 : (i32) -> i64
    %10 = fir.convert %9 : (i64) -> index
    %11 = arith.cmpi sgt, %10, %c0 : index
    %12 = arith.select %11, %10, %c0 : index
    %13 = fir.shape %7, %12 : (index, index) -> !fir.shape<2>
    %14 = fir.declare %arg2(%13) dummy_scope %0 {fortran_attrs = #fir.var_attrs<intent_inout>, uniq_name = "_QFfooEa"} : (!fir.ref<!fir.array<?x?xi32>>, !fir.shape<2>, !fir.dscope) -> !fir.ref<!fir.array<?x?xi32>>
    %15 = fir.embox %14(%13) : (!fir.ref<!fir.array<?x?xi32>>, !fir.shape<2>) -> !fir.box<!fir.array<?x?xi32>>
    %16 = fir.array_coor %14(%13) %c1, %c1 : (!fir.ref<!fir.array<?x?xi32>>, !fir.shape<2>, index, index) -> !fir.ref<i32>
    fir.store %c42_i32 to %16 : !fir.ref<i32>
    return
  }
  func.func @_QPtest() -> i32 {
    %0 = fir.alloca i32 {adapt.valuebyref}
    %1 = fir.alloca i32 {adapt.valuebyref}
    %c1 = arith.constant 1 : index
    %c4_i32 = arith.constant 4 : i32
    %c3_i32 = arith.constant 3 : i32
    %c4 = arith.constant 4 : index
    %c3 = arith.constant 3 : index
    %2 = fir.dummy_scope : !fir.dscope
    %3 = fir.alloca !fir.array<3x4xi32> {bindc_name = "a", uniq_name = "_QFtestEa"}
    %4 = fir.shape %c3, %c4 : (index, index) -> !fir.shape<2>
    %5 = fir.declare %3(%4) {uniq_name = "_QFtestEa"} : (!fir.ref<!fir.array<3x4xi32>>, !fir.shape<2>) -> !fir.ref<!fir.array<3x4xi32>>
    %6 = fir.alloca i32 {bindc_name = "test", uniq_name = "_QFtestEtest"}
    %7 = fir.declare %6 {uniq_name = "_QFtestEtest"} : (!fir.ref<i32>) -> !fir.ref<i32>
    fir.store %c3_i32 to %1 : !fir.ref<i32>
    %false = arith.constant false
    fir.store %c4_i32 to %0 : !fir.ref<i32>
    %false_0 = arith.constant false
    %8 = fir.convert %5 : (!fir.ref<!fir.array<3x4xi32>>) -> !fir.ref<!fir.array<?x?xi32>>
    fir.call @_QPfoo(%1, %0, %8) fastmath<contract> : (!fir.ref<i32>, !fir.ref<i32>, !fir.ref<!fir.array<?x?xi32>>) -> ()
    %9 = fir.array_coor %5(%4) %c1, %c1 : (!fir.ref<!fir.array<3x4xi32>>, !fir.shape<2>, index, index) -> !fir.ref<i32>
    %10 = fir.load %9 : !fir.ref<i32>
    fir.store %10 to %7 : !fir.ref<i32>
    %11 = fir.load %7 : !fir.ref<i32>
    return %11 : i32
  }
}
