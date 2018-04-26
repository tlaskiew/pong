/* the width and height of the screen */
#define WIDTH 240
#define HEIGHT 160

/* these identifiers define different bit positions of the display control */
#define MODE4 0x0004
#define BG2 0x0400

/* this bit indicates whether to display the front or the back buffer
 * this allows us to refer to bit 4 of the display_control register */
#define SHOW_BACK 0x10;

/* the screen is simply a pointer into memory at a specific address this
 *  * pointer points to 16-bit colors of which there are 240x160 */
volatile unsigned short* screen = (volatile unsigned short*) 0x6000000;

/* the display control pointer points to the gba graphics register */
volatile unsigned long* display_control = (volatile unsigned long*) 0x4000000;

/* the address of the color palette used in graphics mode 4 */
volatile unsigned short* palette = (volatile unsigned short*) 0x5000000;

/* pointers to the front and back buffers - the front buffer is the start
 * of the screen array and the back buffer is a pointer to the second half */
volatile unsigned short* front_buffer = (volatile unsigned short*) 0x6000000;
volatile unsigned short* back_buffer = (volatile unsigned short*)  0x600A000;

/* the button register holds the bits which indicate whether each button has
 * been pressed - this has got to be volatile as well
 */
volatile unsigned short* buttons = (volatile unsigned short*) 0x04000130;

/* the bit positions indicate each button - the first bit is for A, second for
 * B, and so on, each constant below can be ANDED into the register to get the
 * status of any one button */
#define BUTTON_A (1 << 0)
#define BUTTON_B (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START (1 << 3)
#define BUTTON_RIGHT (1 << 4)
#define BUTTON_LEFT (1 << 5)
#define BUTTON_UP (1 << 6)
#define BUTTON_DOWN (1 << 7)
#define BUTTON_R (1 << 8)
#define BUTTON_L (1 << 9)

/* the scanline counter is a memory cell which is updated to indicate how
 * much of the screen has been drawn */
volatile unsigned short* scanline_counter = (volatile unsigned short*) 0x4000006;

/* wait for the screen to be fully drawn so we can do something during vblank */
void wait_vblank() {
		/* wait until all 160 lines have been updated */
		while (*scanline_counter < 160) { }
}

/* this function checks whether a particular button has been pressed */
unsigned char button_pressed(unsigned short button) {
		/* and the button register with the button constant we want */
		unsigned short pressed = *buttons & button;

		/* if this value is zero, then it's not pressed */
		if (pressed == 0) {
				return 1;
		} else {
				return 0;
		}
}

/* keep track of the next palette index */
int next_palette_index = 0;

/*
 * function which adds a color to the palette and returns the
 * index to it
 */
unsigned char add_color(unsigned char r, unsigned char g, unsigned char b) {
		unsigned short color = b << 10;
		color += g << 5;
		color += r;

		/* add the color to the palette */
		palette[next_palette_index] = color;

		/* increment the index */
		next_palette_index++;

		/* return index of color just added */
		return next_palette_index - 1;
}

/* a colored square */
struct square {
		unsigned short x, y, size;
		unsigned char color;
};

/* put a pixel on the screen in mode 4 */
void put_pixel(volatile unsigned short* buffer, int row, int col, unsigned char color) {
		/* find the offset which is the regular offset divided by two */
		unsigned short offset = (row * WIDTH + col) >> 1;

		/* read the existing pixel which is there */
		unsigned short pixel = buffer[offset];

		/* if it's an odd column */
		if (col & 1) {
				/* put it in the left half of the short */
				buffer[offset] = (color << 8) | (pixel & 0x00ff);
		} else {
				/* it's even, put it in the left half */
				buffer[offset] = (pixel & 0xff00) | color;
		}
}

/* draw a square onto the screen */
void draw_square(volatile unsigned short* buffer, struct square* s) {
		short row, col;
		/* for each row of the square */
		for (row = s->y; row < (s->y + s->size*5); row++) {
				/* loop through each column of the square */
				for (col = s->x; col < (s->x + s->size); col++) {
						/* set the screen location to this color */
						put_pixel(buffer, row, col, s->color);
				}
		}
}

