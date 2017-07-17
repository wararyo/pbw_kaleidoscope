#include <pebble.h>

static Window *s_main_window;

static Layer *s_bb_info_layer; //BottomBar
static TextLayer *s_bb_date_layer;
static Layer *s_bb_layer;
static Layer *s_hands_layer;
static Layer *s_bg_layer;

static uint16_t HAND_LONG_LENGTH;
static uint16_t HAND_LONG_WIDTH;
static GColor   HAND_LONG_COLOR;
static GColor   HAND_LONG_FILL_COLOR;
static uint16_t HAND_SHORT_LENGTH;
static uint16_t HAND_SHORT_WIDTH;
static GColor   HAND_SHORT_COLOR;
static GColor   HAND_SHORT_FILL_COLOR;
static GColor   HAND_FILL_OVERLAP_COLOR;

uint16_t BOTTOMBAR_HEIGHT = 30;

static GPoint calculate_hands_end(uint32_t angle, uint16_t length, GPoint center);
static GPathInfo generate_fan_path();
static void calculate_fan_path(GPathInfo *pi,uint32_t start_angle, uint32_t angle, GPoint center);
static int32_t angle_delta(uint32_t a, uint32_t b);

//** BEHAVIOUR **//

static void bg_update_proc(Layer *layer, GContext *context){
  
}

static void hands_update_proc(Layer *layer, GContext *context){
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  
  time_t temp = time(NULL);
  struct tm *t = localtime(&temp);
  
  uint32_t short_angle = (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6);
  uint32_t long_angle =  TRIG_MAX_ANGLE * t->tm_min / 60;
  
  GPathInfo path_info = generate_fan_path();
  GPath *path;
  
  // Short Fan
  graphics_context_set_stroke_color(context, GColorClear);
  graphics_context_set_fill_color(context, HAND_SHORT_FILL_COLOR);
  calculate_fan_path(&path_info, short_angle,TRIG_MAX_RATIO / 2, center);
  path = gpath_create(&path_info);
  gpath_draw_filled(context, path);
  gpath_destroy(path);
  
  // Long Fan
  graphics_context_set_fill_color(context, HAND_LONG_FILL_COLOR);
  calculate_fan_path(&path_info, long_angle,TRIG_MAX_RATIO / 2, center);
  path = gpath_create(&path_info);
  gpath_draw_filled(context, path);
  gpath_destroy(path);
  
  // Overlap Fan
  graphics_context_set_fill_color(context, HAND_FILL_OVERLAP_COLOR);
  
  int32_t delta_angle = angle_delta(short_angle,long_angle);
  int32_t half = TRIG_MAX_RATIO / 2;

  if(-half < delta_angle && delta_angle < 0){
      APP_LOG(APP_LOG_LEVEL_DEBUG, "half+delta-angle = %ld", (int)half+delta_angle);
      calculate_fan_path(&path_info, long_angle,delta_angle+half, center);
  }
  else{
      APP_LOG(APP_LOG_LEVEL_DEBUG, "half-delta-angle = %ld", (int)half-delta_angle);
      calculate_fan_path(&path_info, short_angle,half-delta_angle, center);
  }
    
  path = gpath_create(&path_info);
  gpath_draw_filled(context, path);
  gpath_destroy(path);
  
  // Short Hand
  graphics_context_set_stroke_color(context, HAND_SHORT_COLOR);
  graphics_context_set_stroke_width(context, HAND_SHORT_WIDTH);
  graphics_draw_line(context, center, calculate_hands_end(short_angle,HAND_SHORT_LENGTH,center));
  
  // Long Hand
  graphics_context_set_stroke_color(context, HAND_LONG_COLOR);
  graphics_context_set_stroke_width(context, HAND_LONG_WIDTH);
  graphics_draw_line(context, center, calculate_hands_end(long_angle,HAND_LONG_LENGTH,center));
  
  // Dot
  graphics_context_set_stroke_width(context, 0);
  graphics_context_set_fill_color(context, GColorWhite);
  graphics_fill_circle(context, GPoint(bounds.size.w / 2, bounds.size.h / 2), 1);
}

