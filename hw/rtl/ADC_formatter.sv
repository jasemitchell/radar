`timescale 1ns / 1ps
module ADC_formatter (
    input  logic        aclk, aresetn, capture,

    // from XADC (this block receives)
    input  logic [15:0] s_axis_tdata,
    input  logic [4:0]  s_axis_tid,
    input  logic        s_axis_tvalid,
    output logic        s_axis_tready,

    // to FFT (this block sends)
    output logic [31:0] m_axis_tdata,
    output logic        m_axis_tvalid,
    output logic        m_axis_tlast,
    input  logic        m_axis_tready
);

    localparam logic [4:0] TID_I = 5'h11;   // VAUX1
    localparam logic [4:0] TID_Q = 5'h19;   // VAUX9

    logic [15:0] i_hold;
    logic [9:0]  count;
    logic        capturing;
    logic        capture_prev;

    assign s_axis_tready = 1'b1;            // always accept from the XADC

    always_ff @(posedge aclk) begin
        if (!aresetn) begin
            capturing     <= 1'b0;
            capture_prev  <= 1'b0;
            count         <= '0;
            i_hold        <= '0;
            m_axis_tdata  <= '0;
            m_axis_tvalid <= 1'b0;
            m_axis_tlast  <= 1'b0;
        end else begin
            capture_prev  <= capture;
            m_axis_tvalid <= 1'b0;          // default: nothing sent this cycle
            m_axis_tlast  <= 1'b0;

            // rising edge of capture starts a frame
            if (capture && !capture_prev && !capturing) begin
                capturing <= 1'b1;
                count     <= '0;
            end

            if (s_axis_tvalid) begin
                // always keep the latest I, sign-flipped
                if (s_axis_tid == TID_I)
                    i_hold <= s_axis_tdata ^ 16'h8000;

                // Q completes a pair: send it if capturing
                if (s_axis_tid == TID_Q && capturing) begin
                    m_axis_tdata  <= {s_axis_tdata ^ 16'h8000, i_hold};
                    m_axis_tvalid <= 1'b1;
                    m_axis_tlast  <= (count == 10'd511);
                    count         <= count + 1;
                    if (count == 10'd511)
                        capturing <= 1'b0;  // frame done
                end
            end
        end
    end

endmodule