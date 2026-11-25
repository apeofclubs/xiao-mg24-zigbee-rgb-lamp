/**
 * @file fault.h
 * @brief Fault recorder: a HardFault / MemManage / BusFault / UsageFault /
 *        SecureFault prints PC, LR, CFSR, BFAR on RTT, is kept across the
 *        reset and printed again at the next boot. RAM only, no flash write.
 */
#ifndef FAULT_H
#define FAULT_H

/** Takes over the fault vectors (RAM copy of the vector table). Call first
 *  in the application start-up. */
void fault_init(void);

/** Prints the last fault if the last reset was one, then forgets it. */
void fault_report(void);

#endif
