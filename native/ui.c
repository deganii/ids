#include "ui.h"
#include "shapes.h"

#define SENSOR_FULL_WIDTH  3872.
#define SENSOR_FULL_HEIGHT 2764.
#define CROP_WIDTH  640.
#define CROP_HEIGHT 480.



void build_ROI_selector(VGfloat x, VGfloat y, VGfloat w, VGfloat h, float off_x, float off_y){
    VGfloat aspect = w/h;
    VGfloat asp_scale = 1.0;
    if(aspect > 1.4) { // limiting factor is height
        asp_scale = h / SENSOR_FULL_HEIGHT;
    }  else {
        asp_scale = w / SENSOR_FULL_WIDTH;
    }

    Fill(0, 0, 255, 1.0);
    Rect(x,y,asp_scale * SENSOR_FULL_WIDTH, asp_scale * SENSOR_FULL_HEIGHT);

    Fill(255, 0, 0, 1.0);
    Rect(x+off_x*asp_scale,y+asp_scale*off_y,
         asp_scale * CROP_WIDTH,asp_scale * CROP_HEIGHT);
}