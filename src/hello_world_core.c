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
static uint16_t framebuffer[WIDTH * HEIGHT]; // Fixed RGB565
static bool initialized = false;
static bool pixel_format_set = false;
static bool contentless_set = false;
static int env_call_count = 0;
static FILE *log_file = NULL;

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
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Clearing framebuffer");
   else
      fallback_log("DEBUG", "Clearing framebuffer");
   memset(framebuffer, 0, WIDTH * HEIGHT * sizeof(uint16_t));
}

// Draw a single 8x8 character at (x, y) in RGB565 color
static void draw_char(int x, int y, char c, uint16_t color) {
   if (c < 32 || c > 126) {
      if (log_cb)
         log_cb(RETRO_LOG_WARN, "[DEBUG] Invalid character: %c", c);
      else
         fallback_log_format("WARN", "Invalid character: %c", c);
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
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Drawing string: %s at (%d, %d)", str, x, y);
   else
      fallback_log_format("DEBUG", "Drawing string: %s at (%d, %d)", str, x, y);
   int cx = x;
   for (size_t i = 0; str[i]; i++) {
      draw_char(cx, y, str[i], color);
      cx += 8;
   }
}

// Colors (RGB565)
#define COLOR_WHITE 0xFFFF // White
#define COLOR_RED   0xF800 // Red

// Called by the frontend to set environment callbacks
void retro_set_environment(retro_environment_t cb) {
   environ_cb = cb;
   env_call_count++;
   if (!cb) {
      fallback_log("ERROR", "retro_set_environment: Null environment callback");
      return;
   }

   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] retro_set_environment called (count: %d)", env_call_count);
   else
      fallback_log_format("DEBUG", "retro_set_environment called (count: %d)", env_call_count);

   // Set content-less support (only once)
   if (!contentless_set) {
      bool contentless = true;
      if (environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &contentless)) {
         contentless_set = true;
         if (log_cb)
            log_cb(RETRO_LOG_INFO, "[DEBUG] Content-less support enabled");
         else
            fallback_log("DEBUG", "Content-less support enabled");
      } else {
         if (log_cb)
            log_cb(RETRO_LOG_ERROR, "[ERROR] Failed to set content-less support");
         else
            fallback_log("ERROR", "Failed to set content-less support");
      }
   }

   // Set up logging (only if not already set)
   if (!log_cb) {
      struct retro_log_callback logging;
      if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging)) {
         log_cb = logging.log;
         if (log_cb)
            log_cb(RETRO_LOG_INFO, "[DEBUG] Logging callback initialized");
      } else {
         log_cb = NULL;
         fallback_log("WARN", "Failed to get log interface");
      }
   }

   // Set pixel format to RGB565 (only once)
   if (!pixel_format_set) {
      enum retro_pixel_format format = RETRO_PIXEL_FORMAT_RGB565;
      if (environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &format)) {
         pixel_format_set = true;
         if (log_cb)
            log_cb(RETRO_LOG_INFO, "[DEBUG] Pixel format set: RGB565");
         else
            fallback_log("DEBUG", "Pixel format set: RGB565");
      } else {
         if (log_cb)
            log_cb(RETRO_LOG_ERROR, "[ERROR] Failed to set pixel format: RGB565");
         else
            fallback_log("ERROR", "Failed to set pixel format: RGB565");
      }
   }
}

// Called by the frontend to set video refresh callback
void retro_set_video_refresh(retro_video_refresh_t cb) {
   video_cb = cb;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Video refresh callback set");
   else
      fallback_log("DEBUG", "Video refresh callback set");
}

