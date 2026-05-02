#include "bussec_cli.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bussec_policy.h"
#include "clock_config.h"
#include "host_console.h"
#include "pic32cm5164jh01048.h"

/* Tiny bring-up CLI: no libc formatting, no heap, and predictable UART output. */
#define BUSSEC_VERSION          "BusSec-PIC32CM v0.1-alpha"
#define BUSSEC_CLI_LINE_LENGTH  (512u)
#define BUSSEC_STATUS_TICK_HZ   (2u)

static char cli_line[BUSSEC_CLI_LINE_LENGTH];
static uint32_t cli_line_length;
/* Updated by the TC0 ISR callback and consumed by the foreground CLI task. */
static volatile uint32_t status_ticks;
static uint32_t uptime_seconds;
static bussec_mode_t current_mode;
static bool research_sacrificial_confirmed;

static void write_uint32(uint32_t value)
{
    /* Convert in reverse into a fixed stack buffer to avoid pulling in printf. */
    char digits[10];
    uint32_t count = 0u;

    do {
        digits[count] = (char)('0' + (value % 10u));
        value /= 10u;
        count++;
    } while (value != 0u);

    while (count != 0u) {
        count--;
        host_console_putc(digits[count]);
    }
}

static bool text_equals(const char *left, const char *right)
{
    while ((*left != '\0') && (*right != '\0')) {
        if (*left != *right) {
            return false;
        }
        left++;
        right++;
    }

    return (*left == '\0') && (*right == '\0');
}

static bussec_policy_context_t policy_context(void)
{
    return (bussec_policy_context_t){
        .owned_target_confirmed = false,
        .sacrificial_target_confirmed = research_sacrificial_confirmed,
        .target_profile_known = false
    };
}

static void write_bool(bool value)
{
    host_console_write(value ? "yes" : "no");
}

static void write_policy_result(bussec_policy_action_t action,
                                bussec_mode_t mode,
                                bussec_policy_result_t result)
{
    host_console_write("POLICY action=");
    host_console_write(bussec_policy_action_name(action));
    host_console_write(" mode=");
    host_console_write(bussec_policy_mode_name(mode));
    host_console_write(" result=");
    host_console_write(bussec_policy_result_name(result));
    host_console_write("\n");
}

static void write_policy_error(bussec_policy_result_t result)
{
    host_console_write("!ERR code=POLICY_DENIED result=");
    host_console_write(bussec_policy_result_name(result));
    host_console_write("\n");
}

static void write_status(void)
{
    host_console_write("STATUS uptime_s=");
    write_uint32(uptime_seconds);
    host_console_write(" cpu_hz=");
    write_uint32(APP_CPU_CLOCK_HZ);
    host_console_write(" host_baud=");
    write_uint32(HOST_CONSOLE_BAUD);
    host_console_write(" mode=");
    host_console_write(bussec_policy_mode_name(current_mode));
    host_console_write(" sacrificial=");
    write_bool(research_sacrificial_confirmed);
    host_console_write(" rx_overflows=");
    write_uint32(host_console_rx_overflow_count());
    host_console_write(" rx_errors=");
    write_uint32(host_console_rx_error_count());
    host_console_write("\n");
}

static void write_help(void)
{
    host_console_write("Commands: version, help, status\n");
    host_console_write("Research: research status, research begin confirm=SACRIFICIAL_TARGET\n");
    host_console_write("Research: research scan-safe, research end\n");
    host_console_write("Policy: policy selftest\n");
}

static void write_research_status(void)
{
    host_console_write("RESEARCH mode=");
    host_console_write(bussec_policy_mode_name(current_mode));
    host_console_write(" sacrificial=");
    write_bool(research_sacrificial_confirmed);
    host_console_write(" safe_anomaly_scan=");
    host_console_write(bussec_policy_result_name(
        bussec_policy_check(current_mode,
                            BUSSEC_POLICY_ACTION_RESEARCH_SAFE_ANOMALY_SCAN,
                            policy_context())));
    host_console_write(" protected_readout=blocked undocumented_readout=blocked\n");
}

static void begin_research(void)
{
    current_mode = BUSSEC_MODE_RESEARCH_DEV;
    research_sacrificial_confirmed = true;
    host_console_write("RESEARCH_AUDIT action=begin mode=research_dev confirm=SACRIFICIAL_TARGET\nOK\n");
}

static void end_research(void)
{
    current_mode = BUSSEC_MODE_NORMAL;
    research_sacrificial_confirmed = false;
    host_console_write("RESEARCH_AUDIT action=end mode=normal\nOK\n");
}

static void run_research_scan_safe(void)
{
    const bussec_policy_result_t result =
        bussec_policy_check(current_mode,
                            BUSSEC_POLICY_ACTION_RESEARCH_SAFE_ANOMALY_SCAN,
                            policy_context());

    if (!bussec_policy_result_allowed(result)) {
        write_policy_error(result);
        return;
    }

    host_console_write("RESEARCH_SCAN profile=sync-only action=timing-and-state-anomaly ");
    host_console_write("status=not-implemented updi_layer=missing\n");
    host_console_write("RESEARCH_SCAN protected_readout=blocked undocumented_readout=blocked ");
    host_console_write("destructive_actions=none\nOK\n");
}

