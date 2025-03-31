#include "xschem.h"

void save_mouse_pt(int x, int y)
{
  xctx->mx_save = x;
  xctx->my_save = y;
}

void save_elab_mouse_pt(int x, int y)
{
  xctx->shape_point_selected = 0;
  save_mouse_pt(x, y);
  xctx->mx_double_save=xctx->mousex;
  xctx->my_double_save=xctx->mousey;
}

int chk_if_already_selected(Selected sel){
    switch(sel.type) {
        case WIRE:    if(xctx->wire[sel.n].sel)          return 1;
        case xTEXT:   if(xctx->text[sel.n].sel)          return 1;
        case LINE:    if(xctx->line[sel.col][sel.n].sel) return 1;
        case POLYGON: if(xctx->poly[sel.col][sel.n].sel) return 1;
        case xRECT:   if(xctx->rect[sel.col][sel.n].sel) return 1;
        case ARC:     if(xctx->arc[sel.col][sel.n].sel)  return 1;
        case ELEMENT: if(xctx->inst[sel.n].sel)          return 1;
        default: return 0;
    }
}

/* if full crosshair, mouse ptr is obscured and crosshair is snapped to grid points */
Selected get_obj_under_cursor(int draw_xhair, int use_cursor_for_sel, int crosshair_size){
  if(draw_xhair && (use_cursor_for_sel || crosshair_size == 0)) {
    return find_closest_obj(xctx->mousex_snap, xctx->mousey_snap, 0);
    } else {
    return find_closest_obj(xctx->mousex, xctx->mousey, 0);
    }
}
