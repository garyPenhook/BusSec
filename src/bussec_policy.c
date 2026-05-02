#include "bussec_policy.h"

#ifndef BUSSEC_ENABLE_DESTRUCTIVE_RESEARCH
#define BUSSEC_ENABLE_DESTRUCTIVE_RESEARCH (0)
#endif

bussec_policy_result_t bussec_policy_check(bussec_mode_t mode,
                                           bussec_policy_action_t action,
                                           bussec_policy_context_t context)
{
    switch (action) {
    case BUSSEC_POLICY_ACTION_READ_ID:
    case BUSSEC_POLICY_ACTION_READ_STATUS:
        return BUSSEC_POLICY_ALLOW;

    case BUSSEC_POLICY_ACTION_DOCUMENTED_LOCKTEST_READ:
        if (mode != BUSSEC_MODE_LOCKTEST) {
            return BUSSEC_POLICY_DENY_WRONG_MODE;
        }
        if (!context.owned_target_confirmed) {
            return BUSSEC_POLICY_DENY_CONFIRM_REQUIRED;
        }
        if (!context.target_profile_known) {
            return BUSSEC_POLICY_DENY_PROFILE_REQUIRED;
        }
        return BUSSEC_POLICY_ALLOW;

    case BUSSEC_POLICY_ACTION_RESEARCH_SAFE_ANOMALY_SCAN:
    case BUSSEC_POLICY_ACTION_RESEARCH_SACRIFICIAL_MALFORMED:
        if (mode != BUSSEC_MODE_RESEARCH_DEV) {
            return BUSSEC_POLICY_DENY_WRONG_MODE;
        }
        if (!context.sacrificial_target_confirmed) {
            return BUSSEC_POLICY_DENY_CONFIRM_REQUIRED;
        }
        return BUSSEC_POLICY_ALLOW;

    case BUSSEC_POLICY_ACTION_PROTECTED_READOUT:
        return BUSSEC_POLICY_DENY_PROTECTED_READOUT;

    case BUSSEC_POLICY_ACTION_UNDOCUMENTED_READOUT_PROBE:
        return BUSSEC_POLICY_DENY_UNDOCUMENTED_READOUT;

    case BUSSEC_POLICY_ACTION_DESTRUCTIVE_WRITE:
        if (BUSSEC_ENABLE_DESTRUCTIVE_RESEARCH == 0) {
            return BUSSEC_POLICY_DENY_BUILD_CONFIG;
        }
        if (mode != BUSSEC_MODE_PROGRAM) {
            return BUSSEC_POLICY_DENY_WRONG_MODE;
        }
        if (!context.owned_target_confirmed) {
            return BUSSEC_POLICY_DENY_CONFIRM_REQUIRED;
        }
        return BUSSEC_POLICY_ALLOW;

    default:
        return BUSSEC_POLICY_DENY_BUILD_CONFIG;
    }
}

bool bussec_policy_result_allowed(bussec_policy_result_t result)
{
    return result == BUSSEC_POLICY_ALLOW;
}

const char *bussec_policy_mode_name(bussec_mode_t mode)
{
    switch (mode) {
    case BUSSEC_MODE_NORMAL:
        return "normal";
    case BUSSEC_MODE_PROGRAM:
        return "program";
    case BUSSEC_MODE_LOCKTEST:
        return "locktest";
    case BUSSEC_MODE_RESEARCH_DEV:
        return "research_dev";
    default:
        return "unknown";
    }
}

const char *bussec_policy_action_name(bussec_policy_action_t action)
{
    switch (action) {
    case BUSSEC_POLICY_ACTION_READ_ID:
        return "read-id";
    case BUSSEC_POLICY_ACTION_READ_STATUS:
        return "read-status";
    case BUSSEC_POLICY_ACTION_DOCUMENTED_LOCKTEST_READ:
        return "documented-locktest-read";
    case BUSSEC_POLICY_ACTION_RESEARCH_SAFE_ANOMALY_SCAN:
        return "research-safe-anomaly-scan";
    case BUSSEC_POLICY_ACTION_RESEARCH_SACRIFICIAL_MALFORMED:
        return "research-sacrificial-malformed";
    case BUSSEC_POLICY_ACTION_PROTECTED_READOUT:
        return "protected-readout";
    case BUSSEC_POLICY_ACTION_UNDOCUMENTED_READOUT_PROBE:
        return "undocumented-readout-probe";
    case BUSSEC_POLICY_ACTION_DESTRUCTIVE_WRITE:
        return "destructive-write";
    default:
        return "unknown";
    }
}

const char *bussec_policy_result_name(bussec_policy_result_t result)
{
    switch (result) {
    case BUSSEC_POLICY_ALLOW:
        return "allow";
    case BUSSEC_POLICY_DENY_CONFIRM_REQUIRED:
        return "deny-confirm-required";
    case BUSSEC_POLICY_DENY_WRONG_MODE:
        return "deny-wrong-mode";
    case BUSSEC_POLICY_DENY_PROFILE_REQUIRED:
        return "deny-profile-required";
    case BUSSEC_POLICY_DENY_BUILD_CONFIG:
        return "deny-build-config";
    case BUSSEC_POLICY_DENY_PROTECTED_READOUT:
        return "deny-protected-readout";
    case BUSSEC_POLICY_DENY_UNDOCUMENTED_READOUT:
        return "deny-undocumented-readout";
    default:
        return "deny-unknown";
    }
}
