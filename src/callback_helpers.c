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

/* determine if object was already selected when button1 was pressed */
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

    /* In *NON* intuitive interface (or cadence compatibility) 
    * a button1 press with no modifiers will* first unselect everything.*/
void maybe_unsel_all_in_CDNS_compat(int cadence_compat, int state)
{
  if((cadence_compat || !xctx->intuitive_interface) && !(state & (ShiftMask | ControlMask)))
    unselect_all(1);
}

    /* In intuitive interface a button1 press with no modifiers will
    *  unselect everything... we do it here */
void maybe_unsel_all_in_intutive(int already_selected, int state)
{
  if(xctx->intuitive_interface && !already_selected && !(state & (ShiftMask | ControlMask)))
    unselect_all(1);
}

void handle_selection_logic(int already_selected, Selected *sel){
  if(!already_selected) 
    select_object(xctx->mousex, xctx->mousey, SELECTED, 0, sel);
  rebuild_selected_array();
  dbg(1, "Button1Press to select objects, lastsel = %d\n", xctx->lastsel); 
}

void handle_auto_highlighting(Selected sel, int state) {
    if (!tclgetboolvar("auto_hilight") || xctx->shape_point_selected)
        return;

    if (!(state & ShiftMask) && xctx->hilight_nets && sel.type == 0) {
        if (!xctx->lastsel) {
            redraw_hilights(1);
        }
    }
    hilight_net(0);
    if (xctx->lastsel) {
        redraw_hilights(0);
    }
}

/* sets xctx->shape_point_selected */
static int edit_line_point(int state)
{
   int line_n = -1, line_c = -1;
   dbg(1, "1 Line selected\n");
   line_n = xctx->sel_array[0].n;
   line_c = xctx->sel_array[0].col;
  /* lineangle point: Check is user is clicking a control point of a lineangle */
  if(line_n >= 0) {
    xLine *p = &xctx->line[line_c][line_n];

    xctx->need_reb_sel_arr=1;
    if(xctx->mousex_snap == p->x1 && xctx->mousey_snap == p->y1) {
      xctx->shape_point_selected = 1;
      p->sel = SELECTED1;
    }
    else if(xctx->mousex_snap == p->x2 && xctx->mousey_snap == p->y2) {
      xctx->shape_point_selected = 1;
      p->sel = SELECTED2;
    }
    if(xctx->shape_point_selected) {
      /* move one line selected point */
      if(!(state & (ControlMask | ShiftMask))){
        /* xctx->push_undo(); */
        move_objects(START,0,0,0);
        return 1;
      }
    } /* if(xctx->shape_point_selected) */
  } /* if(line_n >= 0) */
  return 0;
}

/* sets xctx->shape_point_selected */
static int edit_wire_point(int state)
{       
   int wire_n = -1;
   dbg(1, "edit_wire_point, ds = %g\n", xctx->cadhalfdotsize);
   wire_n = xctx->sel_array[0].n;
  /* wire point: Check is user is clicking a control point of a wire */
  if(wire_n >= 0) {
    xWire *p = &xctx->wire[wire_n];
        
    xctx->need_reb_sel_arr=1;
    if(xctx->mousex_snap == p->x1 && xctx->mousey_snap == p->y1) {
      xctx->shape_point_selected = 1;
      p->sel = SELECTED1;
    }
    else if(xctx->mousex_snap == p->x2 && xctx->mousey_snap == p->y2) {
      xctx->shape_point_selected = 1;
      p->sel = SELECTED2;
    }
    if(xctx->shape_point_selected) {
      /* move one wire selected point */
      if(!(state & (ControlMask | ShiftMask))){
        /* xctx->push_undo(); */
        move_objects(START,0,0,0);
        return 1;
      }
    } /* if(xctx->shape_point_selected) */
  } /* if(wire_n >= 0) */
  return 0;
}      

