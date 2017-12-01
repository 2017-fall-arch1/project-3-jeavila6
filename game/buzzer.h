#ifndef buzzer_included
#define buzzer_included

// Musical notes
#define N0 0000
#define C3  7645
#define C3S 7216
#define D3  6811
#define D3S 6428
#define E3  6068
#define F3  5727
#define F3S 5405
#define G3  5102
#define G3S 4816
#define A3  4545
#define A3S 4290
#define B3  4050
#define C4  3822
#define C4S 3608
#define D4  3405
#define D4S 3214
#define E4  3034
#define F4  2863
#define F4S 2703
#define G4  2551
#define G4S 2408
#define A4  2273
#define A4S 2145
#define B4  2025
#define C5  1911
#define C5S 1804
#define D5S 1607

void buzzer_init();
void buzzer_advance_frequency();
void buzzerSetPeriod(short cycles);
void singA();
void singB();
void singC();
void singD();


#endif // buzzer_included
