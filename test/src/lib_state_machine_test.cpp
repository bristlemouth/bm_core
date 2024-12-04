#include "gtest/gtest.h"

extern "C" {
#include "lib_state_machine.h"
}

/*
* An example of how to use lib_state_machine.
* You'll need a few things:
*
* 1. An enum strucure identifying your states and the number of states.
* 2. A collection of functions for each state
*     a. Run() a function to run over and over.
*     b. OnStateExit() a function to be called when exiting a state.
*     c. OnStateEntry() a function to be called when entering a state.
* 3. A static global state machine context variable.
* 4. A static const global structure of your states (statically defined)
* 5. A check transitions function to determine which state to run next.
*
* General flow is to hold a SM within a task.
* First initalize your state machine (lib_sm_init), then call lib_sm_run in an infinite loop to pump the state machine.
*
* The state machine operates as follows:
* 1. Run the current action associated with the state
* 2. Identify the next state that should be run.
* 3. On transition to a new state
*   a. call the current state's exit function if it exists.
*   b. update to the new state.
*   c. call the new state's entry function.
* 4. Go to Step 1.
*/

// ENUM describing the position of each state in the states array.
typedef enum {
  STAGE_1,
  STAGE_2,
  STAGE_3,
  STAGE_4,
  NUM_STAGES,
} stageEnum_t;

// State machine state functions forward declarations.
static BmErr stage1Run();
static BmErr stage2Run();
static BmErr stage2Exit();
static BmErr stage3Run();
static BmErr stage3Entry();
static BmErr stage4Run();

// Global context structure for the file.
// It's generally useful to encapsulate the state machine context so you can pair
// it with other variables, but not strictly necessary.
typedef struct {
  // State machine context
  LibSmContext sm_ctx;
  // Stage 1 vars
  uint8_t count_s1;
  // Stage 2 vars
  bool flag_s2;
  // Stage 3 vars
  bool flag_s3;
  // Stage N vars
  // ...
} GlobalContext;

static GlobalContext GLOBAL_CTX;
static const LibSmState states[NUM_STAGES] = {
    {
        .state_enum = STAGE_1,
        .state_name = "Stage1", // The name MUST NOT BE NULL
        .run = stage1Run,       // This function MUST NOT BE NULL
        .on_state_exit = NULL,  // This function can be NULL
        .on_state_entry = NULL, // This function can be NULL
    },
    {
        .state_enum = STAGE_2,
        .state_name = "Stage2",
        .run = stage2Run,
        .on_state_exit = stage2Exit,
        .on_state_entry = NULL,
    },
    {
        .state_enum = STAGE_3,
        .state_name = "Stage3",
        .run = stage3Run,
        .on_state_exit = NULL,
        .on_state_entry = stage3Entry,
    },
    {
        .state_enum = STAGE_4,
        .state_name = "Stage4",
        .run = stage4Run,
        .on_state_exit = NULL,
        .on_state_entry = NULL,
    },
};

// Transitions function
const LibSmState *checkTransitions(uint8_t current_state) {
  switch ((stageEnum_t)current_state) {
  case STAGE_1: {
    if (GLOBAL_CTX.count_s1 == 2) {
      return &states[STAGE_2];
    }
    return &states[STAGE_1];
  }
  case STAGE_2: {
    return &states[STAGE_3];
  }
  case STAGE_3: {
    return &states[STAGE_4];
  }
  case STAGE_4: {
    return &states[STAGE_1];
  }
  default:
    return NULL; // Bad if we get here - This will generate an ASSERT in lib_sm_run.
  }
}

// State function definitions.
static BmErr stage1Run() {
  GLOBAL_CTX.count_s1++;
  return BmOK;
}
static BmErr stage2Run() { return BmOK; }
static BmErr stage2Exit() {
  GLOBAL_CTX.flag_s2 = true;
  return BmOK;
}

static BmErr stage3Run() { return BmOK; }
static BmErr stage3Entry() {
  GLOBAL_CTX.flag_s3 = true;
  return BmOK;
}

static BmErr stage4Run() { return BmOK; }

// The fixture for testing class Foo.
class LibStateMachineTest : public ::testing::Test {
protected:
  // You can remove any or all of the following functions if its body
  // is empty.

  LibStateMachineTest() {
    // You can do set-up work for each test here.
  }

