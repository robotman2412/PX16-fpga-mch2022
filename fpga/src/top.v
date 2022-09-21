
module top (
    input  wire      clk_in,
    input  wire      irq_n,
    
    input  wire      spi_mosi,
    output wire      spi_miso,
    input  wire      spi_clk,
    input  wire      spi_cs_n,
    
    input  wire[7:0] pmod,
    output wire      red_n,
    output wire      green_n,
    output wire      blue_n,
    
    inout  wire[3:0] ram_io,
    output wire      ram_clk,
    output wire      ram_cs_n
);
    
    reg[1:0] last_cmd;
    
    reg[15:0] clkdiv;
    reg[7:0]  spi_recv;
    reg[7:0]  spi_send;
    
    assign spi_miso = spi_cs_n ? 'bz : spi_send[7];
    
    assign red_n   = 1;
    assign green_n = 1;
    assign blue_n  = spi_cs_n;
    
    initial begin
        spi_cs_n_last  = 1;
        spi_bit        = 0;
        clkdiv         = 1;
        spi_byte_index = 0;
    end
    
    // SPI Send.
    reg       spi_cs_n_last;
    reg[2:0]  spi_bit;
    reg[7:0]  spi_byte_index;
    always @(posedge spi_clk | spi_cs_n) begin
        if (spi_cs_n) begin
            // Prepare send data.
            spi_bit        <= 0;
            // First byte: Bus peek data.
            spi_send       <= 0;
            spi_byte_index <= 0;
        end else if (spi_clk) begin
            if (spi_bit == 7) begin
                // More bytes: Bus peek data.
                spi_send       <= spi_byte_index + 1;
                spi_byte_index <= spi_byte_index + 1;
            end else begin
                // Move the send data.
                spi_send[7:1]  <= spi_send[6:0];
                spi_send[0]    <= 0;
            end
            spi_bit <= spi_bit + 1;
        end
        spi_cs_n_last = spi_cs_n;
    end
    
    // SPI Recv.
    always @(posedge spi_clk) begin
        if (spi_cs_n == 0) begin
            // Receive data.
            spi_recv[7:1] <= spi_recv[6:0];
            spi_recv[0]   <= spi_mosi;
        end
    end
    
endmodule
