#include <libretro.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include "font.h"

// Framebuffer dimensions
#define WIDTH 320
#define HEIGHT 240

// Global variables
static retro_environment_t environ_cb;
static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static uint16_t framebuffer[WIDTH * HEIGHT]; // RGB565
static bool initialized = false;
static bool contentless_set = false;
static int env_call_count = 0;
static FILE *log_file = NULL;
static int square_x = 0;
static int square_y = 0;

// Colors (RGB565)
#define COLOR_WHITE 0xFFFF // White
#define COLOR_RED   0xF800 // Red

// File-based logging (simple)
static void fallback_log(const char *level, const char *msg) {
   if (!log_file) {
      log_file = fopen("core.log", "w");
      if (!log_file) {
         fprintf(stderr, "[ERROR] Failed to open core.log\n");
         return;
      }
   }
   fprintf(log_file, "[%s] %s\n", level, msg);
   fflush(log_file);
   fprintf(stderr, "[%s] %s\n", level, msg);
}

// File-based logging (formatted)
static void fallback_log_format(const char *level, const char *fmt, ...) {
   if (!log_file) {
      log_file = fopen("core.log", "w");
      if (!log_file) {
         fprintf(stderr, "[ERROR] Failed to open core.log\n");
         return;
      }
   }
   va_list args;
   va_start(args, fmt);
   fprintf(log_file, "[%s] ", level);
   vfprintf(log_file, fmt, args);
   fprintf(log_file, "\n");
   fflush(log_file);
   va_end(args);
   va_start(args, fmt);
   fprintf(stderr, "[%s] ", level);
   vfprintf(stderr, fmt, args);
   fprintf(stderr, "\n");
   va_end(args);
}

// Clear framebuffer to black
static void clear_framebuffer() {
  //  if (log_cb)
  //     log_cb(RETRO_LOG_INFO, "[DEBUG] Clearing framebuffer\n");
  //  else
  //     fallback_log("DEBUG", "Clearing framebuffer\n");
  memset(framebuffer, 0, WIDTH * HEIGHT * sizeof(uint16_t));
}

// Draw a single 8x8 character at (x, y) in RGB565 color
static void draw_char(int x, int y, char c, uint16_t color) {
   if (c < 32 || c > 126) {
      if (log_cb)
         log_cb(RETRO_LOG_WARN, "[DEBUG] Invalid character: %c\n", c);
      else
         fallback_log_format("WARN", "Invalid character: %c\n", c);
      return;
   }
   const uint8_t *glyph = font_8x8[c - 32];
   for (int gy = 0; gy < 8; gy++) {
      for (int gx = 0; gx < 8; gx++) {
         if (glyph[gy] & (1 << (7 - gx))) {
            int px = x + gx;
            int py = y + gy;
            if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT) {
               framebuffer[py * WIDTH + px] = color;
            }
         }
      }
   }
}

// Draw a string at (x, y) in RGB565 color
static void draw_string(int x, int y, const char *str, uint16_t color) {
  //  if (log_cb)
  //     log_cb(RETRO_LOG_INFO, "[DEBUG] Drawing string: %s at (%d, %d)", str, x, y);
  //  else
  //     fallback_log_format("DEBUG", "Drawing string: %s at (%d, %d)", str, x, y);
   int cx = x;
   for (size_t i = 0; str[i]; i++) {
      draw_char(cx, y, str[i], color);
      cx += 8;
   }
}

// Called by the frontend to set environment callbacks
void retro_set_environment(retro_environment_t cb) {
   environ_cb = cb;
   env_call_count++;
   if (!cb) {
      fallback_log("ERROR", "retro_set_environment: Null environment callback\n");
      return;
   }

   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] retro_set_environment called (count: %d)\n", env_call_count);
   else
      fallback_log_format("DEBUG", "retro_set_environment called (count: %d)\n", env_call_count);

   // Set content-less support
   if (!contentless_set) {
      bool contentless = true;
      if (environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &contentless)) {
         contentless_set = true;
         if (log_cb)
            log_cb(RETRO_LOG_INFO, "[DEBUG] Content-less support enabled\n");
         else
            fallback_log("DEBUG", "Content-less support enabled\n");
      } else {
         if (log_cb)
            log_cb(RETRO_LOG_ERROR, "[ERROR] Failed to set content-less support\n");
         else
            fallback_log("ERROR", "Failed to set content-less support\n");
      }
   }
}

