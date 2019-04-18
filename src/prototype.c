#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

//PORTA is red ground
//PORTC is blue ground

volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s

	// AVR output compare register OCR1A.
	OCR1A = 125;	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	// AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	//Enable global interrupts
	SREG |= 0x80; // 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}

// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

//ADC Init
void InitADC(void)
{
    ADMUX|=(1<<REFS0);   
    ADCSRA|=(1<<ADEN)|(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2); //ENABLE ADC, PRESCALER 128
}
//ADC read
uint16_t readadc(uint8_t ch)
{
    ch&=0b00000111;         //ANDing to limit input to 7
    ADMUX = (ADMUX & 0xf8)|ch;  //Clear last 3 bits of ADMUX, OR with ch
    ADCSRA|=(1<<ADSC);        //START CONVERSION
    while((ADCSRA)&(1<<ADSC));    //WAIT UNTIL CONVERSION IS COMPLETE
    return(ADC);        //RETURN ADC VALUE
}

//Displays a red dot at given position on Matrix
//May have to change to do some logic for display for gho|sts
//PORTA and C
unsigned char temp1;
void displayRed(unsigned char x, unsigned char y){
	PORTD = 0xFF;
	temp1 = (~y << 6);
	PORTC = (temp1 | 0x0F);
	PORTA = (~y | 0x03);
	PORTB = x;
}

//Displays a blue dot at given position on Matrix
//PORTD
void displayBlue(unsigned char x, unsigned char y){
	PORTA = 0xFF;
	PORTC = 0xFF;
	PORTD = ~y;
	PORTB = x;
}

unsigned char countX;
unsigned char countY;
unsigned short tempX, tempY;

void playerMove(){
	tempX = readadc(0);
	tempY = readadc(1);
	if(tempX > 0x03FC){ //go left
		if(countX < 7){
			++countX;
		}
	}
	else if(tempX < 0x0380){ //go right
		if(countX > 0){
			--countX;
		}
	}
	else if(tempY > 0x03FC){ //go down
		if(countY < 6){
			++countY;
		}
	}
	else if(tempY < 0x0300){ //go up
		if(countY > 0){
			--countY;
		}
	}
}

unsigned char gameOn;

void gameStart(){
	if(~PINC & 0x01){
		gameOn = 1;
	}
	while(~PINC & 0x01){
		//wait
	}
}

void gameQuit(){
	if(~PINC & 0x01){
		gameOn = 0;
	}
	while(~PINC & 0x01){
		//wait
	}
}

void displayScore(unsigned char score, unsigned char position){
	PORTA = 0xFF;
	PORTC = 0xFF;
	PORTD = ~position;
	PORTB = score;
}

unsigned char ghost1;
unsigned char ghost1X;
unsigned char ghost1Y;
unsigned char ghost2;
unsigned char ghost2X;
unsigned char ghost2Y;
unsigned char ghost3;
unsigned char ghost3X;
unsigned char ghost3Y;
unsigned char ghost4;
unsigned char ghost4X;
unsigned char ghost4Y;

void ghost1Move(){ //Calculates positions for ghost movement
	if(ghost1 == 0){ //both x and y increasing
		if(ghost1X == 7 && ghost1Y == 6){
			--ghost1X;
			--ghost1Y;
			ghost1 = 3;
		}
		else if(ghost1X == 7){
			--ghost1X;
			++ghost1Y;
			ghost1 = 1;
		}
		else if(ghost1Y == 6){
			++ghost1X;
			--ghost1Y;
			ghost1 = 2;
		}
		else{
			++ghost1X;
			++ghost1Y;
		}
	}
	else if(ghost1 == 1){ //x decreasing, y increasing
		if(ghost1X == 0 && ghost1Y == 6){
			++ghost1X;
			--ghost1Y;
			ghost1 = 2;
		}
		else if(ghost1X == 0){
			++ghost1X;
			++ghost1Y;
			ghost1 = 0;
		}
		else if(ghost1Y == 6){
			--ghost1X;
			--ghost1Y;
			ghost1 = 3;
		}
		else{
			--ghost1X;
			++ghost1Y;
		}
	}
	else if(ghost1 == 2){ //x increasing, y decreasing
		if(ghost1X == 7 && ghost1Y == 0){
			--ghost1X;
			++ghost1Y;
			ghost1 = 1;
		}
		else if(ghost1X == 7){
			--ghost1X;
			--ghost1Y;
			ghost1 = 3;
		}
		else if(ghost1Y == 0){
			++ghost1X;
			++ghost1Y;
			ghost1 = 0;
		}
		else{
			++ghost1X;
			--ghost1Y;
		}
	}
	else if(ghost1 == 3){ //both x and y decreasing
		if(ghost1X == 0 && ghost1Y == 0){
			++ghost1X;
			++ghost1Y;
			ghost1 = 0;
		}
		else if(ghost1X == 0){
			++ghost1X;
			--ghost1Y;
			ghost1 = 2;
		}
		else if(ghost1Y == 0){
			--ghost1X;
			++ghost1Y;
			ghost1 = 1;
		}
		else{
			--ghost1X;
			--ghost1Y;
		}
	}
}

