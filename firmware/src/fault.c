/**
 * @file fault.c
 * @brief Fault recorder: says where a crash happened, on RTT, before and
 *        after the reset.
 *
 * The Zigbee stack's own fault handler (legacy HAL faults.s) records a crash
 * in halCrashInfo and resets with "Reset info: 0x0A (FLT)", but it only prints
 * the details on a UART, which this board does not use, and it keeps MMFAR,
 * not BFAR (separate registers on the Cortex-M33).
 *
 * fault_init() copies the vector table to RAM and points the HardFault,
 * MemManage, BusFault, UsageFault and SecureFault vectors at fault_entry():
 *   1. it records PC, LR, xPSR (stacked by the core), CFSR, HFSR, BFAR, MMFAR
 *      in a .noinit record that survives the reset;
 *   2. it prints them on RTT at once and waits APP_FAULT_PRINT_DELAY_MS so the
 *      probe can read them;
 *   3. it jumps to the stack's handler, which records its own data and resets
 *      (a HardFault too: the SDK's startup file left it on Default_Handler, an
 *      endless loop).
 * At the next boot fault_report() prints the record again, with the stack's
 * probable return addresses (the call chain), then clears it.
 *
 * Nothing here writes flash: the RAM table and the record are in RAM only.
 * The debug interface, the bootloader and the lock bits are not touched.
 */
#include "fault.h"
#include <stdint.h>
#include <string.h>
#include "em_device.h"
#include "SEGGER_RTT.h"
#include "app/framework/include/af.h"
#include "app_config.h"

/* The stack's crash record (legacy HAL diagnostic.c, .noinit), filled by its
 * fault handler before the reset. */
extern HalCrashInfoType halCrashInfo;

#define VECTOR_COUNT     (16u + EXT_IRQ_COUNT)
#define VECTOR_TABLE_ALIGN  512u   /* table size (92 x 4 bytes) rounded up to 2^n */
_Static_assert(VECTOR_COUNT * 4u <= VECTOR_TABLE_ALIGN, "vector table alignment");

#define FAULT_MAGIC      0xFA017C0Du

/* Exception numbers (IPSR) of the vectors this file takes over. */
#define EXC_HARDFAULT    3u
#define EXC_MEMMANAGE    4u
#define EXC_BUSFAULT     5u
#define EXC_USAGEFAULT   6u
#define EXC_SECUREFAULT  7u

typedef struct {
  uint32_t magic;
  uint32_t exception;   /* IPSR: 3 hard, 4 memmanage, 5 bus, 6 usage, 7 secure */
  uint32_t pc;          /* instruction that faulted (stacked PC) */
  uint32_t lr;          /* stacked LR: where the faulting function returns to */
  uint32_t xpsr;
  uint32_t exc_return;
  uint32_t cfsr;
  uint32_t hfsr;
  uint32_t bfar;
  uint32_t mmfar;
  uint32_t sp;
} fault_record_t;

static fault_record_t record __attribute__((section(".noinit")));

static uint32_t ram_vectors[VECTOR_COUNT] __attribute__((aligned(VECTOR_TABLE_ALIGN)));

/* Where fault_entry() goes next, by exception number (read from assembly). */
__attribute__((used)) uint32_t app_fault_next[16];

/* ---- formatting without printf (safe in a fault handler) ---------------- */

static char *put_str(char *p, const char *s)
{
  while (*s != '\0') {
    *p++ = *s++;
  }
  return p;
}

static char *put_hex(char *p, uint32_t v)
{
  static const char digits[] = "0123456789ABCDEF";
  *p++ = '0';
  *p++ = 'x';
  for (int shift = 28; shift >= 0; shift -= 4) {
    *p++ = digits[(v >> (uint32_t)shift) & 0xFu];
  }
  return p;
}

static const char *kind_of(uint32_t exception, uint32_t cfsr)
{
  if (exception == EXC_HARDFAULT) {
    /* Escalated faults keep their CFSR bits. */
    if ((cfsr & SCB_CFSR_BUSFAULTSR_Msk) != 0u) {
      return "HardFault (from BusFault)";
    }
    if ((cfsr & SCB_CFSR_MEMFAULTSR_Msk) != 0u) {
      return "HardFault (from MemManage)";
    }
    if ((cfsr & SCB_CFSR_USGFAULTSR_Msk) != 0u) {
      return "HardFault (from UsageFault)";
    }
    return "HardFault";
  }
  switch (exception) {
    case EXC_MEMMANAGE:   return "MemManage";
    case EXC_BUSFAULT:    return "BusFault";
    case EXC_USAGEFAULT:  return "UsageFault";
    case EXC_SECUREFAULT: return "SecureFault";
    default:              return "fault";
  }
}

static uint32_t format_record(char *buf, const fault_record_t *r, const char *title)
{
  char *p = buf;
  p = put_str(p, title);
  p = put_str(p, kind_of(r->exception, r->cfsr));
  p = put_str(p, "\nFAULT: PC=");
  p = put_hex(p, r->pc);
  p = put_str(p, " LR=");
  p = put_hex(p, r->lr);
  p = put_str(p, " xPSR=");
  p = put_hex(p, r->xpsr);
  p = put_str(p, " SP=");
  p = put_hex(p, r->sp);
  p = put_str(p, "\nFAULT: CFSR=");
  p = put_hex(p, r->cfsr);
  p = put_str(p, " HFSR=");
  p = put_hex(p, r->hfsr);
  p = put_str(p, " BFAR=");
  p = put_hex(p, r->bfar);
  p = put_str(p, ((r->cfsr & SCB_CFSR_BFARVALID_Msk) != 0u) ? " (valid)" : " (not valid)");
  p = put_str(p, " MMFAR=");
  p = put_hex(p, r->mmfar);
  p = put_str(p, ((r->cfsr & SCB_CFSR_MMARVALID_Msk) != 0u) ? " (valid)" : " (not valid)");
  p = put_str(p, " EXC_RETURN=");
  p = put_hex(p, r->exc_return);
  p = put_str(p, "\n");
  return (uint32_t)(p - buf);
}

