Lindsay Smith
ESE 350 Sp2020

Files: emul8.c gamul.c gamul.h

Compile using:
gcc emul8.c gamul.c -o emulator -lGL -lGLU -lglut -std=c99 -Wall

To run: ./emulator ./games/NAMEOFGAME

Errors handled: 
-If the file does not exist/cannot be opened an error message is printed to the terminal
and the PC is set to 0xFFF so that it does not try to run the instruction translation

-If the opcode does not match any of the given ones an error message saying illegal opcode
will be printed to the terminal

-If the PC is not between 0x200 and 0xFFF it will not try to translate the instruction

Functions:
-toInstruct(): reads at memory[PC] and matches it to the correct opcode, which is then executed. PC is incrememnted accordingly.

-loadFont(uint8_t memory[]): takes the fontset and loads it to memory starting at 0x000


Other implementation information:

-For the key press handler the corresponding index in the keyP array is set to 1 if that key is pressed. It also stores a variable pressedKey that is used in FX0A to store the key in vx.

-The PC, SP, registers, memory etc are all kept as global variables and are declared before main(). This allows toInstruct() to not have any parameters and the values stored in the global variables will always be accessible. 

Overall walkthrough:
-First the file given in the command line is opened and read as a binary file, where every 8 bits are stored to a memory array starting at address 0x200

-The fontset is also loaded to memory but starting at address 0x000

-PC is initialized to 0x200 and inside of render() there is a check for if the PC is in bounds. If it is then the toInstruct() function is called

-toInstruct() gets the current opcode from memory[PC] and memory[PC + 1] and has many if else statements that look at different parts of the opcode to find which one it matches to

-Once a match is found that instruction is executed and the PC is either incrememnted or set to what the instruction requires

-The individual opcode executions are commented with their individual logic
 





