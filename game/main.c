#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <stdio.h>
#include "buzzer.h"

#define GREEN_LED BIT6

AbRect rect1  = {abRectGetBounds, abRectCheck, {10, 1}};            /* shape used for paddles */
AbRect rect0  = {abRectGetBounds, abRectCheck, {1,  1}};            /* shape used for ball */

u_int bgColor = COLOR_BLACK, textColor = COLOR_WHITE;

static char score1 = 0, score2 = 0;                                 /* player 1 and 2 score counts */

int redrawScreen = 1;                                               /* boolean, whether screen needs to be redrawn */

Region wall = { {1,10}, {SHORT_EDGE_PIXELS, LONG_EDGE_PIXELS-10} }; /* region used for playing field */

/* layer0 contains ball, layer1 contains P1s paddle, layer2 contains P2s paddle */
Layer layer2 = { (AbShape *) &rect1, {screenWidth/2, 13}, {0,0}, {0,0}, COLOR_RED, 0, };
Layer layer1 = { (AbShape *) &rect1, {screenWidth/2, screenHeight - 13}, {0,0}, {0,0}, COLOR_GREEN, &layer2, };
Layer layer0 = { (AbShape *) &rect0, {(screenWidth/2)+10, (screenHeight/2)+5}, {0,0}, {0,0}, COLOR_WHITE, &layer1, };

/* moving layer, linked list of layer references, velocity represents one iteration of change (direction & magnitude) */
typedef struct MovLayer_s {
    Layer *layer;
    Vec2 velocity;
    struct MovLayer_s *next;
} MovLayer;

MovLayer ml2 = { &layer2, {0,0}, 0 };       /* initial value of {0,0} will be overwritten */
MovLayer ml1 = { &layer1, {0,0}, &ml2 };
MovLayer ml0 = { &layer0, {2,4}, &ml1 };

void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
    int row, col;
    MovLayer *movLayer;
    
    and_sr(~8); /* GIE off */
    for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
        Layer *l = movLayer->layer;
        l->posLast = l->pos;
        l->pos = l->posNext;
    }
    or_sr(8);   /* GIE on */
    for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
        Region bounds;
        layerGetBounds(movLayer->layer, &bounds);
        lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
                    bounds.botRight.axes[0], bounds.botRight.axes[1]);
        for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
            for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
                Vec2 pixelPos = {col, row};
                u_int color = bgColor;
                Layer *probeLayer;
                for (probeLayer = layers; probeLayer; probeLayer = probeLayer->next) { /* probe all layers, in order */
                    if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
                        color = probeLayer->color;
                        break; 
                    }   /* if probe check */
                }       /* for checking all layers at col, row */
                lcd_writeColor(color); 
            }           /* for col */
        }               /* for row */
    }                   /* for moving layer being updated */
}

/* play buzzer note */
void beepOnce(int note) {
    or_sr(8);
    for (int i = 0; i < 10000; i ++)
        buzzerSetPeriod(note);
    and_sr(~8);
}

void buzzerM() 
{
    switch (buzzerState) {
        case 0: /* state 0 - no beep */
            buzzerSetPeriod(N0);
            break;
        case 1: /* state 1 - good beep */
            buzzerBeepOnce(A3);
            break;
        case 2: /* state 2 - bad beep */
            buzzerBeepOnce(C3);
            break;
        case 3: /* state 3 - winner jingle */
            buzzerJingle();
            break;
    }
}

/* advance ball within field */
void mlAdvance(MovLayer *ml)
{
    Vec2 newPos;
    u_char axis;
    Region shapeBoundary;
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    
    for (axis = 0; axis < 2; axis ++) {
        if ((shapeBoundary.topLeft.axes[axis] < wall.topLeft.axes[axis]) || (shapeBoundary.botRight.axes[axis] > wall.botRight.axes[axis]) ) { /* outside field */
            int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
            newPos.axes[axis] += (2*velocity);
            if (shapeBoundary.botRight.axes[1] > wall.botRight.axes[1]) {   /* hits bottom, score for player 2 */
                char b[12];
                sprintf(b, "%d", ++score1);
                drawString5x7(117, 1, b, textColor, bgColor);
            }
            if (shapeBoundary.topLeft.axes[1] < wall.topLeft.axes[1]) {     /* hits top, score for player 1 */
                char b[12];
                sprintf(b, "%d", ++score2);
                drawString5x7(117, screenHeight - 8, b, textColor, bgColor);
            }
            beepOnce(C3);
        }
    }
    ml->layer->posNext = newPos;
}