  ~LibStateMachineTest() override {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
    GLOBAL_CTX.count_s1 = 0;
    GLOBAL_CTX.flag_s2 = false;
    GLOBAL_CTX.flag_s3 = false;
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }

  // Objects declared here can be used by all tests in the test suite for Foo.
};

// Although lib_sm_run should be run in a for(;;) loop, while writing unit tests for our state machines
// We should seperate out each call to run() to more granularly test what's going on.
TEST_F(LibStateMachineTest, basic_pump) {
  // Check Initialization.
  lib_sm_init(&GLOBAL_CTX.sm_ctx, &states[STAGE_1], checkTransitions, "basic");
  EXPECT_TRUE(GLOBAL_CTX.sm_ctx.current_state == &states[STAGE_1]);
  EXPECT_TRUE((strcmp(lib_sm_get_current_state_name(&GLOBAL_CTX.sm_ctx),
                      "Stage1") == 0));

  // Run 1 count_s1 == 1
  lib_sm_run(&GLOBAL_CTX.sm_ctx);
  EXPECT_TRUE(GLOBAL_CTX.sm_ctx.current_state == &states[STAGE_1]);
  EXPECT_TRUE((strcmp(lib_sm_get_current_state_name(&GLOBAL_CTX.sm_ctx),
                      "Stage1") == 0));

  // Run 2 count_s1 == 2
  lib_sm_run(&GLOBAL_CTX.sm_ctx);
  EXPECT_TRUE(GLOBAL_CTX.sm_ctx.current_state == &states[STAGE_2]);
  EXPECT_TRUE((strcmp(lib_sm_get_current_state_name(&GLOBAL_CTX.sm_ctx),
                      "Stage2") == 0));

  // Run 3
  lib_sm_run(&GLOBAL_CTX.sm_ctx);
  EXPECT_TRUE(GLOBAL_CTX.flag_s2 == true);
  EXPECT_TRUE(GLOBAL_CTX.flag_s3 == true);
  EXPECT_TRUE(GLOBAL_CTX.sm_ctx.current_state == &states[STAGE_3]);
  EXPECT_TRUE((strcmp(lib_sm_get_current_state_name(&GLOBAL_CTX.sm_ctx),
                      "Stage3") == 0));

  // Run 4
  lib_sm_run(&GLOBAL_CTX.sm_ctx);
  EXPECT_TRUE(GLOBAL_CTX.sm_ctx.current_state == &states[STAGE_4]);
  EXPECT_TRUE((strcmp(lib_sm_get_current_state_name(&GLOBAL_CTX.sm_ctx),
                      "Stage4") == 0));

  // Run 5
  lib_sm_run(&GLOBAL_CTX.sm_ctx);
  EXPECT_TRUE(GLOBAL_CTX.sm_ctx.current_state == &states[STAGE_1]);
  EXPECT_TRUE((strcmp(lib_sm_get_current_state_name(&GLOBAL_CTX.sm_ctx),
                      "Stage1") == 0));
}

TEST_F(LibStateMachineTest, BadInit) {
  ASSERT_NE(lib_sm_init(&GLOBAL_CTX.sm_ctx, &states[STAGE_1], NULL, "bad_init"),
            BmOK);
}

TEST_F(LibStateMachineTest, BadContext) {
  LibSmContext ctx = {0};
  ASSERT_NE(lib_sm_run(&ctx), BmOK); // No init
}

// An invalid transition (Used for BadContext test)
const LibSmState *badTransition(uint8_t current_state) {
  (void)(current_state);
  return NULL;
}

TEST_F(LibStateMachineTest, BadTransition) {
  LibSmContext ctx = {0};
  ASSERT_EQ(
      lib_sm_init(&ctx, &states[STAGE_1], badTransition, "bad_transition"),
      BmOK);
  ASSERT_NE(lib_sm_run(&ctx), BmOK); // badTransition returns NULL
}

TEST_F(LibStateMachineTest, BadStateName) {
  LibSmContext ctx;
  LibSmState badNameState = {
      .state_enum = 0,
      .state_name = NULL,
      .run = stage1Run,
      .on_state_exit = NULL,
      .on_state_entry = NULL,
  };
  ASSERT_EQ(lib_sm_init(&ctx, &badNameState, checkTransitions, "bad_name"),
            BmOK);
  EXPECT_EQ(lib_sm_get_current_state_name(&ctx), nullptr);
}