void draw_ball(volatile unsigned short* buffer, struct square* s) {
		short row, col;
		/* for each row of the square */
		for (row = s->y; row < (s->y + s->size); row++) {
				/* loop through each column of the square */
				for (col = s->x; col < (s->x + s->size); col++) {
						/* set the screen location to this color */
						put_pixel(buffer, row, col, s->color);
				}
		}
}

void update_screen_ball(volatile unsigned short* buffer, unsigned short color, struct square* s) {
		short row, col;
		for (row = s->y - 3; row < (s->y + s->size + 3); row++) {
				for (col = s->x - 3; col < (s->x + s->size + 3); col++) {
						put_pixel(buffer, row, col, color);
				}
		}
}



/* clear the screen right around the square */
void update_screen(volatile unsigned short* buffer, unsigned short color, struct square* s) {
		short row, col;
		for (row = s->y - 3; row < (s->y + s->size*5 + 3); row++) {
				for (col = s->x - 3; col < (s->x + s->size + 3); col++) {
						put_pixel(buffer, row, col, color);
				}
		}
}

/* this function takes a video buffer and returns to you the other one */
volatile unsigned short* flip_buffers(volatile unsigned short* buffer) {
		/* if the back buffer is up, return that */
		if(buffer == front_buffer) {
				/* clear back buffer bit and return back buffer pointer */
				*display_control &= ~SHOW_BACK;
				return back_buffer;
		} else {
				/* set back buffer bit and return front buffer */
				*display_control |= SHOW_BACK;
				return front_buffer;
		}
}

/* handle the buttons which are pressed down */
void handle_buttons(struct square* s) {
		/* move the square with the arrow keys */
		if (button_pressed(BUTTON_DOWN)) {
				if(s->y > 150){
				}else{
						s->y += 1;
				}
		}
		if (button_pressed(BUTTON_UP)) {
				if(s->y == 0){
				}else{
						s->y -= 1;
				}
		}
}

/* clear the screen to black */
void clear_screen(volatile unsigned short* buffer, unsigned short color) {
		unsigned short row, col;
		/* set each pixel black */
		for (row = 0; row < HEIGHT; row++) {
				for (col = 0; col < WIDTH; col++) {
						put_pixel(buffer, row, col, color);
				}
		}
}

/* move up = 1, move down = 0 */
int AImovement(struct square* s, int move, int ball_direction, struct square* ball){
		if(ball_direction == 5){
				if(s->y+4 > ball->y){
						move = 1;
				}else if(s->y < ball->y){
						move = 0;
				}
		}else if(ball_direction == 4){
				if(s->y+4 > ball->y){
						move = 1;
				}else if(s->y < ball->y){
						move = 0;
				}
		}else if(ball_direction == 6){
				if(s->y+4 > ball->y){
						move = 1;
				}else if(s->y < ball->y){
						move = 0;
				}
		}

		if(s->y > 150){
				move = 1;	
		}
		else if(s->y == 0){
				move = 0;
		}

		if(move == 1){
				s->y -= 1;
		}else if(move == 0){
				s->y += 1;
		}

		return move;
}