static void bottombar_update_proc(Layer *layer, GContext *context){
  // BG
  graphics_context_set_fill_color(context, GColorLightGray);
  graphics_fill_rect(context, layer_get_bounds(layer), 0, GCornerNone);
  
  // Date
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), "%b %d", tick_time);
  text_layer_set_text(s_bb_date_layer, s_buffer);
  
  // Battery
}

//** UTILITIES **//

//const GRect OUTSIDE = {GPoint{-82, -194},GRect{ 164, 188}};//中心が原点

//Angleは0x10000で一周
static GPoint calculate_hands_end(uint32_t angle, uint16_t length, GPoint center){
  GPoint result = {
    .x = (int16_t)(sin_lookup(angle) * (int32_t)length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(angle) * (int32_t)length / TRIG_MAX_RATIO) + center.y,
  };
  return result;
}

static GPathInfo generate_fan_path(){
  GPathInfo result = {
    5, (GPoint [5]) {
      { 0, 0 }
    }
  };
  return result;
}

static void calculate_fan_path(GPathInfo *pi,uint32_t start_angle, uint32_t angle, GPoint center){
  for(int i = 0; i < 4; i++){
    pi->points[i] = calculate_hands_end((angle / 3) * i + start_angle,128,center);
  }
  pi->points[4] = center;
}

static int32_t angle_delta(uint32_t a, uint32_t b){
  int32_t delta = a-b;
  int32_t half = TRIG_MAX_RATIO / 2;
  while(delta > half) delta -= TRIG_MAX_RATIO;
  while(delta < -half) delta += TRIG_MAX_RATIO;
  return delta;
}

//** FOUNDATION **//

static void update_time() {
  layer_mark_dirty(s_bb_layer);
  layer_mark_dirty(s_hands_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // BG Layer
  
  // Hands Layer
  
  s_hands_layer = layer_create(GRect(0,0,bounds.size.w,bounds.size.h - BOTTOMBAR_HEIGHT));
  layer_set_update_proc(s_hands_layer,hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
  
  // BottomBar Layer
  s_bb_layer = layer_create(GRect(0, bounds.size.h - BOTTOMBAR_HEIGHT, bounds.size.w, BOTTOMBAR_HEIGHT));
  layer_set_update_proc(s_bb_layer,bottombar_update_proc);
  layer_add_child(window_layer, s_bb_layer);

  // Date Layer
  s_bb_date_layer = text_layer_create(
      GRect(8, 3, 48, BOTTOMBAR_HEIGHT));
  
  text_layer_set_background_color(s_bb_date_layer, GColorClear);
  text_layer_set_text_color(s_bb_date_layer, GColorBlack);
  text_layer_set_text(s_bb_date_layer, "00:00");
  text_layer_set_font(s_bb_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_bb_date_layer, GTextAlignmentLeft);

  layer_add_child(s_bb_layer, text_layer_get_layer(s_bb_date_layer));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_bb_date_layer);
  layer_destroy(s_bb_layer);
  layer_destroy(s_hands_layer);
}

void init(void) {
  // Load Setting
  HAND_LONG_LENGTH = 80;
  HAND_LONG_WIDTH = 4;
  HAND_LONG_COLOR = GColorBlack;
  HAND_LONG_FILL_COLOR = GColorKellyGreen;
  HAND_SHORT_LENGTH = 48;
  HAND_SHORT_WIDTH = 8;
  HAND_SHORT_COLOR = GColorBlack;
  HAND_SHORT_FILL_COLOR = GColorOrange;
  HAND_FILL_OVERLAP_COLOR = GColorChromeYellow;
  
  // Window Initialization
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
  
  // EventListener
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  update_time();
}

void deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