// Stubbed callbacks
void retro_set_input_poll(retro_input_poll_t cb) { (void)cb; }
void retro_set_input_state(retro_input_state_t cb) { (void)cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { (void)cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { (void)cb; }

// Called when the core is initialized
void retro_init(void) {
   initialized = true;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Hello World core initialized");
   else
      fallback_log("DEBUG", "Hello World core initialized");
   clear_framebuffer();
}

// Called when the core is deinitialized
void retro_deinit(void) {
   if (log_file) {
      fclose(log_file);
      log_file = NULL;
   }
   initialized = false;
   pixel_format_set = false;
   contentless_set = false;
   env_call_count = 0;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Core deinitialized");
   else
      fallback_log("DEBUG", "Core deinitialized");
}

// Called to get system information
void retro_get_system_info(struct retro_system_info *info) {
   memset(info, 0, sizeof(*info));
   info->library_name = "Hello World Core";
   info->library_version = "1.0";
   info->need_fullpath = false;
   info->block_extract = false;
   info->valid_extensions = "";
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] System info: %s v%s, need_fullpath=%d",
             info->library_name, info->library_version, info->need_fullpath);
   else
      fallback_log_format("DEBUG", "System info: %s v%s, need_fullpath=%d",
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
      log_cb(RETRO_LOG_INFO, "[DEBUG] AV info: %dx%d, %.2f fps", WIDTH, HEIGHT, info->timing.fps);
   else
      fallback_log_format("DEBUG", "AV info: %dx%d, %.2f fps", WIDTH, HEIGHT, info->timing.fps);
}

// Called when the core is loaded
void retro_set_controller_port_device(unsigned port, unsigned device) {
   (void)port;
   (void)device;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Controller port device set: port=%u, device=%u", port, device);
   else
      fallback_log_format("DEBUG", "Controller port device set: port=%u, device=%u", port, device);
}

// Called to reset the core
void retro_reset(void) {
   clear_framebuffer();
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Core reset");
   else
      fallback_log("DEBUG", "Core reset");
}

// Called every frame
void retro_run(void) {
   if (!initialized) {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "[ERROR] Core not initialized in retro_run");
      else
         fallback_log("ERROR", "Core not initialized in retro_run");
      return;
   }
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Running frame");
   else
      fallback_log("DEBUG", "Running frame");
   clear_framebuffer();

   // Test: Draw a 20x20 red square at (0, 0)
   for (int y = 0; y < 20; y++) {
      for (int x = 0; x < 20; x++) {
         framebuffer[y * WIDTH + x] = COLOR_RED;
      }
   }

   // Draw "Hello World" at (50, 50)
   draw_string(50, 50, "Hello World", COLOR_WHITE);

   if (video_cb) {
      video_cb(framebuffer, WIDTH, HEIGHT, WIDTH * sizeof(uint16_t));
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "[DEBUG] Framebuffer sent to video_cb");
      else
         fallback_log("DEBUG", "Framebuffer sent to video_cb");
   } else {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "[ERROR] No video callback set");
      else
         fallback_log("ERROR", "No video callback set");
   }
}

// Called to load a game
bool retro_load_game(const struct retro_game_info *game) {
   (void)game;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Game loaded (content-less): Displaying Hello World");
   else
      fallback_log("DEBUG", "Game loaded (content-less): Displaying Hello World");
   clear_framebuffer();
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] retro_load_game completed");
   else
      fallback_log("DEBUG", "retro_load_game completed");
   return true;
}

// Called to load special content
bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info) {
   (void)game_type;
   (void)info;
   (void)num_info;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] retro_load_game_special called (stubbed)");
   else
      fallback_log("DEBUG", "retro_load_game_special called (stubbed)");
   return false; // Not supported
}

// Called to unload a game
void retro_unload_game(void) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Game unloaded");
   else
      fallback_log("DEBUG", "Game unloaded");
}

// Called to get region
unsigned retro_get_region(void) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Region: NTSC");
   else
      fallback_log("DEBUG", "Region: NTSC");
   return RETRO_REGION_NTSC;
}

// Stubbed serialization functions
bool retro_serialize(void *data, size_t size) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Serialize called (stubbed)");
   else
      fallback_log("DEBUG", "Serialize called (stubbed)");
   (void)data; (void)size; return false;
}
bool retro_unserialize(const void *data, size_t size) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Unserialize called (stubbed)");
   else
      fallback_log("DEBUG", "Unserialize called (stubbed)");
   (void)data; (void)size; return false;
}
size_t retro_serialize_size(void) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Serialize size: 0");
   else
      fallback_log("DEBUG", "Serialize size: 0");
   return 0;
}

// Stubbed cheat functions
void retro_cheat_reset(void) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Cheat reset (stubbed)");
   else
      fallback_log("DEBUG", "Cheat reset (stubbed)");
}
void retro_cheat_set(unsigned index, bool enabled, const char *code) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Cheat set: index=%u, enabled=%d, code=%s (stubbed)", index, enabled, code);
   else
      fallback_log_format("DEBUG", "Cheat set: index=%u, enabled=%d, code=%s (stubbed)", index, enabled, code);
   (void)index; (void)enabled; (void)code;
}

// Stubbed memory functions
void *retro_get_memory_data(unsigned id) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Memory data: id=%u (stubbed)", id);
   else
      fallback_log_format("DEBUG", "Memory data: id=%u (stubbed)", id);
   (void)id; return NULL;
}
size_t retro_get_memory_size(unsigned id) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Memory size: id=%u (stubbed)", id);
   else
      fallback_log_format("DEBUG", "Memory size: id=%u (stubbed)", id);
   (void)id; return 0;
}

// Called to get API version
unsigned retro_api_version(void) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] API version: %u", RETRO_API_VERSION);
   else
      fallback_log_format("DEBUG", "API version: %u", RETRO_API_VERSION);
   return RETRO_API_VERSION;
}