/* Display Pong Logo*/
void DrawPong(volatile unsigned short* buffer, unsigned short color){
		//'P'
		put_pixel(buffer, 1, 110, color);
		put_pixel(buffer, 1, 111, color);
		put_pixel(buffer, 1, 112, color);
		put_pixel(buffer, 1, 113, color);
		put_pixel(buffer, 2, 110, color);
		put_pixel(buffer, 3, 110, color);
		put_pixel(buffer, 4, 110, color);
		put_pixel(buffer, 5, 110, color);
		put_pixel(buffer, 2, 113, color);
		put_pixel(buffer, 3, 111, color);
		put_pixel(buffer, 3, 112, color);
		put_pixel(buffer, 3, 113, color);

		//'O'
		put_pixel(buffer, 1, 115, color);
		put_pixel(buffer, 1, 116, color);
		put_pixel(buffer, 1, 117, color);
		put_pixel(buffer, 1, 118, color);
		put_pixel(buffer, 5, 115, color);
		put_pixel(buffer, 5, 116, color);
		put_pixel(buffer, 5, 117, color);
		put_pixel(buffer, 5, 118, color);
		put_pixel(buffer, 2, 115, color);
		put_pixel(buffer, 2, 118, color);
		put_pixel(buffer, 3, 115, color);
		put_pixel(buffer, 3, 118, color);
		put_pixel(buffer, 4, 115, color);
		put_pixel(buffer, 4, 118, color);

		//'N'
		put_pixel(buffer, 1, 120, color);
		put_pixel(buffer, 1, 121, color);
		put_pixel(buffer, 1, 122, color);
		put_pixel(buffer, 1, 123, color);	
		put_pixel(buffer, 2, 120, color);
		put_pixel(buffer, 2, 123, color);
		put_pixel(buffer, 3, 120, color);
		put_pixel(buffer, 3, 123, color);
		put_pixel(buffer, 4, 120, color);
		put_pixel(buffer, 4, 123, color);
		put_pixel(buffer, 5, 120, color);
		put_pixel(buffer, 5, 123, color);

		//'G'
		put_pixel(buffer, 1, 125, color);
		put_pixel(buffer, 1, 126, color);
		put_pixel(buffer, 1, 127, color);
		put_pixel(buffer, 1, 128, color);
		put_pixel(buffer, 2, 125, color);
		put_pixel(buffer, 3, 125, color);
		put_pixel(buffer, 3, 127, color);
		put_pixel(buffer, 3, 128, color);
		put_pixel(buffer, 4, 125, color);
		put_pixel(buffer, 4, 128, color);
		put_pixel(buffer, 5, 125, color);
		put_pixel(buffer, 5, 126, color);
		put_pixel(buffer, 5, 127, color);
		put_pixel(buffer, 5, 128, color);	

}	

//Direction ball is moving
//1 E, 2 NE, 3 N, 4 NW, 5 W, 6 SW, 7 S, 8 SE
int ballMovement(struct square* ball, struct square* userPaddle, struct square* AIPaddle, int direction){
		//Starts Movement
		if(direction == 0){
				direction = 1;
				ball->x += 1;
		}else if(direction == 1){
				if((ball->x == userPaddle->x-1) && (ball->y > userPaddle->y-1 && ball->y < userPaddle->y+9)){
						//Ball hit user Paddle, going left
						if(ball->y == userPaddle->y+5 || ball->y == userPaddle->y+4){
								//West
								direction = 5;
						}else if(ball->y > userPaddle->y-1 && ball->y < userPaddle->y+4){
								//NorthWest
								direction = 4;
						}else if(ball->y < userPaddle->y+9 && ball->y > userPaddle->y+5){
								//SouthWest
								direction = 6;
						}
				}else if(ball->x > 235){
						//Reset
						direction = 102;
						//ball->x=120;
						//ball->y=80;
				}else{
						ball->x += 1;
				}
		}else if(direction == 5){
				if((ball->x == AIPaddle->x+1) && (ball->y > AIPaddle->y-2 && ball->y < AIPaddle->y+10)){
						//Ball hit AI Paddle, going right
						if(ball->y == AIPaddle->y+5 || ball->y == AIPaddle->y+4){
								//East
								direction = 1;
						}else if(ball->y > AIPaddle->y-2 && ball->y < AIPaddle->y+4){
								//NorthEast
								direction = 2;
						}else if(ball->y < AIPaddle->y+10 && ball->y > AIPaddle->y+5){
								//SouthEast
								direction = 8;
						}
				}else if(ball->x <= 1){
						//Reset
						direction = 101;
						//ball->x=120;
						//ball->y=80;
				}else{
						ball->x -= 1;
				}
		}else if(direction == 4){
				if((ball->x == AIPaddle->x+1) && (ball->y > AIPaddle->y-2 && ball->y < AIPaddle->y+10)){
						//Ball hit AI Paddle, going right
						if(ball->y == AIPaddle->y+5 || ball->y == AIPaddle->y+4){
								//East
								direction = 1;
						}else if(ball->y > AIPaddle->y-2 && ball->y < AIPaddle->y+4){
								//NorthEast
								direction = 2;
						}else if(ball->y < AIPaddle->y+10 && ball->y > AIPaddle->y+5){
								//SouthEast
								direction = 8;
						}
				}else if(ball->x <= 1){
						//Reset
						direction = 101;
						//ball->x=120;
						//ball->y=80;
				}else if(ball->y == 0){
						direction = 6;
				}else{
						ball->x -= 1;
						ball->y -= 1;
				}
		}else if(direction == 6){
				if((ball->x == AIPaddle->x+1) && (ball->y > AIPaddle->y-2 && ball->y < AIPaddle->y+10)){
						//Ball hit AI Paddle, going right
						if(ball->y == AIPaddle->y+5 || ball->y == AIPaddle->y+4){
								//East
								direction = 1;
						}else if(ball->y > AIPaddle->y-2 && ball->y < AIPaddle->y+4){
								//NorthEast
								direction = 2;
						}else if(ball->y < AIPaddle->y+10 && ball->y > AIPaddle->y+5){
								//SouthEast
								direction = 8;
						}
				}else if(ball->x <= 1){
						//Reset
						direction = 101;
						//ball->x=120;
						//ball->y=80;
				}else if(ball->y >= 160){
						direction = 4;
				}else{
						ball->x -= 1;
						ball->y += 1;
				}
		}else if(direction == 2){
				if((ball->x == userPaddle->x-1) && (ball->y > userPaddle->y-1 && ball->y < userPaddle->y+9)){
						//Ball hit user Paddle, going left
						if(ball->y == userPaddle->y+5 || ball->y == userPaddle->y+4){
								//West
								direction = 5;
						}else if(ball->y > userPaddle->y-1 && ball->y < userPaddle->y+4){
								//NorthWest
								direction = 4;
						}else if(ball->y < userPaddle->y+9 && ball->y > userPaddle->y+5){
								//SouthWest
								direction = 6;
						}
				}else if(ball->x >= 235){
						//Reset
						direction = 102;
						//ball->x=120;
						//ball->y=80;
				}else if(ball->y <= 0){
						direction = 8;
				}else{
						ball->x += 1;
						ball->y -= 1;
				}
		}else if(direction == 8){
				if((ball->x == userPaddle->x-1) && (ball->y > userPaddle->y-1 && ball->y < userPaddle->y+9)){
						//Ball hit user Paddle, going left
						if(ball->y == userPaddle->y+5 || ball->y == userPaddle->y+4){
								//West
								direction = 5;
						}else if(ball->y > userPaddle->y-1 && ball->y < userPaddle->y+4){
								//NorthWest
								direction = 4;
						}else if(ball->y < userPaddle->y+9 && ball->y > userPaddle->y+5){
								//SouthWest
								direction = 6;
						}
				}else if(ball->x >= 235){
						//Reset
						direction = 102;
						//ball->x=120;
						//ball->y=80;
				}else if(ball->y >= 159){
						direction = 2;
				}else{
						ball->x += 1;
						ball->y += 1;
				}
		}


		return direction;
}

