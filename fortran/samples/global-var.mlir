module {
  //fir.global @answer : i32 {
  //  %c42 = arith.constant 42 : i32
  //  fir.has_value %c42 : i32
  //}
  //fir.global @myarray(dense<1.100000e+00> : tensor<32xf32>) : !fir.array<32xf32>
  fir.global @answer : i32
  fir.global @test : !fir.array<32xf32>
}