void ghost2Move(){ //Calculates positions for ghost movement
	if(ghost2 == 0){ //both x and y increasing
		if(ghost2X == 7 && ghost2Y == 6){
			--ghost2X;
			--ghost2Y;
			ghost2 = 3;
		}
		else if(ghost2X == 7){
			--ghost2X;
			++ghost2Y;
			ghost2 = 1;
		}
		else if(ghost2Y == 6){
			++ghost2X;
			--ghost2Y;
			ghost2 = 2;
		}
		else{
			++ghost2X;
			++ghost2Y;
		}
	}
	else if(ghost2 == 1){ //x decreasing, y increasing
		if(ghost2X == 0 && ghost2Y == 6){
			++ghost2X;
			--ghost2Y;
			ghost2 = 2;
		}
		else if(ghost2X == 0){
			++ghost2X;
			++ghost2Y;
			ghost2 = 0;
		}
		else if(ghost2Y == 6){
			--ghost2X;
			--ghost2Y;
			ghost2 = 3;
		}
		else{
			--ghost2X;
			++ghost2Y;
		}
	}
	else if(ghost2 == 2){ //x increasing, y decreasing
		if(ghost2X == 7 && ghost2Y == 0){
			--ghost2X;
			++ghost2Y;
			ghost2 = 1;
		}
		else if(ghost2X == 7){
			--ghost2X;
			--ghost2Y;
			ghost2 = 3;
		}
		else if(ghost2Y == 0){
			++ghost2X;
			++ghost2Y;
			ghost2 = 0;
		}
		else{
			++ghost2X;
			--ghost2Y;
		}
	}
	else if(ghost2 == 3){ //both x and y decreasing
		if(ghost2X == 0 && ghost2Y == 0){
			++ghost2X;
			++ghost2Y;
			ghost2 = 0;
		}
		else if(ghost2X == 0){
			++ghost2X;
			--ghost2Y;
			ghost2 = 2;
		}
		else if(ghost2Y == 0){
			--ghost2X;
			++ghost2Y;
			ghost2 = 1;
		}
		else{
			--ghost2X;
			--ghost2Y;
		}
	}
}

void ghost3Move(){ //Calculates positions for ghost movement
	if(ghost3 == 0){ //both x and y increasing
		if(ghost3X == 7 && ghost3Y == 6){
			--ghost3X;
			--ghost3Y;
			ghost3 = 3;
		}
		else if(ghost3X == 7){
			--ghost3X;
			++ghost3Y;
			ghost3 = 1;
		}
		else if(ghost3Y == 6){
			++ghost3X;
			--ghost3Y;
			ghost3 = 2;
		}
		else{
			++ghost3X;
			++ghost3Y;
		}
	}
	else if(ghost3 == 1){ //x decreasing, y increasing
		if(ghost3X == 0 && ghost3Y == 6){
			++ghost3X;
			--ghost3Y;
			ghost3 = 2;
		}
		else if(ghost3X == 0){
			++ghost3X;
			++ghost3Y;
			ghost3 = 0;
		}
		else if(ghost3Y == 6){
			--ghost3X;
			--ghost3Y;
			ghost3 = 3;
		}
		else{
			--ghost3X;
			++ghost3Y;
		}
	}
	else if(ghost3 == 2){ //x increasing, y decreasing
		if(ghost3X == 7 && ghost3Y == 0){
			--ghost3X;
			++ghost3Y;
			ghost3 = 1;
		}
		else if(ghost3X == 7){
			--ghost3X;
			--ghost3Y;
			ghost3 = 3;
		}
		else if(ghost3Y == 0){
			++ghost3X;
			++ghost3Y;
			ghost3 = 0;
		}
		else{
			++ghost3X;
			--ghost3Y;
		}
	}
	else if(ghost3 == 3){ //both x and y decreasing
		if(ghost3X == 0 && ghost3Y == 0){
			++ghost3X;
			++ghost3Y;
			ghost3 = 0;
		}
		else if(ghost3X == 0){
			++ghost3X;
			--ghost3Y;
			ghost3 = 2;
		}
		else if(ghost3Y == 0){
			--ghost3X;
			++ghost3Y;
			ghost3 = 1;
		}
		else{
			--ghost3X;
			--ghost3Y;
		}
	}
}

