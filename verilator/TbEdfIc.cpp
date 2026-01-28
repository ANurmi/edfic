#include <iostream>
#include <stdint.h>

#include "Vedfic_top.h"
#include "verilated.h"
#include "verilated_fst_c.h"
#include "VipEdfIc.h"

int main(int argc, char** argv) {

  printf("### Starting EDF-IC testbench ####################\n");

  const char* wavePath = "../build/waveform.fst";
  Verilated::commandArgs(argc, argv);
  Vedfic_top* dut = new Vedfic_top;
  VipEdfIc*   vip = new VipEdfIc(dut, wavePath);

  vip->raise_reset();

  vip->configure_irq(0, 200);
  vip->configure_irq(1, 100);
  vip->configure_irq(2, 300);
  vip->configure_irq(3, 50);

  vip->delay(20);

  vip->set_pend_irq(1);
  vip->set_pend_irq(2);

  vip->set_enable_irq(3);
  vip->set_enable_irq(2);
  vip->set_enable_irq(1);
  vip->set_enable_irq(0);

  vip->drive_rand_irqs(10);

  delete vip;
  delete dut;

  printf("##################################################\n");
  printf("TB execution complete, wave: %s\n", wavePath);
  return 0;
}
