/*
:name: 22.5.1--define_expansion_4
:description: Test
:should_fail: 0
:tags: 22.5.1
:type: preprocessing
*/
`define D(x,y) initial $display("start", x , y, "end");
module top ();
`D(,)
endmodule
