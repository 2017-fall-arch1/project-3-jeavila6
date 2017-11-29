#include <libTimer.h>
#include "lcdutils.h"
#include "lcddraw.h"
#include "p2switches.h"
#include "shape.h"

AbRect rect10 = {abRectGetBounds, abRectCheck, 10, 10}; // (getBounds, check, halfSize (vector))
AbRArrow arrow30 = {abRArrowGetBounds, abRArrowCheck, 30}; // (getBounds, check, size)

Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; // (topLeft, botRight)

// (referece to abstract shape, layer's current position, layer's color, reference to lower layer)
Layer layer2 = {
    (AbShape *)&arrow30,
    {screenWidth/2+40, screenHeight/2+10},    /* position */
    {0,0}, {0,0},                             /* last & next pos */
    COLOR_BLUE,
    0,
};

Layer layer1 = {
    (AbShape *)&rect10,
    {screenWidth/2, screenHeight/2},          /* position */
    {0,0}, {0,0},                             /* last & next pos */
    COLOR_RED,
    &layer2,
};

Layer layer0 = {
    (AbShape *)&rect10,
    {(screenWidth/2)+10, (screenHeight/2)+5}, /* position */
    {0,0}, {0,0},                             /* last & next pos */
    COLOR_YELLOW,
    &layer1,
};

u_int bgColor = COLOR_BLACK; // background color used for layers
u_int textColor = COLOR_WHITE;

int main()
{
    configureClocks();
    lcd_init();        // initialize the lcd
    //u_char width = screenWidth, height = screenHeight; // height is 160, width is 128

    //clearScreen(COLOR_GREEN);
    //drawChar5x7(10, 10, 'a', COLOR_BLACK, COLOR_GREEN);
    //fillRectangle(30, 30, 5, 5, COLOR_BLUE); 
    //drawRectOutline(30, 30, 20, 20, COLOR_RED);
    //drawPixel(20, 20, COLOR_RED);
  
    p2sw_init(15); // initialize switches
    or_sr(0x8); // GIE (enable interrupts)
    u_char width = screenWidth, height = screenHeight;
    
    shapeInit();        // base class for Abstract Shapes
    layerInit(&layer0); // sets bounds into a consistent state
    layerDraw(&layer0); // renders all layers, pixels not contained in a layer are set to bgColor
    
    drawString5x7(1, 1, "REV 1", textColor, bgColor); // (column, row, string, fg color, bg color)
    drawString5x7(50, 1, "help", COLOR_RED, bgColor);
    
    drawString5x7(10, 10, "switches:", textColor, bgColor);
    while (1) { // TODO exit loop
        u_int switches = p2sw_read();
        char str[5];
        for (u_int i = 0; i < 4; i++)
            str[i] = (switches & (1 << i)) ? '-' : '0' + i;
        str[4] = 0;
        drawString5x7(10, 20, str, textColor, bgColor);
    }
    // refer to switchdemo.c for led+switch
}
