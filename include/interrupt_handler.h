#ifndef NABENABE_INTERRUPT_HANDLER_H
#define NABENABE_INTERRUPT_HANDLER_H

bool was_interrupted();

// Catch system interrupt, so we can gracefully exit any active database connections or file handles.
void enable_interrupt_handler();

#endif // NABENABE_INTERRUPT_HANDLER_H