#pragma once
/* Call only from USB core 1. Disabled builds link to no-op implementations. */
void bb_panel_init(void);
void bb_panel_poll(void);
