#include <WProgram.h>
#include "PinChangeInt.h"

int main(void)
{
	init();

	setup();
    
	for (;;)
		loop();
        
	return 0;
}

