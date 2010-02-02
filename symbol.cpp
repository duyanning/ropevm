#include "std.h"
#include "jam.h"
#include "symbol.h"

#define SYMBOL_VALUE(name, value) value
const char *symbol_values[] = {
    SYMBOLS_DO(SYMBOL_VALUE)
};

void initialiseSymbol() {
    int i;

    for(i = 0; i < MAX_SYMBOL_ENUM; i++)
        if(symbol_values[i] != newUtf8(symbol_values[i])) {
            jam_fprintf(stderr, "Error when initialising VM symbols.  Aborting.\n");
            exit(1);
        }
}