/* ---- the handler ---------------------------------------------------------- */

/* Called from fault_entry() with interrupts masked. saved[] holds R0-R3, R12
 * and EXC_RETURN as pushed by fault_entry(); the core's exception frame is
 * just above them (MSP) or on the PSP. */
__attribute__((used)) void app_fault_record(const uint32_t *saved)
{
  uint32_t exc_return = saved[5];
  const uint32_t *frame = ((exc_return & 4u) != 0u)
                          ? (const uint32_t *)__get_PSP()
                          : saved + 6;

  record.exception = __get_IPSR() & 0x1FFu;
  record.exc_return = exc_return;
  record.sp = (uint32_t)frame;
  record.cfsr = SCB->CFSR;
  record.hfsr = SCB->HFSR;
  record.bfar = SCB->BFAR;
  record.mmfar = SCB->MMFAR;
  /* Read the stacked registers only if the frame is in RAM: a bad stack
   * pointer must not fault again inside the fault handler. */
  uint32_t f = (uint32_t)frame;
  if (f >= SRAM_BASE && f <= (SRAM_BASE + SRAM_SIZE - 32u) && (f & 3u) == 0u) {
    record.lr = frame[5];
    record.pc = frame[6];
    record.xpsr = frame[7];
  } else {
    record.lr = 0u;
    record.pc = 0u;
    record.xpsr = 0u;
  }
  record.magic = FAULT_MAGIC;

  char buf[320];
  uint32_t n = format_record(buf, &record, "\nFAULT: ");
  n += (uint32_t)(put_str(buf + n, "FAULT: resetting (the stack's handler records it as FLT)\n")
                  - (buf + n));
  (void)SEGGER_RTT_Write(0, buf, n);

  /* Give the probe time to read the RTT buffer before the reset. Busy wait:
   * no interrupt or timer can run here. About 4 cycles per iteration. */
  SystemCoreClockGet();
  uint32_t loops = (SystemCoreClock / 4000u) * APP_FAULT_PRINT_DELAY_MS;
  for (volatile uint32_t i = 0; i < loops; i++) {
  }
}

/* Saves the caller-saved registers, records, restores them exactly (the
 * stack's handler reads R0-R12, the SP and the stacked frame), then jumps to
 * app_fault_next[IPSR]. */
__attribute__((naked)) static void fault_entry(void)
{
  __asm volatile (
    "cpsid i                      \n"
    "push  {r0-r3, r12, lr}       \n"
    "mov   r0, sp                 \n"
    "bl    app_fault_record       \n"
    "pop   {r0-r3, r12, lr}       \n"
    "push  {r0, r1}               \n"
    "mrs   r0, ipsr               \n"
    "and   r0, r0, #15            \n"
    "ldr   r1, =app_fault_next    \n"
    "ldr   r12, [r1, r0, lsl #2]  \n"
    "pop   {r0, r1}               \n"
    "bx    r12                    \n"
    ".ltorg                       \n"
    );
}

/* ---- public ----------------------------------------------------------------- */

void fault_init(void)
{
  const uint32_t *flash_vectors = (const uint32_t *)SCB->VTOR;
  memcpy(ram_vectors, flash_vectors, sizeof(ram_vectors));
  for (uint32_t i = 0; i < 16u; i++) {
    app_fault_next[i] = flash_vectors[i];
  }
  /* The stack's BusFault_Handler is a plain branch to its common fault
   * routine (record + reset): HardFault goes there too instead of the
   * startup file's endless Default_Handler loop. */
  app_fault_next[EXC_HARDFAULT] = flash_vectors[EXC_BUSFAULT];

  ram_vectors[EXC_HARDFAULT] = (uint32_t)fault_entry;
  ram_vectors[EXC_MEMMANAGE] = (uint32_t)fault_entry;
  ram_vectors[EXC_BUSFAULT] = (uint32_t)fault_entry;
  ram_vectors[EXC_USAGEFAULT] = (uint32_t)fault_entry;
  ram_vectors[EXC_SECUREFAULT] = (uint32_t)fault_entry;

  __disable_irq();
  SCB->VTOR = (uint32_t)ram_vectors;
  __DSB();
  __ISB();
  __enable_irq();
}

void fault_report(void)
{
  if (record.magic != FAULT_MAGIC) {
    return;
  }
  char buf[320];
  uint32_t n = format_record(buf, &record, "Last reset was a ");
  buf[n] = '\0';
  /* sl_zigbee_app_debug_println adds its own line end. */
  if (n > 0u && buf[n - 1u] == '\n') {
    buf[n - 1u] = '\0';
  }
  sl_zigbee_app_debug_println("%s", buf);
  if (halResetWasCrash()) {
    /* Probable return addresses found on the stack by the stack's handler
     * (newest first): the call chain to look up in the .map / addr2line. */
    sl_zigbee_app_debug_print("FAULT: call chain:");
    for (uint32_t i = 0; i < NUM_RETURNS; i++) {
      if (halCrashInfo.returns[i] != 0u) {
        sl_zigbee_app_debug_print(" 0x%08lX", (unsigned long)halCrashInfo.returns[i]);
      }
    }
    sl_zigbee_app_debug_println("");
  }
  record.magic = 0u;
}
