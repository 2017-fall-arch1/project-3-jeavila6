#include <libTimer.h>
#include "lcdutils.h"
#include "lcddraw.h"
#include "shape.h"

const AbRect rect10 = {abRectGetBounds, abRectCheck, 10, 10};;

// (pointer to abstract shape, pointer to center (a vector), fg color, bg color)
void abDrawPos(AbShape *shape, Vec2 *shapeCenter, u_int fg_color, u_int bg_color)
{
  u_char row, col;
  Region bounds;   // specifies a rectangular region given topLeft and botRight
  
  // computes bounding box of abShape in screen coordinates (in:shape, in:centerPos, out:bounds)
  abShapeGetBounds(shape, shapeCenter, &bounds);
  
  // set area to draw to (start col, start row, end col, end row)
  lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1],
	      bounds.botRight.axes[0]-1, bounds.botRight.axes[1]-1);
  
  // unsure what anything else does
  for (row = bounds.topLeft.axes[1]; row < bounds.botRight.axes[1]; row++) {
    for (col = bounds.topLeft.axes[0]; col < bounds.botRight.axes[0]; col++) {
      Vec2 pixelPos = {col, row};
      int color = abShapeCheck(shape, shapeCenter, &pixelPos) ?
	fg_color : bg_color;
      lcd_writeColor(color);
    }
  }
}



int main()
{
  configureClocks();
  lcd_init();
  shapeInit();
  Vec2 rectPos = screenCenter; // the position of the rectangle is the center of the screen ?

  clearScreen(COLOR_BLUE);
  drawString5x7(20,20, "hello", COLOR_GREEN, COLOR_RED);
  //shapeInit();
  
  abDrawPos((AbShape*)&rect10, &rectPos, COLOR_ORANGE, COLOR_BLUE);

}