// Called by the frontend to set video refresh callback
void retro_set_video_refresh(retro_video_refresh_t cb) {
   video_cb = cb;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Video refresh callback set\n");
   else
      fallback_log("DEBUG", "Video refresh callback set\n");
}

// Input callbacks
void retro_set_input_poll(retro_input_poll_t cb) {
   input_poll_cb = cb;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Input poll callback set\n");
   else
      fallback_log("DEBUG", "Input poll callback set\n");
}

void retro_set_input_state(retro_input_state_t cb) {
   input_state_cb = cb;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Input state callback set\n");
   else
      fallback_log("DEBUG", "Input state callback set\n");
}

// Stubbed callbacks
void retro_set_audio_sample(retro_audio_sample_t cb) { (void)cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { (void)cb; }

// Called when the core is initialized
void retro_init(void) {
   initialized = true;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Hello World core initialized\n");
   else
      fallback_log("DEBUG", "Hello World core initialized\n");
   clear_framebuffer();

   // Set pixel format to RGB565
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
   if (environ_cb && environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt)) {
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "[DEBUG] Pixel format set: RGB565\n");
      else
         fallback_log("DEBUG", "Pixel format set: RGB565\n");
   } else {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "[ERROR] Failed to set pixel format: RGB565\n");
      else
         fallback_log("ERROR", "Failed to set pixel format: RGB565\n");
      if (environ_cb)
         environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
   }

   // Set up logging
   struct retro_log_callback logging;
   if (environ_cb && environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging)) {
      log_cb = logging.log;
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "[DEBUG] Logging callback initialized\n");
   } else {
      if (log_cb)
         log_cb(RETRO_LOG_WARN, "[WARN] Failed to get log interface\n");
      else
         fallback_log("WARN", "Failed to get log interface\n");
   }
}

// Called when the core is deinitialized
void retro_deinit(void) {
   if (log_file) {
      fclose(log_file);
      log_file = NULL;
   }
   initialized = false;
   contentless_set = false;
   env_call_count = 0;
   square_x = 0;
   square_y = 0;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Core deinitialized\n");
   else
      fallback_log("DEBUG", "Core deinitialized\n");
}

// Called to get system information
void retro_get_system_info(struct retro_system_info *info) {
   memset(info, 0, sizeof(*info));
   info->library_name = "Libretro Core Hello World";
   info->library_version = "1.0";
   info->need_fullpath = false;
   info->block_extract = false;
   info->valid_extensions = "";
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] System info: %s v%s, need_fullpath=%d\n",
             info->library_name, info->library_version, info->need_fullpath);
   else
      fallback_log_format("DEBUG", "System info: %s v%s, need_fullpath=%d\n",
                         info->library_name, info->library_version, info->need_fullpath);
}

// Called to get system AV information
void retro_get_system_av_info(struct retro_system_av_info *info) {
   memset(info, 0, sizeof(*info));
   info->geometry.base_width = WIDTH;
   info->geometry.base_height = HEIGHT;
   info->geometry.max_width = WIDTH;
   info->geometry.max_height = HEIGHT;
   info->geometry.aspect_ratio = (float)WIDTH / HEIGHT;
   info->timing.fps = 60.0;
   info->timing.sample_rate = 48000.0;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] AV info: %dx%d, %.2f fps\n", WIDTH, HEIGHT, info->timing.fps);
   else
      fallback_log_format("DEBUG", "AV info: %dx%d, %.2f fps\n", WIDTH, HEIGHT, info->timing.fps);
}

// Called when the core is loaded
void retro_set_controller_port_device(unsigned port, unsigned device) {
   (void)port;
   (void)device;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Controller port device set: port=%u, device=%u\n", port, device);
   else
      fallback_log_format("DEBUG", "Controller port device set: port=%u, device=%u\n", port, device);
}

// Called to reset the core
void retro_reset(void) {
   clear_framebuffer();
   square_x = 0;
   square_y = 0;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Core reset\n");
   else
      fallback_log("DEBUG", "Core reset\n");
}

