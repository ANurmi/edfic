// Clock period in picoseconds, used for generating waveforms
const uint32_t CLOCK_PERIOD_PS = /* 100 MHz */ 10000;
const uint8_t  MTIME_PRESCALER = 3;
const uint8_t  IE_OFFS = 0;
const uint8_t  IP_OFFS = 1;
const uint8_t  NR_IRQS = 4;

typedef struct {
  uint8_t count;
  uint8_t value;
} prescaler_t;

typedef struct {
  bool running;
  prescaler_t ps;
} mtimer_t;

class VipEdfIc {
  public:
    Vedfic_top    *m_dut;
    VerilatedFstC* m_trace;
    uint64_t       m_tickcount;
    mtimer_t       mtimer;

    VipEdfIc(Vedfic_top* dut, const char* fst_name) :
        m_trace(NULL), m_tickcount(01) {
      m_dut = dut;
      mtimer.running = false;
      mtimer.ps.count = 0;
      mtimer.ps.value = 0;
      Verilated::traceEverOn(true);
      m_dut->clk_i = 0;
      m_dut->eval();
      open_trace(fst_name);
    }

    ~VipEdfIc(void){
      close_trace();
      m_dut = NULL;
    }

    void raise_reset(void){
      cycles(12);
      m_dut->rst_ni = 1;
      init_mtimer(MTIME_PRESCALER);
      cycles(12);
    }

    void configure_irq(uint8_t idx, uint16_t offs){
      cfg_write(idx*4, (uint32_t)(offs << 8));
      printf("[CFG_DRIVER] Set line %d to offset %d\n",
          idx, offs);
    }

    void set_pend_irq(uint8_t idx){
      const uint32_t line_addr = idx*4;
      uint32_t read_val = cfg_read(line_addr);
      uint32_t masked   = read_val | (1 << IP_OFFS);
      cfg_write(line_addr, masked);
      printf("[CFG_DRIVER] Set IP on line %d\n", idx);
    }

    void set_enable_irq(uint8_t idx){
      const uint32_t line_addr = idx*4;
      uint32_t read_val = cfg_read(line_addr);
      uint32_t masked   = read_val | (1 << IE_OFFS);
      cfg_write(line_addr, masked);
      printf("[CFG_DRIVER] Set IE on line %d\n", idx);
    }

    void delay(uint32_t count) {cycles(count);}

    void drive_rand_irqs(uint32_t tx_limit) {
      uint32_t tx_count = 0;
      while (tx_count < tx_limit) {

        m_dut->irq_i     = 0;
        m_dut->irq_ack_i = 0;
        m_dut->irq_id_i  = 0;

        // Acknowledge valid interrupt with variable latency
        if (m_dut->irq_valid_o & (rand() % 3 == 0)) {
          m_dut->irq_ack_i = 1;
          m_dut->irq_id_i = m_dut->irq_id_o;
          printf("[IRQ_DRIVER] Ack interrupt on line %d with dl %d at mtime %ld\n",
              m_dut->irq_id_o, m_dut->irq_dl_o, m_dut->mtime_i);
        }

        if (m_dut->irq_ack_i) {
        }

        if (rand() % 15 == 0){
          m_dut->irq_i = rand();
          for(uint32_t i=0; i<NR_IRQS; i++){
            if (m_dut->irq_i & (1 << i)) {
              printf("[IRQ_DRIVER] Line %d driven at mtime %ld\n",
                  i, m_dut->mtime_i);
            }
          }
          tx_count++;
        }

        cycles(1);
      }
    }

  private:
    void open_trace(const char* fst_name) {
      if (!m_trace) {
        m_trace = new VerilatedFstC;
        m_dut->trace(m_trace, 99);
        m_trace->open(fst_name);
      }
    }

    void close_trace(void) {
      if (m_trace) {
        m_trace->close();
        delete m_trace;
        m_trace = NULL;
      }
    }

    void tick(void) {
      m_tickcount++;
      m_dut->eval();
      if (m_trace) m_trace->dump((vluint64_t)(CLOCK_PERIOD_PS*m_tickcount-CLOCK_PERIOD_PS/5));
      m_dut->clk_i = 1;
      m_dut->eval();
      if (m_trace) m_trace->dump((vluint64_t)(CLOCK_PERIOD_PS*m_tickcount));
      m_dut->clk_i = 0;
      m_dut->eval();
      if (m_trace) {
        m_trace->dump((vluint64_t)(CLOCK_PERIOD_PS*m_tickcount+CLOCK_PERIOD_PS/2));
        m_trace->flush();
      }
    }

    void cycles(uint32_t num) {
      for (uint32_t i=0; i<num; i++) {
        if (mtimer.running) {
          if (mtimer.ps.count == mtimer.ps.value){
            m_dut->mtime_i++;
            mtimer.ps.count = 0;
          }
          else {
            mtimer.ps.count++;
          }
        }
        tick();
      }
    }

    void init_mtimer(uint8_t prescaler) {
      mtimer.running = true;
      mtimer.ps.value = prescaler;
    }

    void cfg_write(uint32_t addr, uint32_t wdata) {
      m_dut->cfg_addr_i  = addr;
      m_dut->cfg_wdata_i = wdata;
      m_dut->cfg_we_i    = 1;
      m_dut->cfg_req_i   = 1;
      cycles(1);
      m_dut->cfg_addr_i  = 0;
      m_dut->cfg_wdata_i = 0;
      m_dut->cfg_we_i    = 0;
      m_dut->cfg_req_i   = 0;
    }

    uint32_t cfg_read(uint32_t addr) {
      m_dut->cfg_addr_i  = addr;
      m_dut->cfg_we_i    = 0;
      m_dut->cfg_req_i   = 1;
      cycles(1);
      m_dut->cfg_addr_i  = 0;
      m_dut->cfg_we_i    = 0;
      m_dut->cfg_req_i   = 0;
      return m_dut->cfg_rdata_o;
    }

};
