// File: design.v
// Generated by MyHDL 0.8
// Date: Tue Dec  3 04:33:14 2013


`timescale 1ns/10ps

module top (
    x,clk,rst,a
);

output x;
reg x;
input clk;
input rst;
input [1:0] a;

always @(posedge clk, negedge rst) begin: DESIGN_PROCESSOR
    reg i;
    if (!rst) begin
        i = 0;
        x = 0;
    end
    else begin
        case (a)
            2'b00: begin
				x = 0;
                i = 0;
            end
            2'b01: begin
                x = i;
            end
            2'b10: begin
                i = 1;
            end
            2'b11: begin
                i = 0;
            end
            default: begin
                x = 0;
                i = 0;
            end
        endcase
    end
end

endmodule