// Called every frame
void retro_run(void) {
   if (!initialized) {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "[ERROR] Core not initialized in retro_run\n");
      else
         fallback_log("ERROR", "Core not initialized in retro_run\n");
      return;
   }

   clear_framebuffer();

   // Handle input
   if (input_poll_cb)
      input_poll_cb();
   if (input_state_cb) {
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT)) {
         square_x += 1;
         if (square_x > WIDTH - 20) square_x = WIDTH - 20;
      }
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT)) {
         square_x -= 1;
         if (square_x < 0) square_x = 0;
      }
      // Enable for up/down movement
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN)) {
         square_y += 1;
         if (square_y > HEIGHT - 20) square_y = HEIGHT - 20;
      }
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP)) {
         square_y -= 1;
         if (square_y < 0) square_y = 0;
      }
   }

   // Draw a 20x20 red square at (square_x, square_y)
   for (int y = 0; y < 20; y++) {
      for (int x = 0; x < 20; x++) {
         if (square_x + x < WIDTH && square_y + y < HEIGHT)
            framebuffer[(y + square_y) * WIDTH + (x + square_x)] = COLOR_RED;
      }
   }
  //  if (log_cb)
  //     log_cb(RETRO_LOG_INFO, "[DEBUG] Drawing red square at (%d, %d)\n", square_x, square_y);
  //  else
  //     fallback_log_format("DEBUG", "Drawing red square at (%d, %d)\n", square_x, square_y);

   // Draw "Hello World" at (50, 50)
   draw_string(50, 50, "Hello World", COLOR_WHITE);

   if (video_cb) {
      video_cb(framebuffer, WIDTH, HEIGHT, WIDTH * sizeof(uint16_t));
      // if (log_cb)
      //    log_cb(RETRO_LOG_INFO, "[DEBUG] Framebuffer sent to video_cb\n");
      // else
      //    fallback_log("DEBUG", "Framebuffer sent to video_cb\n");
   } else {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "[ERROR] No video callback set\n");
      else
         fallback_log("ERROR", "No video callback set\n");
   }
}

// Called to load a game
bool retro_load_game(const struct retro_game_info *game) {
   (void)game;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Game loaded (content-less): Displaying Hello World\n");
   else
      fallback_log("DEBUG", "Game loaded (content-less): Displaying Hello World \n");
   clear_framebuffer();
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] retro_load_game completed\n");
   else
      fallback_log("DEBUG", "retro_load_game completed\n");
   return true;
}

// Called to load special content
bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info) {
   (void)game_type;
   (void)info;
   (void)num_info;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] retro_load_game_special called (stubbed)\n");
   else
      fallback_log("DEBUG", "retro_load_game_special called (stubbed)\n");
   return false; // Not supported
}

// Called to unload a game
void retro_unload_game(void) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Game unloaded\n");
   else
      fallback_log("DEBUG", "Game unloaded\n");
}

// Called to get region
unsigned retro_get_region(void) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Region: NTSC\n");
   else
      fallback_log("DEBUG", "Region: NTSC\n");
   return RETRO_REGION_NTSC;
}

// Stubbed serialization functions
bool retro_serialize(void *data, size_t size) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Serialize called (stubbed)\n");
   else
      fallback_log("DEBUG", "Serialize called (stubbed)\n");
   (void)data; (void)size; return false;
}
bool retro_unserialize(const void *data, size_t size) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Unserialize called (stubbed)\n");
   else
      fallback_log("DEBUG", "Unserialize called (stubbed)\n");
   (void)data; (void)size; return false;
}
size_t retro_serialize_size(void) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Serialize size: 0\n");
   else
      fallback_log("DEBUG", "Serialize size: 0\n");
   return 0;
}

// Stubbed cheat functions
void retro_cheat_reset(void) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Cheat reset (stubbed)\n");
   else
      fallback_log("DEBUG", "Cheat reset (stubbed)\n");
}
void retro_cheat_set(unsigned index, bool enabled, const char *code) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Cheat set: index=%u, enabled=%d, code=%s (stubbed)\n", index, enabled, code);
   else
      fallback_log_format("DEBUG", "Cheat set: index=%u, enabled=%d, code=%s (stubbed)\n", index, enabled, code);
   (void)index; (void)enabled; (void)code;
}

// Stubbed memory functions
void *retro_get_memory_data(unsigned id) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Memory data: id=%u (stubbed)\n", id);
   else
      fallback_log_format("DEBUG", "Memory data: id=%u (stubbed)\n", id);
   (void)id; return NULL;
}
size_t retro_get_memory_size(unsigned id) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Memory size: id=%u (stubbed)\n", id);
   else
      fallback_log_format("DEBUG", "Memory size: id=%u (stubbed)\n", id);
   (void)id; return 0;
}

// Called to get API version
unsigned retro_api_version(void) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] API version: %u\n", RETRO_API_VERSION);
   else
      fallback_log_format("DEBUG", "API version: %u\n", RETRO_API_VERSION);
   return RETRO_API_VERSION;
}