//Waits for userpaddle to go either up or down to start
int startPong(int direction){
		if(direction == 100){
				if(button_pressed(BUTTON_DOWN)){
						direction = 1;

				}else if(button_pressed(BUTTON_UP)){
						direction = 1;
				}
		}	
		return direction;
}

void clear_ball(volatile unsigned short* buffer, unsigned short color, struct square* s) {
		short row = s->y;
		short col = s->x;
		put_pixel(buffer, row, col, color);
		put_pixel(buffer, row+1, col, color);
		put_pixel(buffer, row, col+1, color);
		put_pixel(buffer, row+1, col+1, color);
}

void drawScore(int anchor, int score, volatile unsigned short* buffer, unsigned short color, unsigned short white){
		//Clear 3x4 to change AI Score
		if(anchor == 16){
				for(int i = 7; i <= 11; i++){
						for(int j = 115; j <= 117; j++){
								put_pixel(buffer, i, j, color);
						}
				}
				//Clear 3x4 to change User Score
		}else{
				for(int i = 7; i <= 11; i++){
						for(int j = 121; j <= 123; j++){
								put_pixel(buffer, i, j, color);
						}
				}
		} 
		if(anchor == 16){
				if(score == 1){
						put_pixel(buffer, 7, 116, white);
						put_pixel(buffer, 8, 116, white);
						put_pixel(buffer, 9, 116, white);
						put_pixel(buffer, 10, 116, white);
						put_pixel(buffer, 11, 116, white);
						put_pixel(buffer, 9, 119, white);
				}else if(score == 2){
						put_pixel(buffer, 7, 116, white);
						put_pixel(buffer, 9, 116, white);
						put_pixel(buffer, 11, 116, white);
						put_pixel(buffer, 7, 115, white);
						put_pixel(buffer, 9, 115, white);	
						put_pixel(buffer, 10, 115, white);
						put_pixel(buffer, 11, 115, white);
						put_pixel(buffer, 7, 117, white);
						put_pixel(buffer, 8, 117, white);
						put_pixel(buffer, 9, 117, white);
						put_pixel(buffer, 11, 117, white);
				}else if(score == 3){
						put_pixel(buffer, 7, 116, white);
						put_pixel(buffer, 7, 115, white);
						put_pixel(buffer, 7, 117, white);
						put_pixel(buffer, 9, 116, white);
						put_pixel(buffer, 9, 115, white);
						put_pixel(buffer, 9, 117, white);
						put_pixel(buffer, 11, 116, white);
						put_pixel(buffer, 11, 115, white);
						put_pixel(buffer, 11, 117, white);
						put_pixel(buffer, 8, 117, white);
						put_pixel(buffer, 10, 117, white);
				}else if(score == 0){
						put_pixel(buffer, 7, 116, white);
						put_pixel(buffer, 11, 116, white);
						put_pixel(buffer, 7, 115, white);
						put_pixel(buffer, 8, 115, white);
						put_pixel(buffer, 9, 115, white);
						put_pixel(buffer, 10, 115, white);
						put_pixel(buffer, 11, 115, white);	
						put_pixel(buffer, 7, 117, white);
						put_pixel(buffer, 8, 117, white);
						put_pixel(buffer, 9, 117, white);
						put_pixel(buffer, 10, 117, white);
						put_pixel(buffer, 11, 117, white);
				}
		}else if(anchor == 22){
				if(score == 1){
						put_pixel(buffer, 7, 122, white);
						put_pixel(buffer, 8, 122, white);
						put_pixel(buffer, 9, 122, white);
						put_pixel(buffer, 10, 122, white);
						put_pixel(buffer, 11, 122, white);
						put_pixel(buffer, 9, 119, white);
				}else if(score == 2){
						put_pixel(buffer, 7, 122, white);
						put_pixel(buffer, 9, 122, white);
						put_pixel(buffer, 11, 122, white);
						put_pixel(buffer, 7, 121, white);
						put_pixel(buffer, 9, 121, white);	
						put_pixel(buffer, 10, 121, white);
						put_pixel(buffer, 11, 121, white);
						put_pixel(buffer, 7, 123, white);
						put_pixel(buffer, 8, 123, white);
						put_pixel(buffer, 9, 123, white);
						put_pixel(buffer, 11, 123, white);
				}else if(score == 3){
						put_pixel(buffer, 7, 122, white);
						put_pixel(buffer, 7, 121, white);
						put_pixel(buffer, 7, 123, white);
						put_pixel(buffer, 9, 122, white);
						put_pixel(buffer, 9, 121, white);
						put_pixel(buffer, 9, 123, white);
						put_pixel(buffer, 11, 122, white);
						put_pixel(buffer, 11, 121, white);
						put_pixel(buffer, 11, 123, white);
						put_pixel(buffer, 8, 123, white);
						put_pixel(buffer, 10, 123, white);
				}else if(score == 0){
						put_pixel(buffer, 7, 122, white);
						put_pixel(buffer, 11, 122, white);
						put_pixel(buffer, 7, 121, white);
						put_pixel(buffer, 8, 121, white);
						put_pixel(buffer, 9, 121, white);
						put_pixel(buffer, 10, 121, white);
						put_pixel(buffer, 11, 121, white);	
						put_pixel(buffer, 7, 123, white);
						put_pixel(buffer, 8, 123, white);
						put_pixel(buffer, 9, 123, white);
						put_pixel(buffer, 10, 123, white);
						put_pixel(buffer, 11, 123, white);
				}

		}	

}

