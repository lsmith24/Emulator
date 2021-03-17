/* ESE 350, Spring 2018
 * ------------------
 * Starter code for Lab 4
 *
 * The main purpose is to provide all the FreeGLUT
 * code required to handle display and input.
 * 
 * You are encouraged and, in some cases, required to edit
 * the given functions to suit your implementation.
 *
 * You are by no means bound to use all/any of the given code
 * or required to stick to this style.
 * Feel free to experiment and give a better solution.
 *
 * This uses the FreeGLUT libraries installed in your VM.
 * If you choose to use a different set of third party libraries,
 * please consult a TA.
 */

#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include "gamul.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 320


//architecture we are emulating, refer gamul.h
gamul8 gamer;

volatile uint8_t keyP[16] = {0};
uint8_t memory[0xFFF] = {0};
uint16_t PC = 0x200;
uint16_t stack[16];
uint8_t SP = 0;
uint16_t I;
uint8_t delay = 0x000;
uint8_t sound = 0x000;
uint8_t registers[16];
int flag = 0;
uint8_t pressedKey = 0;

int load_file(char *file_name, unsigned char *buffer);

// method to draw a single pixel on the screen
void draw_square(float x_coord, float y_coord);

// update and render logic, called by glutDisplayFunc
void render();

// idling function for animation, called by glutIdleFunc
void idle();

// initializes GL 2D mode and other settings
void initGL();

// function to handle user keyboard input (when pressed) 
void your_key_press_handler(unsigned char key, int x, int y);

// function to handle key release
void your_key_release_handler(unsigned char key, int x, int y);

void toInstruct();


/*	FUNCTION: main
 *  --------------
 *	Main emulation loop. Loads ROM, executes it, and draws to screen.
 *  You may also want to call here the initialization function you have written
 *  that initializes memory and registers.
 *	PARAMETERS:
 *				argv: number of command line arguments
 *				argc[]: array of command line arguments
 *	RETURNS:	usually 0
 */
int main(int argc, char *argv[])
{
	// seed random variable for use in emulation
	srand(time(NULL));

	// initialize GLUT
	glutInit(&argc, argv);

	// initialize display and window
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutCreateWindow("ESE 350 - Gamul8r");

	// initialize orthographic 2D view, among other things
	initGL();
	 
	// handle key presses and releases
	glutKeyboardFunc(your_key_press_handler);
	glutKeyboardUpFunc(your_key_release_handler);

	// GLUT draw function

	glutDisplayFunc(render);

	load_file(argv[1], &memory[0x200]); //load memory with values from game files
	loadFont(memory); //load fontset to memory


	// GLUT idle function, causes screen to redraw
	glutIdleFunc(idle);

	// main loop, all events processed here
	glutMainLoop();

	return 0;
}

