uintptr_t __guest_get_tp(void);
static inline uintptr_t __get_tp(void) { return __guest_get_tp(); }

#define MC_PC gregs[REG_RIP]
