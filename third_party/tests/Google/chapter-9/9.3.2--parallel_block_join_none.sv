/*
:name: parallel_block_join_none
:description: parallel block check
:should_fail: 0
:tags: 9.3.2
*/
module parallel_tb ();
	reg a = 0;
	reg b = 0;
	reg c = 0;
	initial
		fork
			a = 1;
			b = 0;
			c = 1;
		join_none
endmodule
