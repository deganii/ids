#include "ui.h"
#include "shapes.h"

void build_ROI_selector(VGfloat x, VGfloat y, VGfloat w, VGfloat h){

    Fill(255, 255, 255, 0.3);
    Rect(x,y,w,h);

    /*VGPath vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_APPEND_TO);
	vguRect(path, x, y, w, h);
	vgDrawPath(path, VG_FILL_PATH | VG_STROKE_PATH);
	vgDestroyPath(path);*/
}