void showWinner(int who, volatile unsigned short* buffer){
		unsigned char red = add_color(20, 0, 0);
		unsigned char green = add_color(0, 20, 0);

		if(who == 0){
				//AI
				put_pixel(buffer, 74, 117, red);
				put_pixel(buffer, 74, 118, red);
				put_pixel(buffer, 75, 116, red);
				put_pixel(buffer, 76, 116, red);
				put_pixel(buffer, 77, 116, red);
				put_pixel(buffer, 78, 116, red);
				put_pixel(buffer, 75, 119, red);
				put_pixel(buffer, 76, 119, red);
				put_pixel(buffer, 77, 119, red);
				put_pixel(buffer, 78, 119, red);
				put_pixel(buffer, 76, 117, red);
				put_pixel(buffer, 76, 118, red);
				put_pixel(buffer, 78, 121, red);
				put_pixel(buffer, 78, 122, red);
				put_pixel(buffer, 78, 123, red);
				put_pixel(buffer, 74, 121, red);
				put_pixel(buffer, 74, 122, red);
				put_pixel(buffer, 74, 123, red);
				put_pixel(buffer, 75, 122, red);
				put_pixel(buffer, 76, 122, red);
				put_pixel(buffer, 77, 122, red);

				//Won!
				//'o'
				put_pixel(buffer, 81, 120, red);	
				put_pixel(buffer, 81, 121, red);
				put_pixel(buffer, 81, 119, red);
				put_pixel(buffer, 81, 118, red);
				put_pixel(buffer, 85, 120, red);
				put_pixel(buffer, 85, 121, red);
				put_pixel(buffer, 85, 119, red);
				put_pixel(buffer, 85, 118, red);
				put_pixel(buffer, 82, 118, red);
				put_pixel(buffer, 83, 118, red);
				put_pixel(buffer, 84, 118, red);
				put_pixel(buffer, 82, 121, red);
				put_pixel(buffer, 83, 121, red);
				put_pixel(buffer, 84, 121, red);
				//'w'
				put_pixel(buffer, 81, 112, red);
				put_pixel(buffer, 82, 112, red);
				put_pixel(buffer, 83, 112, red);
				put_pixel(buffer, 84, 112, red);
				put_pixel(buffer, 81, 116, red);
				put_pixel(buffer, 82, 116, red);
				put_pixel(buffer, 83, 116, red);
				put_pixel(buffer, 84, 116, red);
				put_pixel(buffer, 83, 114, red);
				put_pixel(buffer, 84, 114, red);
				put_pixel(buffer, 85, 113, red);
				put_pixel(buffer, 85, 115, red);
				//'n'
				put_pixel(buffer, 81, 123, red);
				put_pixel(buffer, 81, 124, red);
				put_pixel(buffer, 81, 125, red);
				put_pixel(buffer, 81, 126, red);
				put_pixel(buffer, 82, 123, red);
				put_pixel(buffer, 83, 123, red);
				put_pixel(buffer, 84, 123, red);
				put_pixel(buffer, 85, 123, red);
				put_pixel(buffer, 82, 126, red);
				put_pixel(buffer, 83, 126, red);
				put_pixel(buffer, 84, 126, red);
				put_pixel(buffer, 85, 126, red);
				//'!'
				put_pixel(buffer, 81, 128, red);
				put_pixel(buffer, 82, 128, red);
				put_pixel(buffer, 83, 128, red);
				put_pixel(buffer, 85, 128, red);

		}else{
				//User
				//'u'
				put_pixel(buffer, 74, 111, green);
				put_pixel(buffer, 75, 111, green);
				put_pixel(buffer, 76, 111, green);
				put_pixel(buffer, 77, 111, green);
				put_pixel(buffer, 78, 111, green);
				put_pixel(buffer, 74, 114, green);
				put_pixel(buffer, 75, 114, green);
				put_pixel(buffer, 76, 114, green);
				put_pixel(buffer, 77, 114, green);
				put_pixel(buffer, 78, 114, green);
				put_pixel(buffer, 78, 112, green);
				put_pixel(buffer, 78, 113, green);
				//'s'
				put_pixel(buffer, 74, 116, green);
				put_pixel(buffer, 74, 117, green);
				put_pixel(buffer, 74, 118, green);
				put_pixel(buffer, 74, 119, green);
				put_pixel(buffer, 76, 116, green);
				put_pixel(buffer, 76, 117, green);
				put_pixel(buffer, 76, 118, green);
				put_pixel(buffer, 76, 119, green);
				put_pixel(buffer, 78, 116, green);
				put_pixel(buffer, 78, 117, green);
				put_pixel(buffer, 78, 118, green);
				put_pixel(buffer, 78, 119, green);
				put_pixel(buffer, 75, 116, green);
				put_pixel(buffer, 77, 119, green);
				//'e'
				put_pixel(buffer, 74, 121, green);
				put_pixel(buffer, 74, 122, green);
				put_pixel(buffer, 74, 123, green);
				put_pixel(buffer, 74, 124, green);	
				put_pixel(buffer, 76, 121, green);
				put_pixel(buffer, 76, 122, green);
				put_pixel(buffer, 76, 123, green);
				put_pixel(buffer, 76, 124, green);		
				put_pixel(buffer, 78, 121, green);
				put_pixel(buffer, 78, 122, green);
				put_pixel(buffer, 78, 123, green);
				put_pixel(buffer, 78, 124, green);
				put_pixel(buffer, 75, 121, green);
				put_pixel(buffer, 77, 121, green);
				//'r'
				put_pixel(buffer, 74, 126, green);
				put_pixel(buffer, 74, 127, green);
				put_pixel(buffer, 74, 128, green);
				put_pixel(buffer, 74, 129, green);
				put_pixel(buffer, 75, 126, green);
				put_pixel(buffer, 75, 129, green);
				put_pixel(buffer, 76, 126, green);
				put_pixel(buffer, 76, 127, green);
				put_pixel(buffer, 76, 128, green);
				put_pixel(buffer, 77, 126, green);
				put_pixel(buffer, 78, 126, green);
				put_pixel(buffer, 77, 129, green);
				put_pixel(buffer, 78, 129, green);



				//Won!
				//'o'
				put_pixel(buffer, 81, 120, green);	
				put_pixel(buffer, 81, 121, green);
				put_pixel(buffer, 81, 119, green);
				put_pixel(buffer, 81, 118, green);
				put_pixel(buffer, 85, 120, green);
				put_pixel(buffer, 85, 121, green);
				put_pixel(buffer, 85, 119, green);
				put_pixel(buffer, 85, 118, green);
				put_pixel(buffer, 82, 118, green);
				put_pixel(buffer, 83, 118, green);
				put_pixel(buffer, 84, 118, green);
				put_pixel(buffer, 82, 121, green);
				put_pixel(buffer, 83, 121, green);
				put_pixel(buffer, 84, 121, green);
				//'w'
				put_pixel(buffer, 81, 112, green);
				put_pixel(buffer, 82, 112, green);
				put_pixel(buffer, 83, 112, green);
				put_pixel(buffer, 84, 112, green);
				put_pixel(buffer, 81, 116, green);
				put_pixel(buffer, 82, 116, green);
				put_pixel(buffer, 83, 116, green);
				put_pixel(buffer, 84, 116, green);
				put_pixel(buffer, 83, 114, green);
				put_pixel(buffer, 84, 114, green);
				put_pixel(buffer, 85, 113, green);
				put_pixel(buffer, 85, 115, green);
				//'n'
				put_pixel(buffer, 81, 123, green);
				put_pixel(buffer, 81, 124, green);
				put_pixel(buffer, 81, 125, green);
				put_pixel(buffer, 81, 126, green);
				put_pixel(buffer, 82, 123, green);
				put_pixel(buffer, 83, 123, green);
				put_pixel(buffer, 84, 123, green);
				put_pixel(buffer, 85, 123, green);
				put_pixel(buffer, 82, 126, green);
				put_pixel(buffer, 83, 126, green);
				put_pixel(buffer, 84, 126, green);
				put_pixel(buffer, 85, 126, green);
				//'!'
				put_pixel(buffer, 81, 128, green);
				put_pixel(buffer, 82, 128, green);
				put_pixel(buffer, 83, 128, green);
				put_pixel(buffer, 85, 128, green);

		}

}