/* bounce ball from paddles */
void checkBounce(MovLayer *ml0, MovLayer *ml1, MovLayer *ml2)
{
    Vec2 newPos0, newPos1, newPos2;
    Region ballBound, padBound1, padBound2;
    
    vec2Add(&newPos0, &ml0->layer->posNext, &ml0->velocity);
    abShapeGetBounds(ml0->layer->abShape, &newPos0, &ballBound);

    vec2Add(&newPos1, &ml1->layer->posNext, &ml1->velocity);
    abShapeGetBounds(ml1->layer->abShape, &newPos1, &padBound1);

    vec2Add(&newPos2, &ml2->layer->posNext, &ml2->velocity);
    abShapeGetBounds(ml2->layer->abShape, &newPos2, &padBound2);
    
    int half = (ballBound.topLeft.axes[0] + ballBound.botRight.axes[0]) / 2; /* horizontal midpoint of ball */

    /* bounce from paddle 1 */
    if (half >= padBound1.topLeft.axes[0] && half <= padBound1.botRight.axes[0] && ballBound.botRight.axes[1] > padBound1.topLeft.axes[1]) {
        int velocity = ml0->velocity.axes[1] = -ml0->velocity.axes[1];
        newPos0.axes[1] += (2*velocity);
        beepOnce(A3);
        ml0->layer->posNext = newPos0;
    }
    /* bounce from paddle 2 */
    if (half >= padBound2.topLeft.axes[0] && half <= padBound2.botRight.axes[0] && ballBound.botRight.axes[1] < padBound2.topLeft.axes[1]) {
        int velocity = ml0->velocity.axes[1] = -ml0->velocity.axes[1];
        newPos0.axes[1] += (2*velocity);
        beepOnce(A3);
        ml0->layer->posNext = newPos0;
    }
}

void movePaddles()
{
    u_int switches = p2sw_read();
    u_int sw1 = switches & (1 << 0);
    u_int sw2 = switches & (1 << 1);
    u_int sw3 = switches & (1 << 2);
    u_int sw4 = switches & (1 << 3);

    if (!sw1) { /* s1 is pressed, move paddle 1 left */
        Vec2 newPos; // TODO use a single Vec2 newPos
        vec2Add(&newPos, &ml2.layer->posNext, &ml2.velocity);
        Region boundary;
        abShapeGetBounds(ml2.layer->abShape, &newPos, &boundary);
        if (boundary.topLeft.axes[0] - 4 > wall.topLeft.axes[0])
            newPos.axes[0] += -4;
        ml2.layer->posNext = newPos;
    }
    if (!sw2) { /* s2 is pressed, move paddle 1 right */
        Vec2 newPos;
        vec2Add(&newPos, &ml2.layer->posNext, &ml2.velocity);
        Region boundary;
        abShapeGetBounds(ml2.layer->abShape, &newPos, &boundary);
        if (boundary.botRight.axes[0] + 4 < wall.botRight.axes[0])
            newPos.axes[0] += 4;
        ml2.layer->posNext = newPos;
    }
    if (!sw3) { /* s3 is pressed, move paddle 2 left */
        Vec2 newPos;
        vec2Add(&newPos, &ml1.layer->posNext, &ml1.velocity);
        Region boundary;
        abShapeGetBounds(ml1.layer->abShape, &newPos, &boundary);
        if (boundary.topLeft.axes[0] - 4 > wall.topLeft.axes[0])
            newPos.axes[0] += -4;
        ml1.layer->posNext = newPos;
    }
    if (!sw4) { /* s4 is pressed, move paddle 2 right */
        Vec2 newPos;
        vec2Add(&newPos, &ml1.layer->posNext, &ml1.velocity);
        Region boundary;
        abShapeGetBounds(ml1.layer->abShape, &newPos, &boundary);
        if (boundary.botRight.axes[0] + 4 < wall.botRight.axes[0])
            newPos.axes[0] += 4;
        ml1.layer->posNext = newPos;
    }
}

