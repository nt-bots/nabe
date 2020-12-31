#include "interrupt_handler.h"

#include <signal.h>

#include "print_helpers.h"

// Interrupt signal handler
static volatile bool _running = true;

bool was_interrupted()
{
	return _running == false;
}

void interrupt(int dummy)
{
	print(Warning, "RECEIVED SYSTEM INTERRUPT (SIGINT)");
	print(Info, "Preparing to close the program, please wait...");
	_running = false;
}

void enable_interrupt_handler()
{
	_running = true;
	signal(SIGINT, interrupt);
}