/* sets xctx->shape_point_selected */
static int edit_rect_point(int state)
{
   int rect_n = -1, rect_c = -1;
   dbg(1, "1 Rectangle selected\n");
   rect_n = xctx->sel_array[0].n;
   rect_c = xctx->sel_array[0].col;
  /* rectangle point: Check is user is clicking a control point of a rectangle */
  if(rect_n >= 0) {
    double ds = xctx->cadhalfdotsize * 2;
    xRect *p = &xctx->rect[rect_c][rect_n];

    xctx->need_reb_sel_arr=1;
    if(POINTINSIDE(xctx->mousex, xctx->mousey, p->x1, p->y1, p->x1 + ds, p->y1 + ds)) {
      xctx->shape_point_selected = 1;
      p->sel = SELECTED1;
    }
    else if(POINTINSIDE(xctx->mousex, xctx->mousey, p->x2 - ds, p->y1, p->x2, p->y1 + ds)) {
      xctx->shape_point_selected = 1;
      p->sel = SELECTED2;
    }
    else if(POINTINSIDE(xctx->mousex, xctx->mousey, p->x1, p->y2 - ds, p->x1 + ds, p->y2)) {
      xctx->shape_point_selected = 1;
      p->sel = SELECTED3;
    }
    else if(POINTINSIDE(xctx->mousex, xctx->mousey, p->x2 - ds, p->y2 - ds, p->x2, p->y2)) {
      xctx->shape_point_selected = 1;
      p->sel = SELECTED4;
    }
    if(xctx->shape_point_selected) { 
      /* move one rectangle selected point */
      if(!(state & (ControlMask | ShiftMask))){
        /* xctx->push_undo(); */
        move_objects(START,0,0,0);
        return 1;
      }
    } /* if(xctx->shape_point_selected) */
  } /* if(rect_n >= 0) */
  return 0;
}

/* sets xctx->shape_point_selected */
static int edit_polygon_point(int state)
{
   int poly_n = -1, poly_c = -1;
   dbg(1, "1 Polygon selected\n");
   poly_n = xctx->sel_array[0].n;
   poly_c = xctx->sel_array[0].col;
  /* polygon point: Check is user is clicking a control point of a polygon */
  if(poly_n >= 0) {
    int i;
    double ds = xctx->cadhalfdotsize;
    int c = poly_c;
    int n = poly_n;
    xPoly *p = &xctx->poly[c][n];

    xctx->need_reb_sel_arr=1;
    for(i = 0; i < p->points; i++) {
      if(
          POINTINSIDE(xctx->mousex, xctx->mousey, p->x[i] - ds, p->y[i] - ds, 
                        p->x[i] + ds, p->y[i] + ds)
        ) {
          dbg(1, "selecting point %d\n", i);
          p->selected_point[i] = 1;
          xctx->shape_point_selected = 1;
          break;
      }
    }
    if(xctx->shape_point_selected) { 
      int j;
      int points = p->points;

      /* add a new polygon/bezier point after selected one and start moving it*/
      if(state & ShiftMask) {
        xctx->push_undo();
        points++;
        my_realloc(_ALLOC_ID_, &p->x, sizeof(double) * points);
        my_realloc(_ALLOC_ID_, &p->y, sizeof(double) * points);
        my_realloc(_ALLOC_ID_, &p->selected_point, sizeof(unsigned short) * points);
        p->selected_point[i] = 0;
        for(j = points - 2; j > i; j--) {
          p->x[j + 1] = p->x[j];
          p->y[j + 1] = p->y[j];
          p->selected_point[j + 1] = p->selected_point[j];
        }
        p->selected_point[i + 1] = 1;
        p->x[i + 1] = p->x[i];
        p->y[i + 1] = p->y[i];
        p->points = points;
        p->sel = SELECTED1;
        move_objects(START,0,0,0);
        return 1;
      }
      /* delete polygon/bezier selected point */
      else if(points > 2 && state & ControlMask) {
        xctx->push_undo();
        points--;
        for(j = i ; j < points ; j++) {
           p->x[j] = p->x[j + 1];
           p->y[j] = p->y[j + 1];
           p->selected_point[j] = p->selected_point[j + 1];
        }
        my_realloc(_ALLOC_ID_, &p->x, sizeof(double) * points);
        my_realloc(_ALLOC_ID_, &p->y, sizeof(double) * points);
        my_realloc(_ALLOC_ID_, &p->selected_point, sizeof(unsigned short) * points);
        p->points = points;
        p->sel = SELECTED;
        return 1;
      /* move one polygon/bezier selected point */
      } else if(!(state & (ControlMask | ShiftMask))){
        /* xctx->push_undo(); */
        p->sel = SELECTED1;
        move_objects(START,0,0,0);
        return 1;
      }
    } /* if(xctx->shape_point_selected) */
  } /* if(poly_n >= 0) */
  return 0;
}


bool try_edit_shape_point(int state, int already_selected) {
    if (xctx->lastsel != 1) return false;
    Selected *sel_array = &xctx->sel_array[0];
  
    if (sel_array->type == POLYGON && edit_polygon_point(state)) return true;
  
    if (xctx->intuitive_interface && already_selected) {
      if ((sel_array->type == xRECT && edit_rect_point(state)) ||
          (sel_array->type == LINE && edit_line_point(state)) ||
          (sel_array->type == WIRE && edit_wire_point(state))) {
        return true;
      }
    }
    return false;
  }