//FUNCTION FOR TRANSLATING OPCODES
void toInstruct() {
	uint16_t num = ((memory[PC] << 8) | memory[PC+1]);
	uint16_t subcode;

	if (delay > 0) {
		delay-=1;
	}

	if (sound > 0) {
		sound-=1;
		system("paplay beep.aiff &> /dev/null &");
	}

	//display: clears screen
	if (num == 0x00E0) {
		for (int i = 0; i < SCREEN_HEIGHT; i++) {
			for (int j = 0; j < SCREEN_WIDTH; j++) {
				gamer.display[j][i] = 0; //set every pixel on screen to be 0
			}
		}
		PC+=2;
	}
	else if (num == 0x00EE) { //return from subroutine
		PC = stack[SP]; 
		SP--;
		PC+=2;
	}
	else {
		//need to further break down opcode
		subcode = num >> 12; 

		if (subcode == 0x01) {
		//1NNN jump to adr NNN
			uint16_t nnn = num & 0xFFF;
			PC = nnn;
		}
		else if (subcode == 0x02) {
		//2NNN call subroutine at NNN
			uint16_t nnn = num & 0xFFF;
			SP++;
			stack[SP] = PC;
			PC = nnn;
		}
		else if (subcode == 0x03) {
		//3XNN skip next inst if vx == nn
			uint16_t x = (num >> 8) & 0x0F;
			uint8_t nn = num & 0xFF;
			if (registers[x] == nn)	{
				PC+=4;
			} else {
				PC+=2;
			}
		}
		else if (subcode == 0x04) {
		//4XNN skip next inst if vx != nn
			uint16_t x = (num >> 8) & 0x0F;;
			uint8_t nn = num & 0xFF;
			if (registers[x] != nn)	{
				PC+=4;
			} else {
				PC+=2;
			}	
		} 
		else if (subcode == 0x05) {
		//5XY0 skip next inst if vx == vy
			uint16_t x = (num & 0xF00) >> 8;
			uint16_t y = (num & 0xF0) >> 4;
			if (registers[x] == registers[y]) {
				PC+=4;
			} else {
				PC+=2;
			}	
		}
		else if (subcode == 0x06) {
		//6XNN set vx to nn
			uint16_t x = (num >> 8) & 0x0F;
			uint8_t nn = num & 0xFF;
			registers[x] = nn;
			PC+=2;	
		}
		else if (subcode == 0x07) {
		//7XNN add nn to vx
			uint16_t x = (num >> 8) & 0x0F;
			uint8_t nn = num & 0xFF;
			registers[x] += nn;
			PC+=2;
		}
		else if (subcode == 0x09) {
		//9XY0 skip next inst is vx != vy
			uint16_t x = (num & 0xF00) >> 8;
			uint16_t y = (num & 0xF0) >> 4;
			if (registers[x] != registers[y]) {
				PC+=4;
			} else {
				PC+=2;
			}	
		}
		else if (subcode == 0x0A) {
		//ANNN set I to address NNN
			uint16_t nnn = num & 0xFFF;
			I = nnn;
			PC+=2;
		}
		else if (subcode == 0x0B) {
		//BNNN jump to adr NNN + V0
			uint16_t nnn = num & 0xFFF;
			PC = nnn + registers[0];
		} 
		else if (subcode == 0x0C) {
		//CXNN set vx to random num bitwise AND w/ NN
			uint16_t random = rand() % 255;
			uint16_t x = (num >> 8) & 0x0F;
			uint8_t nn = num & 0xFF;
			registers[x] = random & nn;
			PC+=2;
		}
		else if (subcode == 0x0D) {
		//DXYN draws sprite at (vx,vy) width:8 height:N
			uint16_t x = (num >> 8) & 0x0F;
			uint16_t y = (num >> 4) & 0x0F;
			uint8_t n = num & 0x0F;

			uint8_t regX = registers[x];
			uint8_t regY = registers[y];

			registers[0xF] = 0;
			for (int i = 0; i < n; i++) { //incrememnts per byte

				uint8_t byte = memory[I + i];

				for (int j = 0; j < 8; j++) { //increments per bit
					uint8_t bit = ((byte >> (7-j)) & 0x01); //need to obtain each bit of the byte
					uint8_t w = (regX + j) % SCREEN_WIDTH; //need to mod by the width/height bc wrap around
					uint8_t h = (regY + i) % SCREEN_HEIGHT;
					uint8_t prev = gamer.display[w][h];
					gamer.display[w][h] ^= bit;
					if (prev == 1 && bit == 1) { //collision flag
						registers[0xF] = 1;
					} 
						
				}
			}

			PC+=2;
		}
		
			
		else if (subcode == 0x08) {
		//need to break down further
			uint8_t n = (num & 0x0F); //will equal only bottom 4 bits of original
			uint16_t x = (num >> 8) & 0x0F;
			uint16_t y = (num >> 4) & 0x0F;
			if (n == 0x00) {
			//8XY0 set vx to value of vy
				registers[x] = registers[y];
				PC+=2;
			}
			else if (n == 0x01) {
			//8XY1 set vx to (vx OR vy)
				registers[x] = (registers[x] | registers[y]);	
				PC+=2;
			}
			else if (n == 0x02) {
			//8XY2 set vx to (vx AND vy)
				registers[x] = (registers[x] & registers[y]);	
				PC+=2;	
			}
			else if (n == 0x03) {
			//8XY3 set vx to (vx XOR vy)
				registers[x] = (registers[x] ^ registers[y]);	
				PC+=2;
			}
			else if (n == 0x04) {
			//8XY4 add vy to vx, vf set to 1 for carry
				signed int a = registers[x] + registers[y];
				if (a < 0) { //a signed number will have 1 in the msb (be negative) if there is carry
					registers[0xF] = 1;
				} else {
					registers[0xF] = 0;
				}
				registers[x] = registers[x] + registers[y];
				PC+=2;
			}
			else if (n == 0x05) {
			//8XY5 vx = vx - vy, vf = 0 for borrow
				signed int a = registers[x] - registers[y];
				if (a < 0) {
					registers[0xF] = 0;
				} else {
					registers[0xF] = 1;
				}
				registers[x] = registers[x] - registers[y];
				PC+=2;
			}
			else if (n == 0x06) {
			//8XY6 vx >> 1, vf = lsb before shift
				registers[0xF] = (registers[x] & 0x01);
				registers[x] = (registers[x] >> 1);
				PC+=2;
			}
			else if (n == 0x07) {
			//8XY7 vx = vy - vx, vf = 0 for borrow
				signed int a = registers[y] - registers[x];
				if (a < 0) {
					registers[0xF] = 0;
				} else {
					registers[0xF] = 1;
				}
				registers[x] = registers[y] - registers[x];
				PC+=2;
			}
			else if (n == 0x0E) {
			//8XYE vx << 1. vf = msb before shift
				registers[0xF] = (registers[x] & 0x80);
				registers[x] = (registers[x] << 1);
				PC+=2;
			}
		}
		else if (subcode == 0x0E) {
			uint8_t n = (num & 0xFF); //bottom eight bits
			if (n == 0x9E) {
			//EX9E skip next inst if key stored in vx is pressed
				uint16_t x = (num & 0xF00) >> 8;
				uint8_t k = registers[x];
				if (keyP[k] == 1) {
					PC+=4;;
				} else {
				PC+=2;
				}
			}
			else if (n == 0xA1) {
			//EXA1 skip next inst if key stored in vx is Not pressed
				uint16_t x = (num & 0xF00) >> 8;
				uint8_t k = registers[x];
				if (keyP[k] == 0) {
					PC+=4;
				} else {
				PC+=2;
				}
			}
		}
		else if (subcode == 0x0F) {
			uint16_t n = (num & 0xFF); //bottom eight bits
			uint16_t x = (num >> 8) & 0x0F;
			if (n == 0x07) {
			//FX07 set vx to value of delay timer
				registers[x] = delay;
				PC+=2;
			}
			else if (n == 0x0A) {
			//FX0A key press awaited and stored in vx
				if(pressedKey > 0) {
					registers[x] = pressedKey - 1;
					PC+=2;
				}
			}
			else if (n == 0x15) {
			//FX15 set delay timer to vx
				delay = registers[x];
				PC+=2;
			}
			else if (n == 0x18) {
			//FX18 set sound timer to vx
				sound = registers[x];
				PC+=2;
			}
			else if (n == 0x1E) {
			//FX1E add VX to I
				I = registers[x] + I;
				PC+=2;
			}
			else if (n == 0x29) {
			//FX29 set I to location of sprite char in VX
				I = 5 * registers[x]; //sprites are each 5 numbers long
				PC+=2;
			}
			else if (n == 0x33) {
			//FX33 store binary dec of vx (more details on sheet)
				uint16_t val = registers[x];
				for (int i = 0; i < 3; i++) {
					memory[I + i] = val % 10; //val % 10 will give each place value 
					val = val / 10;
				}
				PC+=2;
			}
			else if (n == 0x55) {
			//FX55 store v0 to vx (inclusive) in mem starting at addr I
				for (int i = 0; i <= x; i++) {
					memory[I + i] = registers[i];
				}
				PC+=2;
			}
			else if (n == 0x65) {
			//FX65 fill v0 to vx (inclusive) from mem starting at addr I
				for (int i = 0; i <= x; i++) {
					registers[i] = memory[I + i];
				}
				PC+=2;
			}
		} else {
			printf("Illegal Opcode\n");
			PC = 0xFFF;
		}
	}
}