/* the main function */
int main() {
		/* we set the mode to mode 4 with bg2 on */
		*display_control = MODE4 | BG2;

		/* make user paddle*/
		struct square s = {220, 80, 2, add_color(20, 20, 20)};

		/* make AI paddle*/
		struct square AI = {20,80,2, add_color(20,20,20)};	

		/* make ball*/
		struct square Ball = {120,80,2, add_color(0,10,20)};

		/* add black to the palette */
		unsigned char black = add_color(0, 0, 0);

		/* the buffer we start with */
		volatile unsigned short* buffer = front_buffer;

		//AI paddle movement
		int move = 0;

		//Ball movement
		int direction = 100;

		unsigned char white = add_color(20,20,20);

		int User_Score = 0;
		int AI_Score = 0;

		int AI_Won = 0;
		int User_Won = 0;

		/* clear whole screen first */
		clear_screen(front_buffer, black);
		clear_screen(back_buffer, black);


		/* loop forever */
		while (1) {
				if(AI_Won == 1 || User_Won == 1){
						if(AI_Won == 1){
								showWinner(0, buffer);
								/* Wait for vblank before switching buffers */
								wait_vblank();

								/* Swap the buffers */
								buffer = flip_buffers(buffer);	
								AI_Won = 3;
						}else if(User_Won == 1){
								showWinner(1, buffer);
								/* Wait for vblank before switching buffers */
								wait_vblank();

								/* Swap the buffers */
								buffer = flip_buffers(buffer);	

								User_Won = 3;
						}
				}else if(AI_Won == 3 || User_Won == 3){

				}else{
						/* Clear the screen - only the areas around the square! */
						update_screen(buffer, black, &s);
						update_screen(buffer, black, &AI);
						update_screen_ball(buffer, black, &Ball);

						direction = startPong(direction);		

						//Display Logo
						DrawPong(buffer, white);

						/* Draw the paddles */
						draw_square(buffer, &s);
						draw_square(buffer, &AI);

						//Draw Ball
						draw_ball(buffer, &Ball);		

						//Ball Movement
						direction = ballMovement(&Ball, &s, &AI, direction);	

						//Checks if ball hit wall
						if(direction == 101){
								clear_ball(buffer, black, &Ball);
								Ball.x = 120;
								Ball.y = 80;
								direction = 100;
								User_Score += 1;
								drawScore(22, User_Score, buffer, black, white);
								drawScore(16, AI_Score, buffer, black, white);
								/* Wait for vblank before switching buffers */
								wait_vblank();

								/* Swap the buffers */
								buffer = flip_buffers(buffer);	

						}else if(direction == 102){
								clear_ball(buffer, black, &Ball);
								Ball.x = 120;
								Ball.y = 80;
								direction = 100;
								AI_Score += 1;
								drawScore(16, AI_Score, buffer, black, white);
								drawScore(22, User_Score, buffer, black, white);
								/* Wait for vblank before switching buffers */
								wait_vblank();

								/* Swap the buffers */
								buffer = flip_buffers(buffer);	

						}

						//Check to see if anyone has hit 3 for winner
						if(AI_Score == 3){
								AI_Won = 1;
						}else if(User_Score == 3){
								User_Won = 1;
						}


						//Moving AI Paddle
						move = AImovement(&AI, move, direction, &Ball);

						/* handle button input */
						handle_buttons(&s);

						/* Wait for vblank before switching buffers */
						wait_vblank();

						/* Swap the buffers */
						buffer = flip_buffers(buffer);				
				}
		}
}

/* the game boy advance uses "interrupts" to handle certain situations
 * for now we will ignore these */
void interrupt_ignore() {
		/* do nothing */
}

/* this table specifies which interrupts we handle which way
 * for now, we ignore all of them */
typedef void (*intrp)();
const intrp IntrTable[13] = {
		interrupt_ignore,   /* V Blank interrupt */
		interrupt_ignore,   /* H Blank interrupt */
		interrupt_ignore,   /* V Counter interrupt */
		interrupt_ignore,   /* Timer 0 interrupt */
		interrupt_ignore,   /* Timer 1 interrupt */
		interrupt_ignore,   /* Timer 2 interrupt */
		interrupt_ignore,   /* Timer 3 interrupt */
		interrupt_ignore,   /* Serial communication interrupt */
		interrupt_ignore,   /* DMA 0 interrupt */
		interrupt_ignore,   /* DMA 1 interrupt */
		interrupt_ignore,   /* DMA 2 interrupt */
		interrupt_ignore,   /* DMA 3 interrupt */
		interrupt_ignore,   /* Key interrupt */
};

