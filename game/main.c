#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include "buzzer.h"
//#include <abCircle.h>

#define GREEN_LED BIT6
//Region wall = {{1,10}, {SHORT_EDGE_PIXELS, LONG_EDGE_PIXELS-10}};
Region wall = {{1,15}, {SHORT_EDGE_PIXELS, LONG_EDGE_PIXELS-10}};

AbRect rect1 = {abRectGetBounds, abRectCheck, {10, 1}};
AbRect rect0 = {abRectGetBounds, abRectCheck, {1,  1}};

u_int bgColor = COLOR_BLACK, textColor = COLOR_WHITE;

static char score1 = 0, score2 = 0;

int redrawScreen = 1; // boolean, whether screen needs to be redrawn
Region fieldFence;    // fence around playing field

AbRectOutline fieldOutline = {
    abRectOutlineGetBounds, abRectOutlineCheck,   
    {screenWidth/2 - 1, screenHeight/2 - 10}
};

Layer layer2 = {                                // layer with green padle
    (AbShape *) &rect1,
    {screenWidth/2, 13},
    {0,0}, {0,0},
    COLOR_RED,
    0,
};

Layer layer1 = {                                // layer with green padle
    (AbShape *) &rect1,
    {screenWidth/2, screenHeight - 13},
    {0,0}, {0,0},
    COLOR_GREEN,
    &layer2,
};

Layer layer0 = {                                // layer with white ball
    (AbShape *) &rect0,
    {(screenWidth/2)+10, (screenHeight/2)+5},   // bit below & right of center
    {0,0}, {0,0},                               // last and next pos
    COLOR_WHITE,
    &layer1,                                    // next layer
};

// Moving layer, linked list of layer references, velocity represents one iteration of change (direction & magnitude)
// (reference to layer, velocity vector, reference to next MovLayer)
typedef struct MovLayer_s {
    Layer *layer;
    Vec2 velocity;
    struct MovLayer_s *next;
} MovLayer;

/* initial value of {0,0} will be overwritten */
MovLayer ml2 = { &layer2, {0,0}, 0 };
MovLayer ml1 = { &layer1, {0,0}, &ml2 };    // TODO add second paddle velocity 2,0
MovLayer ml0 = { &layer0, {2,4}, &ml1}; // ball

void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
    int row, col;
    MovLayer *movLayer;
    
    and_sr(~8); // disable interrupts (GIE off)
    for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
        Layer *l = movLayer->layer;
        l->posLast = l->pos;
        l->pos = l->posNext;
    }
    or_sr(8);   // disable interrupts (GIE on)
    
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
                    } /* if probe check */
                } // for checking all layers at col, row
                lcd_writeColor(color); 
            } // for col
        } // for row
    } // for moving layer being updated
}

void beepOnce(int note) {
    or_sr(8);
    for (int i = 0; i < 10000; i ++)
        buzzerSetPeriod(note);
    and_sr(~8);
}

void mlAdvance(MovLayer *ml)
{
    Vec2 newPos;
    u_char axis;
    Region shapeBoundary;
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    
    for (axis = 0; axis < 2; axis ++) {
        if ((shapeBoundary.topLeft.axes[axis] < wall.topLeft.axes[axis]) || (shapeBoundary.botRight.axes[axis] > wall.botRight.axes[axis]) ) { // if outside fence
            int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
            newPos.axes[axis] += (2*velocity);
            if (shapeBoundary.botRight.axes[1] > wall.botRight.axes[1]) { // hits top, score for player 1
                char b[12];
                sprintf(b, "%d", ++score1);
                drawString5x7(117, 1, b, textColor, bgColor);
            }
            if (shapeBoundary.topLeft.axes[1] < wall.topLeft.axes[1]) { // hits top, score for player 2
                char b[12];
                sprintf(b, "%d", ++score2);
                drawString5x7(117, screenHeight - 8, b, textColor, bgColor);
            }
            beepOnce(C3);
        }
    }
    ml->layer->posNext = newPos;

}



void checkBounce(MovLayer *ml0, MovLayer *ml1)
{
    Vec2 newPos0;
    vec2Add(&newPos0, &ml0->layer->posNext, &ml0->velocity);
    Region ballBound;
    abShapeGetBounds(ml0->layer->abShape, &newPos0, &ballBound);
    
    Vec2 newPos1;
    vec2Add(&newPos1, &ml1->layer->posNext, &ml1->velocity);
    Region padBound;
    abShapeGetBounds(ml1->layer->abShape, &newPos1, &padBound);
    
    int half = (ballBound.topLeft.axes[0] + ballBound.botRight.axes[0]) / 2;
    
    //int third = (padBound.topLeft.axes[0] + padBound.botRight.axes[0]) / 3;
    
    if (half >= padBound.topLeft.axes[0] && half <= padBound.botRight.axes[0] && ballBound.botRight.axes[1] > padBound.topLeft.axes[1]) {
        int velocity = ml0->velocity.axes[1] = -ml0->velocity.axes[1];
        newPos0.axes[1] += (2*velocity);
        
//         if ((half >= padBound.topLeft.axes[0] && half < padBound.topLeft.axes[0] + 3) || (half <= padBound.botRight.axes[0] && half > padBound.botRight.axes[0] - 3)) {
//             velocity = ml0->velocity.axes[1] = -ml0->velocity.axes[1];
//             newPos0.axes[1] += (2*velocity) + 1;
//         }
        beepOnce(A3);
        ml0->layer->posNext = newPos0;
        
        
    }
}

