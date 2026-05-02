#ifndef BUSSEC_POLICY_H
#define BUSSEC_POLICY_H

#include <stdbool.h>

typedef enum {
    BUSSEC_MODE_NORMAL = 0,
    BUSSEC_MODE_PROGRAM,
    BUSSEC_MODE_LOCKTEST,
    BUSSEC_MODE_RESEARCH_DEV
} bussec_mode_t;

typedef enum {
    BUSSEC_POLICY_ACTION_READ_ID = 0,
    BUSSEC_POLICY_ACTION_READ_STATUS,
    BUSSEC_POLICY_ACTION_DOCUMENTED_LOCKTEST_READ,
    BUSSEC_POLICY_ACTION_RESEARCH_SAFE_ANOMALY_SCAN,
    BUSSEC_POLICY_ACTION_RESEARCH_SACRIFICIAL_MALFORMED,
    BUSSEC_POLICY_ACTION_PROTECTED_READOUT,
    BUSSEC_POLICY_ACTION_UNDOCUMENTED_READOUT_PROBE,
    BUSSEC_POLICY_ACTION_DESTRUCTIVE_WRITE
} bussec_policy_action_t;

typedef enum {
    BUSSEC_POLICY_ALLOW = 0,
    BUSSEC_POLICY_DENY_CONFIRM_REQUIRED,
    BUSSEC_POLICY_DENY_WRONG_MODE,
    BUSSEC_POLICY_DENY_PROFILE_REQUIRED,
    BUSSEC_POLICY_DENY_BUILD_CONFIG,
    BUSSEC_POLICY_DENY_PROTECTED_READOUT,
    BUSSEC_POLICY_DENY_UNDOCUMENTED_READOUT
} bussec_policy_result_t;

typedef struct {
    bool owned_target_confirmed;
    bool sacrificial_target_confirmed;
    bool target_profile_known;
} bussec_policy_context_t;

bussec_policy_result_t bussec_policy_check(bussec_mode_t mode,
                                           bussec_policy_action_t action,
                                           bussec_policy_context_t context);
bool bussec_policy_result_allowed(bussec_policy_result_t result);
const char *bussec_policy_mode_name(bussec_mode_t mode);
const char *bussec_policy_action_name(bussec_policy_action_t action);
const char *bussec_policy_result_name(bussec_policy_result_t result);

#endif /* BUSSEC_POLICY_H */