void ghost4Move(){ //Calculates positions for ghost movement
	if(ghost4 == 0){ //both x and y increasing
		if(ghost4X == 7 && ghost4Y == 6){
			--ghost4X;
			--ghost4Y;
			ghost4 = 3;
		}
		else if(ghost4X == 7){
			--ghost4X;
			++ghost4Y;
			ghost4 = 1;
		}
		else if(ghost4Y == 6){
			++ghost4X;
			--ghost4Y;
			ghost4 = 2;
		}
		else{
			++ghost4X;
			++ghost4Y;
		}
	}
	else if(ghost4 == 1){ //x decreasing, y increasing
		if(ghost4X == 0 && ghost4Y == 6){
			++ghost4X;
			--ghost4Y;
			ghost4 = 2;
		}
		else if(ghost4X == 0){
			++ghost4X;
			++ghost4Y;
			ghost4 = 0;
		}
		else if(ghost4Y == 6){
			--ghost4X;
			--ghost4Y;
			ghost4 = 3;
		}
		else{
			--ghost4X;
			++ghost4Y;
		}
	}
	else if(ghost4 == 2){ //x increasing, y decreasing
		if(ghost4X == 7 && ghost4Y == 0){
			--ghost4X;
			++ghost4Y;
			ghost4 = 1;
		}
		else if(ghost4X == 7){
			--ghost4X;
			--ghost4Y;
			ghost4 = 3;
		}
		else if(ghost4Y == 0){
			++ghost4X;
			++ghost4Y;
			ghost4 = 0;
		}
		else{
			++ghost4X;
			--ghost4Y;
		}
	}
	else if(ghost4 == 3){ //both x and y decreasing
		if(ghost4X == 0 && ghost4Y == 0){
			++ghost4X;
			++ghost4Y;
			ghost4 = 0;
		}
		else if(ghost4X == 0){
			++ghost4X;
			--ghost4Y;
			ghost4 = 2;
		}
		else if(ghost4Y == 0){
			--ghost4X;
			++ghost4Y;
			ghost4 = 1;
		}
		else{
			--ghost4X;
			--ghost4Y;
		}
	}
}

unsigned char gameLose;

void intersect(){ //Checks if player intersects with a ghost
	if(ghost1X == countX && ghost1Y == countY){
		gameLose = 1;
		gameOn = 0;
	}
	if(ghost2X == countX && ghost2Y == countY){
		gameLose = 1;
		gameOn = 0;
	}
	if(ghost3X == countX && ghost3Y == countY){
		gameLose = 1;
		gameOn = 0;
	}
	if(ghost4X == countX && ghost4Y == countY){
		gameLose = 1;
		gameOn = 0;
	}
}