int main()
{
    P1DIR |= GREEN_LED;     // green LED will be on when CPU is on
    P1OUT |= GREEN_LED;
    
    configureClocks();
    lcd_init();             // init lcd
    p2sw_init(15);          // motion demo uses 1, use 15 to read all switches
    buzzer_init();          // init buzzer
    
    layerInit(&layer0);     // sets bounds into a consistent state
    layerDraw(&layer0);     // renders all layers, pixels not contained in a layer are
    
    enableWDTInterrupts();  // enable periodic interrupt
    or_sr(0x8);             // GIE (enable interrupts)
    
    drawString5x7(1, screenHeight - 8, "Pong v8",  textColor, bgColor); // (column, row, string, fg color, bg color)
    drawString5x7(117, 1, "0", textColor, bgColor);
    drawString5x7(117, screenHeight - 8, "0", textColor, bgColor);
    
    for(;;) { 
        while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
            P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
            or_sr(0x10);	      /**< CPU OFF */
            //drawString5x7(1, 1, "OF",  textColor, bgColor);
        }
        P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
        redrawScreen = 0;
        
        mlAdvance(&ml0);
        checkBounce(&ml0, &ml1);
        moveOpponent(&ml0, &ml2);
        readSwitches();
        
        movLayerDraw(&ml0, &layer0);
        
       
    }
}


void readSwitches() {
    u_int switches = p2sw_read();
    u_int sw1 = switches & (1 << 0);
    u_int sw2 = switches & (1 << 1);
    u_int sw3 = switches & (1 << 2);
    u_int sw4 = switches & (1 << 3);
    
    // TODO state machine
    if (!sw2) {
    }
    if (!sw3) {
        //drawString5x7(screenWidth/2, screenHeight/2, "3", textColor, bgColor);
        Vec2 newPos;
        vec2Add(&newPos, &ml1.layer->posNext, &ml1.velocity);
        Region boundary;
        abShapeGetBounds(ml1.layer->abShape, &newPos, &boundary);
        if (boundary.topLeft.axes[0] - 4 > wall.topLeft.axes[0])
            newPos.axes[0] += -4;
        ml1.layer->posNext = newPos;
    }
    if (!sw4) {
        //drawString5x7(screenWidth/2, screenHeight/2, "4", textColor, bgColor);
        Vec2 newPos;
        vec2Add(&newPos, &ml1.layer->posNext, &ml1.velocity);
        Region boundary;
        abShapeGetBounds(ml1.layer->abShape, &newPos, &boundary);
        if (boundary.botRight.axes[0] + 4 < wall.botRight.axes[0])
            newPos.axes[0] += 4;
        ml1.layer->posNext = newPos;
    }
}

void moveOpponent(MovLayer *ml0, MovLayer *ml2) {
    Vec2 newPos0;
    vec2Add(&newPos0, &ml0->layer->posNext, &ml0->velocity);
    Region ballBound;
    abShapeGetBounds(ml0->layer->abShape, &newPos0, &ballBound);
    
    Vec2 newPos2;
    vec2Add(&newPos2, &ml2->layer->posNext, &ml2->velocity);
    Region padBound;
    abShapeGetBounds(ml2->layer->abShape, &newPos2, &padBound);
    
    if (ballBound.topLeft.axes[0] < padBound.topLeft.axes[0] && padBound.topLeft.axes[0] - 4 > wall.topLeft.axes[0])
        newPos2.axes[0] -= 4;
    if (ballBound.botRight.axes[0] > padBound.botRight.axes[0] && padBound.botRight.axes[0] + 4 < wall.botRight.axes[0])
        newPos2.axes[0] += 4;
    ml2->layer->posNext = newPos2;
    
}

// watchdog timer interrupt handler, 10 interrupts/sec
void wdt_c_handler()
{
    static short count = 0;
    P1OUT |= GREEN_LED;         // green LED on when CPU on
    count ++;
    if (count == 15) {
        redrawScreen = 1;
        count = 0;
        buzzerSetPeriod(N0);
    } 
    P1OUT &= ~GREEN_LED;        // green LED off when CPU off
}