//END OF OPCODE TRANSLATION FUNCTION


/*	FUNCTION: draw_square
 *  ----------------------
 *	Draws a square. Represents a single pixel
 *  (Up-scaled to a 640 x 320 display for better visibility)
 *	PARAMETERS: 
 *	x_coord: x coordinate of the square
 *	y_coord: y coordinate of the square
 *	RETURNS: none
 */
void draw_square(float x_coord, float y_coord)
{
	// draws a white 10 x 10 square with the coordinates passed

	glBegin(GL_QUADS);  //GL_QUADS treats the following group of 4 vertices 
						//as an independent quadrilateral

		glColor3f(1.0f, 1.0f, 1.0f); 	//color in RGB
										//values between 0 & 1
										//E.g. Pure Red = (1.0f, 0.0f, 0.0f)
		glVertex2f(x_coord, y_coord);			//vertex 1
		glVertex2f(x_coord + 10, y_coord);		//vertex 2
		glVertex2f(x_coord + 10, y_coord + 10);	//vertex 3
		glVertex2f(x_coord, y_coord + 10);		//vertex 4

		//glVertex3f lets you pass X, Y and Z coordinates to draw a 3D quad
		//only if you're interested

	glEnd();
}

/*	FUNCTION: render
 *  ----------------
 *	GLUT render function to draw the display. Also emulates one
 *	cycle of emulation.
 *	PARAMETERS: none
 *	RETURNS: none
 */
void render()
{
	// clears screen
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	
	glLoadIdentity();

	//display_func(&gamer);
	
	if (PC > 0x1FF && PC < 0xFFF) {
		toInstruct();
		//printf("PC: %04X\n", PC);
	}
	


	// draw a pixel for each display bit
	int i, j;
	for (i = 0; i < SCREEN_WIDTH; i++) {
		for (j = 0; j < SCREEN_HEIGHT; j++) {
			if (gamer.display[i][j] == 1) {
				draw_square((float)(i * 10), (float)(j * 10));
			}
		}
	}

	// swap buffers, allows for smooth animation
	glutSwapBuffers();
}

