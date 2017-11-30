#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>

#define GREEN_LED BIT6

AbRect rect10 = {abRectGetBounds, abRectCheck, {10, 1}};    // (getBounds, check, halfSize (vector))

//Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; // (topLeft, botRight) Not used?

// playing field (getBounds, check, ???)

u_int bgColor = COLOR_BLACK, textColor = COLOR_WHITE;

AbRectOutline fieldOutline = {
    abRectOutlineGetBounds, abRectOutlineCheck,   
    {screenWidth/2 - 1, screenHeight/2 - 10}
};

Layer fieldLayer = {                    // playing field as layer
    (AbShape *) &fieldOutline,
    {screenWidth/2, screenHeight/2},    // center
    {0,0}, {0,0},                       // last and next
    COLOR_WHITE,
    0,
};

Layer layer1 = {
    (AbShape *) &rect10,
    {screenWidth/2, screenHeight/2},    // center
    {0,0}, {0,0},
    COLOR_GREEN,
    &fieldLayer,
};

Layer layer0 = {                                // layer with white circle
    (AbShape *) &circle2,
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
//MovLayer ml2 = { &layer2, {1,1}, 0 }; /**< not all layers move */
//MovLayer ml1 = { &layer1, {1,2}, &ml2 };
MovLayer ml1 = { &layer1, {1,2}, 0 };    // paddle
MovLayer ml0 = { &layer0, {2,1}, &ml1 }; // ball

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

/* 
 * advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */

static char bounces = 0;

void mlAdvance(MovLayer *ml, Region *fence)
{
    Vec2 newPos;
    u_char axis;
    Region shapeBoundary;
    
    for (; ml; ml = ml->next) { // for ml
        vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
        abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
        for (axis = 0; axis < 2; axis ++) { // for axis
            if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) || (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) { // if outside fence
                int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
                newPos.axes[axis] += (2*velocity);
                // TODO seperate points for player and PC
                if ((shapeBoundary.topLeft.axes[1] < fence->topLeft.axes[1]) || (shapeBoundary.botRight.axes[1] > fence->botRight.axes[1]) ) { // if outside top or bottom
                    char b[12];
                    sprintf(b, "%d", ++bounces);
                    drawString5x7(48, 1, b, textColor, bgColor);
                }
            }
        }
        ml->layer->posNext = newPos;
    }
}



int redrawScreen = 1; // boolean, whether screen needs to be redrawn
Region fieldFence;    // fence around playing field

int main()
{
    P1DIR |= GREEN_LED; // green LED will be on when CPU is on
    P1OUT |= GREEN_LED;
    
    configureClocks();
    lcd_init();        // initialize the lcd
    //p2sw_init(15); // initialize switches, read all four switches
    p2sw_init(1); // motion demo uses 1
    
    layerInit(&layer0); // sets bounds into a consistent state
    layerDraw(&layer0); // renders all layers, pixels not contained in a layer are
    
    //u_char width = screenWidth, height = screenHeight; // height is 160, width is 128
    //clearScreen(COLOR_GREEN);
    //drawChar5x7(10, 10, 'a', COLOR_BLACK, COLOR_GREEN);
    //fillRectangle(30, 30, 5, 5, COLOR_BLUE); 
    //drawRectOutline(30, 30, 20, 20, COLOR_RED);
    //drawPixel(20, 20, COLOR_RED);
    
    layerGetBounds(&fieldLayer, &fieldFence);
    
    enableWDTInterrupts();      /**< enable periodic interrupt */
    or_sr(0x8); // GIE (enable interrupts)
    
    //u_char width = screenWidth, height = screenHeight;
        
    drawString5x7(1, screenHeight - 8, "Version 3",  textColor, bgColor); // (column, row, string, fg color, bg color)
    drawString5x7(82, 1, "Score", textColor, bgColor);
    drawString5x7(1, 1, "Points", textColor, bgColor);
    
    for(;;) { 
        while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
            P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
            or_sr(0x10);	      /**< CPU OFF */
        }
        P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
        redrawScreen = 0;
        movLayerDraw(&ml0, &layer0);
        //drawString5x7(117, 1, "99", textColor, bgColor); // TODO placeholder for points
    }
    
    //     drawString5x7(10, 10, "switches:", textColor, bgColor);
    //     while (1) { // TODO exit loop
    //         u_int switches = p2sw_read();
    //         char str[5];
    //         for (u_int i = 0; i < 4; i++)
    //             str[i] = (switches & (1 << i)) ? '-' : '0' + i;
    //         str[4] = 0;
    //         drawString5x7(10, 20, str, textColor, bgColor);
    //     }
    // refer to switchdemo.c for led+switch
}

// watchdog timer interrupt handler, 10 interrupts/sec
void wdt_c_handler()
{
    static short count = 0;
    P1OUT |= GREEN_LED;         // green LED on when CPU on
    count ++;
    if (count == 15) {
        mlAdvance(&ml0, &fieldFence);
        u_int switches = p2sw_read();
        if (switches)
            redrawScreen = 1;
        count = 0;
    } 
    P1OUT &= ~GREEN_LED;        // green LED off when CPU off
}
