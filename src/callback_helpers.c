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

int add_wire_from_inst(Selected *sel, double mx, double my)
{
  int res = 0;
  int prev_state = xctx->ui_state;
  int i, type = sel->type;
  double pinx0, piny0;
  if(type == ELEMENT) {
    int n = sel->n;
    xSymbol *symbol = xctx->sym + xctx->inst[n].ptr;
    int npin = symbol->rects[PINLAYER];
    for(i = 0; i < npin; ++i) {
      get_inst_pin_coord(n, i, &pinx0, &piny0);
      if(pinx0 == mx && piny0 == my) {
        break;
      }
    }
    if(i < npin) {
      dbg(1, "pin: %g %g\n", pinx0, piny0);
      unselect_all(1);
      start_wire(xctx->mousex_snap, xctx->mousey_snap); 
      if(prev_state == STARTWIRE) {
        tcleval("set constr_mv 0" );
        xctx->constr_mv=0;
      }
      res = 1;
    }
  }
  return res;
}

int add_wire_from_wire(Selected *sel, double mx, double my)
{
  int res = 0;
  int prev_state = xctx->ui_state;
  int type = sel->type;
  if(type == WIRE) {
    int n = sel->n;
    double x1 = xctx->wire[n].x1;
    double y1 = xctx->wire[n].y1;
    double x2 = xctx->wire[n].x2;
    double y2 = xctx->wire[n].y2;
    if( (mx == x1 && my == y1) || (mx == x2 && my == y2) ) {
      unselect_all(1); 
      start_wire(xctx->mousex_snap, xctx->mousey_snap); 
      if(prev_state == STARTWIRE) {
        tcleval("set constr_mv 0" );
        xctx->constr_mv=0;
      }
      res = 1;
    }
  }
  return res;
}

    /* Clicking and drag on an instance pin -> drag a new wire .. or */
    /* Clicking and drag on a wire end -> drag a new wire */
bool handle_wire_drawing_if_needed(Selected sel, int already_selected) {
    if (xctx->intuitive_interface && !already_selected) {
      if (add_wire_from_inst(&sel, xctx->mousex_snap, xctx->mousey_snap) ||
          add_wire_from_wire(&sel, xctx->mousex_snap, xctx->mousey_snap)) {
        return true;
      }
    }
    return false;
}