static void write_policy_selftest(void)
{
    bussec_policy_context_t context = {
        .owned_target_confirmed = true,
        .sacrificial_target_confirmed = true,
        .target_profile_known = true
    };

    write_policy_result(BUSSEC_POLICY_ACTION_DOCUMENTED_LOCKTEST_READ,
                        BUSSEC_MODE_LOCKTEST,
                        bussec_policy_check(BUSSEC_MODE_LOCKTEST,
                                            BUSSEC_POLICY_ACTION_DOCUMENTED_LOCKTEST_READ,
                                            context));
    write_policy_result(BUSSEC_POLICY_ACTION_RESEARCH_SAFE_ANOMALY_SCAN,
                        BUSSEC_MODE_RESEARCH_DEV,
                        bussec_policy_check(BUSSEC_MODE_RESEARCH_DEV,
                                            BUSSEC_POLICY_ACTION_RESEARCH_SAFE_ANOMALY_SCAN,
                                            context));
    write_policy_result(BUSSEC_POLICY_ACTION_UNDOCUMENTED_READOUT_PROBE,
                        BUSSEC_MODE_RESEARCH_DEV,
                        bussec_policy_check(BUSSEC_MODE_RESEARCH_DEV,
                                            BUSSEC_POLICY_ACTION_UNDOCUMENTED_READOUT_PROBE,
                                            context));
    write_policy_result(BUSSEC_POLICY_ACTION_PROTECTED_READOUT,
                        BUSSEC_MODE_RESEARCH_DEV,
                        bussec_policy_check(BUSSEC_MODE_RESEARCH_DEV,
                                            BUSSEC_POLICY_ACTION_PROTECTED_READOUT,
                                            context));
    host_console_write("OK\n");
}

static void dispatch_command(void)
{
    /* Reserve one byte for the terminator by limiting input to LINE_LENGTH - 1. */
    cli_line[cli_line_length] = '\0';

    if (cli_line_length == 0u) {
        return;
    }

    host_console_write("> ");
    host_console_write(cli_line);
    host_console_write("\n");

    if (text_equals(cli_line, "version")) {
        host_console_write(BUSSEC_VERSION "\nOK\n");
    } else if (text_equals(cli_line, "help")) {
        write_help();
        host_console_write("OK\n");
    } else if (text_equals(cli_line, "status")) {
        write_status();
        host_console_write("OK\n");
    } else if (text_equals(cli_line, "research status")) {
        write_research_status();
        host_console_write("OK\n");
    } else if (text_equals(cli_line, "research begin confirm=SACRIFICIAL_TARGET")) {
        begin_research();
    } else if (text_equals(cli_line, "research scan-safe")) {
        run_research_scan_safe();
    } else if (text_equals(cli_line, "research end")) {
        end_research();
    } else if (text_equals(cli_line, "policy selftest")) {
        write_policy_selftest();
    } else {
        host_console_write("!ERR code=BAD_COMMAND\n");
    }
}

static void handle_byte(uint8_t byte)
{
    if ((byte == '\r') || (byte == '\n')) {
        /* Treat either newline convention as command submission. */
        dispatch_command();
        cli_line_length = 0u;
        return;
    }

    if ((byte == '\b') || (byte == 0x7Fu)) {
        /* Accept both ASCII backspace and DEL from common terminal emulators. */
        if (cli_line_length != 0u) {
            cli_line_length--;
        }
        return;
    }

    if ((byte < 0x20u) || (byte > 0x7Eu)) {
        /* Ignore control and non-ASCII bytes until binary protocol support exists. */
        return;
    }

    if (cli_line_length < (BUSSEC_CLI_LINE_LENGTH - 1u)) {
        cli_line[cli_line_length] = (char)byte;
        cli_line_length++;
    } else {
        /* Drop a partial overlong command instead of dispatching a truncated command. */
        cli_line_length = 0u;
        host_console_write("!ERR code=LINE_TOO_LONG\n");
    }
}

void bussec_cli_init(void)
{
    cli_line_length = 0u;
    status_ticks = 0u;
    uptime_seconds = 0u;
    current_mode = BUSSEC_MODE_NORMAL;
    research_sacrificial_confirmed = false;

    host_console_write("# " BUSSEC_VERSION "\n");
    host_console_write("# Type help\n");
}

void bussec_cli_status_tick(void)
{
    /* Called from the periodic timer path; keep this ISR-facing hook minimal. */
    status_ticks++;
}

void bussec_cli_task(void)
{
    uint8_t byte;
    uint32_t pending_status_ticks;
    const uint32_t primask = __get_PRIMASK();

    while (host_console_read_byte(&byte)) {
        handle_byte(byte);
    }

    /* Snapshot and consume full seconds with interrupts masked around the shared counter. */
    __disable_irq();
    pending_status_ticks = status_ticks;
    if (pending_status_ticks >= BUSSEC_STATUS_TICK_HZ) {
        status_ticks = pending_status_ticks - BUSSEC_STATUS_TICK_HZ;
    }
    __set_PRIMASK(primask);

    if (pending_status_ticks >= BUSSEC_STATUS_TICK_HZ) {
        uptime_seconds++;
        write_status();
    }
}

bool bussec_cli_has_pending_work(void)
{
    return host_console_has_pending_rx() || (status_ticks >= BUSSEC_STATUS_TICK_HZ);
}
