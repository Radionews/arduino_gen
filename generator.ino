#include <GyverEncoder.h>
#include <Wire.h> 
#include <LiquidCrystal.h> // Подключение библиотеки
#include <avr/io.h>
#include <avr/interrupt.h>

#define CLK 7
#define DT 6
#define SW 8

Encoder enc1(CLK, DT, SW);
LiquidCrystal lcd(12, 10, 9, 4, 3, 2);

int value = 0;
unsigned char mode_gen = 0;
unsigned char range = 0;
unsigned long divider = 1;
volatile unsigned long iMicros = 0;
volatile float Ttime;     // переменная для хранения общей длительности периода
volatile float frequency; // переменная для хранения частоты
float Ttime_ind = 0;

volatile unsigned int clock_k[7] = {65535,32767,16383,4095,1023,255,127};

void setup() {
    lcd.begin(16, 2);
    lcd.print("Aka Kasyan");
    lcd.setCursor(0, 1);
    lcd.print("Radionews");
    
    enc1.setType(TYPE2);
    enc1.setFastTimeout(40);

    //настройка таймера 2 на вывод
    pinMode(11, OUTPUT); 
    TCCR2A = 0b01000010;
    TCCR2B = 0b00000001;
    OCR2A = 1;

    pinMode(5, INPUT);

    delay(1000);
    //настройка таймера 1 для счета импульсов
    cli(); // отключить глобальные прерывания
    TCCR1A = 0; // установить регистры в 0
    TCCR1B = 0; 

    OCR1A = 65535; // установка регистра совпадения
    TCCR1B |= (1 << WGM12); // включение в CTC режим

    TIMSK1 |= (1 << OCIE1A);  // включение прерываний по совпадению
    TCCR1B |= (1 << CS12)|(1 << CS11);
    sei();  // включить глобальные прерывания
}

void loop() {
  
  enc1.tick();
  if (enc1.isRight()) {value++; if(value>1252) value = 1252;}
  if (enc1.isLeft()) {value--;  if(value<0) value = 0;}       
  if (enc1.isRightH()) {value+=5; if(value>1252) value = 1252;}
  if (enc1.isLeftH()) {value-=5;  if(value<0) value = 0;}     
  if (enc1.isFastR()) {value+=10; if(value>1252) value = 1252;}
  if (enc1.isFastL()) {value-=10;  if(value<0) value = 0;}   
  if (enc1.isTurn()) {
    if(mode_gen==0){                                        
      if(value>=0 && value<256)     {TCCR2B = 1; OCR2A = value; divider = value;}
      if(value>=256 && value<481)   {TCCR2B = 2; OCR2A = value%256 + 31; divider = OCR2A; divider *= 8;}
      if(value>=481 && value<673)   {TCCR2B = 3; OCR2A = value%481 + 63; divider = OCR2A; divider *= 32;}
      if(value>=673 && value<802)   {TCCR2B = 4; OCR2A = value%673 + 127; divider = OCR2A; divider *= 64;}
      if(value>=802 && value<931)   {TCCR2B = 5; OCR2A = value%802 + 127; divider = OCR2A; divider *= 128;}
      if(value>=931 && value<1060)  {TCCR2B = 6; OCR2A = value%931 + 127; divider = OCR2A; divider *= 256;}
      if(value>=1060 && value<1253) {TCCR2B = 7; OCR2A = value%1060 + 63; divider = OCR2A; divider *= 1024;}
    }
    else{
      if(value>255) value = 255;
      OCR2A = value;
    }
    Ttime_ind = 0;
  }
  if (enc1.isSingle()){
    mode_gen++;
    if(mode_gen==0) { TCCR2A = 0b01000010; TCCR2B = 0b00000001; OCR2A = value;}
    if(mode_gen==1) { TCCR2A = 0b10000011; TCCR2B = 0b00000001; OCR2A = value;}
    if(mode_gen==2) { TCCR2A = 0b10000011; TCCR2B = 0b00000010; OCR2A = value;}
    if(mode_gen==3) { TCCR2A = 0b10000011; TCCR2B = 0b00000011; OCR2A = value;}
    if(mode_gen==4) { TCCR2A = 0b10000011; TCCR2B = 0b00000100; OCR2A = value;}
    if(mode_gen==5) { TCCR2A = 0b10000011; TCCR2B = 0b00000101; OCR2A = value;}
    if(mode_gen==6) { TCCR2A = 0b10000011; TCCR2B = 0b00000110; OCR2A = value;}
    if(mode_gen==7) { TCCR2A = 0b10000011; TCCR2B = 0b00000111; OCR2A = value;}
    if(mode_gen==8) { TCCR2A = 0b01000010; TCCR2B = 0b00000001; OCR2A = value; mode_gen = 0;}  
    Ttime_ind = 0;
  }
  if(Ttime!=Ttime_ind){
    frequency = 1 * (clock_k[range]*1000000.0)/Ttime;
    lcd.clear();
    lcd.setCursor(0, 0);
    if(mode_gen==0){
      lcd.print("Gen: ");
      lcd.print(8000000.0/(1.0+divider));
    }
    else{
      lcd.print("PWM");
      lcd.print(mode_gen);
      lcd.print(" ");
      lcd.print((value/255.0)*100.0);
      lcd.print("%");
    }
    lcd.setCursor(0,1);
    lcd.print("Freq: ");
    lcd.print(frequency/1000);
    lcd.print(" kHz");
    Ttime_ind = Ttime;
    
    if(Ttime>1000000 && range<6)  {range++; OCR1A = clock_k[range]; TCNT1H = 0; TCNT1L = 0; }
    if(Ttime<100000 && range>0)    {range--; OCR1A = clock_k[range]; TCNT1H = 0; TCNT1L = 0; }
  }
}

ISR(TIMER1_COMPA_vect)
{
  iMicros = micros() - iMicros;
  Ttime = iMicros;
  iMicros = micros();
}