int main(){
	//init io
	DDRD = 0xFF;
	DDRB = 0xFF;
	DDRA = 0xFF;
	DDRC = 0xF0;
	
	PORTD = 0x00;
	PORTB = 0x00;
	PORTA = 0x00;
	PORTC = 0x0F;

	TimerSet(1); 	// Timer Period: 1ms
	TimerOn();		// Turn timer on
	
	gameOn = 0;
	unsigned char score = 0x00;
	unsigned char hiScore;
	
	//load hiScore and prevScore from eeprom into variable
	//hiScore = eeprom_read_byte...
	//prevScore = eeprom_read_byte...
	uint8_t ByteOfData;
	ByteOfData = eeprom_read_byte((uint8_t*)46);
	hiScore = (unsigned char)ByteOfData;
	if(hiScore == 0xFF){
		hiScore = 0x01;
	}
	
	//declare and initialize game timer variables
	unsigned short gameCounter;
	unsigned char displayCounter;
	
	unsigned char ghost1Counter;
	unsigned char ghost2Counter;
	unsigned char ghost3Counter;
	unsigned char ghost4Counter;
	
	//ADC init
	InitADC();
	
	while(1){
		displayScore(hiScore, 0x80);
		while(!gameOn){
			gameStart();
			if(~PINC & 0x02){
				if(!(hiScore == 0x01)){
					hiScore = 0x01;
					displayScore(hiScore, 0x80);
					//write hiScore into EEPROM
					ByteOfData = (uint8_t)hiScore;
					eeprom_write_byte((uint8_t*)46, ByteOfData);
				}
			}
		}
		
		gameCounter = 0;
		while(gameCounter < 200){
			++gameCounter;
			while(!TimerFlag);
			TimerFlag = 0;
		}
		//reset game variables
		gameLose = 0;
		score = 0;
		gameCounter = 0;
		displayCounter = 0;
		ghost1Counter = 0;
		ghost2Counter = 0;
		ghost3Counter = 0;
		ghost4Counter = 0;
		countX = 4;
		countY = 3;
		//ghost position variables
		ghost1 = 0;
		ghost1X = 0;
		ghost1Y = 0;
		ghost2 = 0;
		ghost2X = 0;
		ghost2Y = 6;
		ghost3 = 0;
		ghost3X = 7;
		ghost3Y = 0;
		ghost4 = 0;
		ghost4X = 7;
		ghost4Y = 6;
		
		while(gameOn){ //game played in this while loop
			gameQuit();
			if(ghost1Counter == 100 && !gameLose){
				ghost1Move();
				intersect();
				ghost1Counter = 0;
			}
			if(ghost2Counter == 150 && !gameLose){
				ghost2Move();
				intersect();
				ghost2Counter = 0;
			}
			if(ghost3Counter == 200 && !gameLose){
				ghost3Move();
				intersect();
				ghost3Counter = 0;
			}
			if(ghost4Counter == 250 && !gameLose){
				ghost4Move();
				intersect();
				ghost4Counter = 0;
			}
			if(gameCounter == 150 && !gameLose){
				playerMove();
				if(score < 0xFE){
					++score;
				}
				intersect();
				gameCounter = 0;
			}
			++gameCounter;
			++ghost1Counter;
			++ghost2Counter;
			++ghost3Counter;
			++ghost4Counter;
			if(displayCounter == 0){
				//have to use Red for now since blue wont work
				displayBlue((0x01 << countX), (0x01 << countY));
				displayCounter = 1;
			}
			else if(displayCounter == 1){
				displayRed((0x01 << ghost1X), (0x01 << ghost1Y));
				displayCounter = 2;
			}
			else if(displayCounter == 2){
				displayRed((0x01 << ghost2X), (0x01 << ghost2Y));
				displayCounter = 3;
			}
			else if(displayCounter == 3){
				displayRed((0x01 << ghost3X), (0x01 << ghost3Y));
				displayCounter = 4;
			}
			else if(displayCounter == 4){
				displayRed((0x01 << ghost4X), (0x01 << ghost4Y));
				displayCounter = 5;
			}
			else if(displayCounter == 5){
				displayScore(score, 0x80);
				displayCounter = 0;
			}
			while(!TimerFlag);
			TimerFlag = 0;
		}
		
		gameCounter = 0;
		while(gameCounter < 500){ //game loss screen for 500ms
			++gameCounter;
			if(displayCounter == 0){
				//have to use Red for now since blue wont work
				displayBlue((0x01 << countX), (0x01 << countY));
				displayCounter = 1;
			}
			else if(displayCounter == 1){
				displayRed((0x01 << ghost1X), (0x01 << ghost1Y));
				displayCounter = 2;
			}
			else if(displayCounter == 2){
				displayRed((0x01 << ghost2X), (0x01 << ghost2Y));
				displayCounter = 3;
			}
			else if(displayCounter == 3){
				displayRed((0x01 << ghost3X), (0x01 << ghost3Y));
				displayCounter = 4;
			}
			else if(displayCounter == 4){
				displayRed((0x01 << ghost4X), (0x01 << ghost4Y));
				displayCounter = 5;
			}
			else if(displayCounter == 5){
				displayScore(score, 0x80);
				displayCounter = 0;
			}
			while(!TimerFlag);
			TimerFlag = 0;
		}
		
		displayRed(0x00, 0x00);
		if(score > hiScore){ //game ended, check if new hiScore
			hiScore = score;
			//write hiScore into EEPROM
			ByteOfData = (uint8_t)hiScore;
			eeprom_write_byte((uint8_t*)46, ByteOfData);
		}
	}
	
	return 0;
}