/*	FUNCTION: idle
 *  -------------- 
 *	GLUT idle function. Instructs GLUT window to redraw itself
 *	PARAMETERS: none
 *	RETURNS: none
 */
void idle()
{
	// gives the call to redraw the screen
	glutPostRedisplay();

}


/*	FUNCTION: initGL
 *  ----------------
 *	Initializes GLUT settings.
 *	PARAMETERS: none
 *	RETURN VALUE: none
 */
void initGL()
{
	// sets up GLUT window for 2D drawing
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// clears screen color (RGBA)
	glClearColor(0.f, 0.f, 0.f, 1.f);
}

/*	FUNCTION: your_key_press_handler
 *  --------------------------------
 *	Handles all keypresses and passes them to the emulator.
 *  This is also where you will be mapping the original GAMUL8
 *  keyboard layout to the layout of your choice
 *  Something like this:
 *  |1|2|3|C|		=>		|1|2|3|4|
 *	|4|5|6|D|		=>		|Q|W|E|R|
 *	|7|8|9|E|		=>		|A|S|D|F|
 *	|A|0|B|F|		=>		|Z|X|C|V|
 *	PARAMETERS: 
 *	key: the key that is pressed.
 *	x: syntax required by GLUT
 *	y: syntax required by GLUT
 *  (x & y are callback parameters that indicate the mouse location
 *  on the window. We are not using the mouse, so they won't be used, 
 *  but still pass them as glutKeyboardFunc needs it.) 
 *	RETURNS: none
 *  NOTE: If you intend to handle this in a different manner, you need not
 *  write this function.
 */
void your_key_press_handler(unsigned char key, int x, int y)
{
	switch(key) {
		case '1':
			keyP[0] = 1;
			pressedKey = 1;
			break;
		case '2':
			keyP[1] = 1;
			pressedKey = 2;
			break;
		case '3':
			keyP[2] = 1;
			pressedKey = 3;
			break;
		case '4':
			keyP[3] = 1;
			pressedKey = 4; 
			break;
		case 'q':
			keyP[4] = 1;
			pressedKey = 5;
			break;
		case 'w':
			keyP[5] = 1;
			pressedKey = 6;
			break;
		case 'e':
			keyP[6] = 1;
			pressedKey = 7;
			break;
		case 'r':
			keyP[7] = 1;
			pressedKey = 8;
			break;
		case 'a':
			keyP[8] = 1;
			pressedKey = 9;
			break;
		case 's':
			keyP[9] = 1;
			pressedKey = 10;
			break;
		case 'd':
			keyP[10] = 1;
			pressedKey = 11;
			break;
		case 'f':
			keyP[11] = 1;
			pressedKey = 12;
			break;
		case 'z':
			keyP[12] = 1;
			pressedKey = 13;
			break;
		case 'x':
			keyP[13] = 1;
			pressedKey = 14;
			break;
		case 'c':
			keyP[14] = 1;
			pressedKey = 15;
			break;
		case 'v':
			keyP[15] = 1;
			pressedKey = 16;
			break;

	}
}

/*	FUNCTION: your_key_release_handler
 *  --------------------------------
 *	Tells emulator if any key is released. You can maybe give
 *  a default value if the key is released.
 *	PARAMETERS: 
 *	key: the key that is pressed.
 *	x: syntax required by GLUT
 *	y: syntax required by GLUT
 *  (x & y are callback parameters that indicate the mouse location
 *  on the window. We are not using the mouse, so they won't be used, 
 *  but still pass them as glutKeyboardFunc needs it.) 
 *	RETURNS: none
 *  NOTE: If you intend to handle this in a different manner, you need not
 *  write this function.
 */
void your_key_release_handler(unsigned char key, int x, int y)
{
switch(key) {
		case '1':
			keyP[0] = 0;
			break;
		case '2':
			keyP[1] = 0;
			break;
		case '3':
			keyP[2] = 0;
			break;
		case '4':
			keyP[3] = 0;
			break;
		case 'q':
			keyP[4] = 0;
			break;
		case 'w':
			keyP[5] = 0;
			break;
		case 'e':
			keyP[6] = 0;
			break;
		case 'r':
			keyP[7] = 0;
			break;
		case 'a':
			keyP[8] = 0;
			break;
		case 's':
			keyP[9] = 0;
			break;
		case 'd':
			keyP[10] = 0;
			break;
		case 'f':
			keyP[11] = 0;
			break;
		case 'z':
			keyP[12] = 0;
			break;
		case 'x':
			keyP[13] = 0;
			break;
		case 'c':
			keyP[14] = 0;
			break;
		case 'v':
			keyP[15] = 0;
			break;

	}
}
