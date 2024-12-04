#include "lib_state_machine.h"
#include "bm_config.h"

/*!
  Initialize the state machine

  \param[in] ctx Pointer to the state machine context.
  \param[in] init_state Pointer to the state to start the state machine at.
  \param[in] checkTransitionsForNextState Pointer to the function which will check for state transitions.
  \return N/A
*/
BmErr lib_sm_init(LibSmContext *ctx, const LibSmState *init_state,
                  CheckTransitionsForNextState check_transitions_for_next_state,
                  const char *name) {
  BmErr err = BmEINVAL;
  if (check_transitions_for_next_state && ctx && init_state && name) {
    ctx->current_state = init_state;
    ctx->check_transitions_for_next_state = check_transitions_for_next_state;
    ctx->name = name;
    err = BmOK;
  }
  return err;
}

/*!
  @brief Run an iteration of the state machine

  @details This API runs through all state transitions and will print out
           any errors that might have occurred in transitions, or while
           running the current state

  @param[in] ctx Pointer to the state machine context.
  @return BmOK if all things 
*/
BmErr lib_sm_run(LibSmContext *ctx) {
  BmErr err = BmEINVAL;
  BmErr run_err = BmOK;
  BmErr exit_err = BmOK;
  BmErr enter_err = BmOK;

  if (ctx->current_state && ctx->current_state->run) {
    bm_err_report_print(run_err, ctx->current_state->run(),
                        "running current state: %s of %s",
                        ctx->current_state->state_name, ctx->name);
    err = BmENODEV;
    const LibSmState *next_state =
        ctx->check_transitions_for_next_state(ctx->current_state->state_enum);
    if (next_state) {
      err = BmOK;
      if (ctx->current_state != next_state) {
        if (ctx->current_state->on_state_exit) {
          bm_err_report_print(exit_err, ctx->current_state->on_state_exit(),
                              "exiting current state: %s of %s",
                              ctx->current_state->state_name, ctx->name);
        }
        ctx->current_state = next_state;
        if (ctx->current_state->on_state_entry) {
          bm_err_report_print(enter_err, ctx->current_state->on_state_entry(),
                              "entering current state: %s of %s",
                              ctx->current_state->state_name, ctx->name);
        }
      }
    }
  }
  return err;
}

/*!
  Returns the current state's string name.

  \param[in] ctx Pointer to the state machine context.
  \return The name of the current state.
*/
const char *lib_sm_get_current_state_name(const LibSmContext *ctx) {
  return ctx->current_state->state_name;
}

/*!
  Returns the current state's string name.

  \param[in] ctx Pointer to the state machine context.
  \return The name of the current state.
*/
uint8_t get_current_state_enum(const LibSmContext *ctx) {
  return ctx->current_state->state_enum;
}