/* display title and controls */
void startScreen()
{
    clearScreen(bgColor);
    drawString5x7(1, 1,   "PONG", textColor, bgColor);
    drawString5x7(1, 20,  "CONTROLS", textColor, bgColor);
    drawString5x7(1, 30,  "S1 - P1 LEFT", textColor, bgColor);    
    drawString5x7(1, 40,  "S2 - P1 RIGHT", textColor, bgColor);
    drawString5x7(1, 50,  "S3 - P2 LEFT", textColor, bgColor);
    drawString5x7(1, 60,  "S4 - P2 RIGHT", textColor, bgColor);
    drawString5x7(1, 80,  "5 PTS WINS ROUND", textColor, bgColor);
    drawString5x7(1, 100, "PRESS S4 TO BEGIN", textColor, bgColor);
    while (1) { /* wait for s4 press to start game*/
        u_int sw4 = p2sw_read() & (1 << 3);
        if (!sw4)
            break;
    }
}

/* display winner and pause game */
void endScreen()
{
    buzzerSetPeriod(N0);
    redrawScreen = 1;
    drawString5x7(1, 30, "WINNER", textColor, bgColor);
    if (score1 > score2)
        drawString5x7(1, 40, "PLAYER 1", textColor, bgColor);
    else
        drawString5x7(1, 40, "PLAYER 2", textColor, bgColor);
    drawString5x7(1, 60, "PRESS S4 FOR", textColor, bgColor);
    drawString5x7(1, 70, "NEXT ROUND", textColor, bgColor);
    
    for (int i = 0; i < 3; i++) { /* play winner tone */
        beepOnce(C3);
        beepOnce(E3);
        beepOnce(G3);
        beepOnce(C4);
    }
  
    //or_sr(8);
    while (1) { // TODO simplify here and elsewhere
        u_int sw4 = p2sw_read() & (1 << 3);
        if (!sw4)
            break;
    }
    //and_sr(~8);
    score1 = score2 = 0; /* reset scores */
    
    layerInit(&layer0);
    layerDraw(&layer0);
    drawString5x7(117, 1, "0", textColor, bgColor);
    drawString5x7(117, screenHeight - 8, "0", textColor, bgColor);    
}

/* check if either player has accumulated 5 points */
void checkScore()
{
    if (score1 >= 5 || score2 >= 5)
        endScreen();
}

int main()
{
    P1DIR |= GREEN_LED;
    P1OUT |= GREEN_LED;
    
    configureClocks();
    lcd_init();             /* init lcd */
    p2sw_init(15);          /* prepare to read all switches */
    buzzer_init();          /* init buzzer */
    enableWDTInterrupts();  /* enable periodic interrupt */
    or_sr(0x8);             /* GIE, enable interrupts */
    
    startScreen();
    
    layerInit(&layer0);     /* sets bounds into a consistent state */
    layerDraw(&layer0);     /* render all layers */
    drawString5x7(117, 1, "0", textColor, bgColor);
    drawString5x7(117, screenHeight - 8, "0", textColor, bgColor);
    
    for(;;) { 
        while (!redrawScreen) {     /* pause CPU if screen doesn't need updating */
            P1OUT &= ~GREEN_LED;    /* green LED off */
            or_sr(0x10);            /* CPU off */
        }
        P1OUT |= GREEN_LED;         /* green LED on when CPU on */
        redrawScreen = 0;
        mlAdvance(&ml0);
        checkBounce(&ml0, &ml1, &ml2);
        checkScore();
        movePaddles();
        movLayerDraw(&ml0, &layer0);
    }
}

/* watchdog timer interrupt handler, 15 interrupts/sec */
void wdt_c_handler()
{
    static short count = 0;
    P1OUT |= GREEN_LED;         /* green LED on when CPU on */
    count ++;
    if (count == 15) {
        redrawScreen = 1;
        count = 0;
        buzzerSetPeriod(N0); // TODO try moving this out
    } 
    P1OUT &= ~GREEN_LED;        /* green LED off when CPU off */
}
