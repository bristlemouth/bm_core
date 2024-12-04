#ifndef __LIB_STATE_MACHINE_H__
#define __LIB_STATE_MACHINE_H__

#include "util.h"
#include <stdint.h>

typedef struct {
  uint8_t state_enum;     // Should match to an ENUM corresponding to state.
  const char *state_name; // MUST NOT BE NULL
  BmErr (*run)(void);     // MUST NOT BE NULL
  BmErr (*on_state_exit)(void);  // Null or function pointer
  BmErr (*on_state_entry)(void); // Null or function pointer
} LibSmState;

typedef const LibSmState *(*CheckTransitionsForNextState)(uint8_t);

typedef struct {
  const LibSmState *current_state;
  CheckTransitionsForNextState check_transitions_for_next_state;
  const char *name;
} LibSmContext;

BmErr lib_sm_init(LibSmContext *ctx, const LibSmState *init_state,
                  CheckTransitionsForNextState check_transitions_for_next_state,
                  const char *name);
BmErr lib_sm_run(LibSmContext *ctx);
const char *lib_sm_get_current_state_name(const LibSmContext *ctx);
uint8_t get_current_state_enum(const LibSmContext *ctx);

#endif // __LIB_STATE_MACHINE_H__
