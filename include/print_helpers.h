#ifndef NABENABE_PRINT_HELPERS_H
#define NABENABE_PRINT_HELPERS_H

enum LogLevel {
	Info, Error, Warning, RawText, NewLine
};

// Displays an ASCII art logo, just for fun
void show_ascii_logo();

void print(LogLevel loglvl, const char* msg = nullptr, ...);

#endif // NABENABE_PRINT_HELPERS_H