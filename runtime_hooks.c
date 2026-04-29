void _init(void)
{
    /* newlib calls this hook during C runtime startup; no project init is needed here. */
}

void _fini(void)
{
    /* Bare-metal firmware never returns to a host process, so finalization is intentionally empty. */
}
