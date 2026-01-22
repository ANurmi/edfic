module edfic_top #(
    parameter  int unsigned NrIrqs     = 4,
    parameter  int unsigned TsWidth    = 24,
    // Set clipping of TS to exchange bits for granularity
    parameter  int unsigned TsClip     = 0,
    parameter  int unsigned BaseAddr   = 0,
    localparam int unsigned OutTsWidth = TsWidth + TsClip,
    localparam int unsigned IdWidth    = $clog2(NrIrqs)
) (
    input logic clk_i,
    input logic rst_ni,
    // csr in interface
    // this controls interrupt deadlines
    input  logic        cfg_req_i,
    input  logic        cfg_we_i,
    input  logic [31:0] cfg_addr_i,
    input  logic [31:0] cfg_wdata_i,
    output logic [31:0] cfg_rdata_o,
    input  logic [63:0] mtime_i,
    // interrupt inputs
    input  logic [NrIrqs-1:0] irq_i,
    // current arbitration winner
    output logic [IdWidth-1:0] irq_id_o,
    input  logic [IdWidth-1:0] irq_id_i,
    output logic [OutTsWidth-1:0] irq_dl_o,
    // there is an active interrupt
    output logic irq_valid_o,
    input  logic irq_ack_i
);

typedef struct packed {
  logic ip;
  logic ie;
  logic trig_type;
  logic trig_pol;
  logic [TsWidth-2:0] dl; // zero-extended
  logic [TsWidth-1:0] ts;
} line_t;

line_t [NrIrqs-1:0] lines_d, lines_q;

logic [IdWidth-1:0] local_addr;
assign local_addr = cfg_addr_i[(IdWidth+2)-1:2];

logic [TsWidth-1:0] irq_dl_arb;

logic read_event, write_event;
assign read_event  = cfg_req_i & ~cfg_we_i;
assign write_event = cfg_req_i &  cfg_we_i;

logic [NrIrqs-1:0]              trig_type;
logic [NrIrqs-1:0]              trig_polarity;
logic [NrIrqs-1:0]              valid;
logic [NrIrqs-1:0]              ip;
logic [NrIrqs-1:0]              gated;
logic [NrIrqs-1:0][TsWidth-1:0] ts;

// Interrupt must be pended and enabled to be valid
for (genvar i=0;i<NrIrqs;i++) begin
  assign ip[i]            = lines_q[i].ip;
  assign valid[i]         = lines_q[i].ie & lines_q[i].ip;
  assign ts[i]            = lines_q[i].ts - mtime_i[(TsWidth-1)+TsClip:TsClip]; // make relative for comparison
  assign trig_type[i]     = lines_q[i].trig_type;
  assign trig_polarity[i] = lines_q[i].trig_pol;
end


always_ff @(posedge clk_i or negedge rst_ni) begin
  if (~rst_ni) begin
    lines_q <= '0;
  end else begin
    lines_q <= lines_d;
  end
end

always_comb begin : main_comb

  lines_d = lines_q;
  cfg_rdata_o = 32'h0;

  if (write_event) begin
    lines_d[local_addr].dl = cfg_wdata_i[(TsWidth+8-1)-1:8];
    lines_d[local_addr].trig_pol  = cfg_wdata_i[3];
    lines_d[local_addr].trig_type = cfg_wdata_i[2];
    lines_d[local_addr].ip = cfg_wdata_i[1];
    lines_d[local_addr].ie = cfg_wdata_i[0];
  end
  else if (read_event) begin
    cfg_rdata_o = {
      1'b0,
      lines_q[local_addr].dl,
      4'b0,
      lines_q[local_addr].trig_pol,
      lines_q[local_addr].trig_type,
      lines_q[local_addr].ip,
      lines_q[local_addr].ie
      };
  end

  // React to external interrupt from gateway
  if (|gated) begin
    for(int i=0; i<NrIrqs; i++) begin
      if(gated[i]) begin
        lines_d[i].ip = 1'b1;
        lines_d[i].ts = {1'b0, lines_q[i].dl} + mtime_i[(TsWidth-1)+TsClip:TsClip];
      end
    end
  end

  // Claim acknowledged interrupt
  if (irq_ack_i) begin
    lines_d[irq_id_i].ip = 1'b0;
  end

end : main_comb

edfic_arbiter #(
  .NrInputs (NrIrqs),
  .PrioWidth(TsWidth)
) i_arb (
  .valid_i (valid),
  .valid_o (irq_valid_o),
  .prio_i  (ts),
  .prio_o  (irq_dl_arb),
  .idx_o   (irq_id_o)
);

edfic_gateway #(
  .NrInputs (NrIrqs)
) i_gw (
  .clk_i,
  .rst_ni,
  .trig_polarity_i (trig_polarity),
  .trig_type_i     (trig_type),
  .ip_i            (ip),
  .irqs_i          (irq_i),
  .irqs_o          (gated)
);

if (TsClip == 0) begin
  assign irq_dl_o = irq_dl_arb;
end else begin
  assign irq_dl_o = {irq_dl_arb, (TsClip'(0))};
end

endmodule : edfic_top

