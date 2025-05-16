# Libretro Core Development Guide

This guide provides an overview of creating a libretro core, a library that interfaces with frontends like RetroArch to emulate games or run applications. It includes required functions, code examples, and references for further reading.

## Table of Contents
1. Introduction to Libretro (#introduction-to-libretro)
2. Core Types (#core-types)
3. Required Libretro Core Functions (#required-libretro-core-functions)
4. Sample Implementation (#sample-implementation)
5. Building and Testing (#building-and-testing)
6. References (#references)
7. Additional Resources (#additional-resources)

## Introduction to Libretro

Libretro is an API that allows developers to create cores (emulators or applications) that can be loaded by compatible frontends, such as RetroArch. A libretro core is a dynamic library (.so, .dll, or .dylib) that implements a set of required functions to handle initialization, input, audio, video, and content loading.

## Key Components
- Frontend (e.g., RetroArch): Queries the core for metadata and capabilities, loads the core, and manages user interaction.
- Core: Implements the emulation or application logic and communicates with the frontend via callbacks.

## Core Types

Libretro cores can be one of two types:

1. Contentless Core (Standalone): Runs without external content (e.g., a self-contained application or demo).
2. Content-Loading Core: Requires external content, such as a ROM or game file, to operate (e.g., an emulator).

The frontend uses the RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME environment call to determine if a core supports contentless operation.

## Required Libretro Core Functions

A libretro core must implement 25 specific functions to interface with the frontend. Missing or incomplete implementations will cause errors. Below is the list of required functions with brief descriptions:

|Function|Description|
|---|---|
|retro_set_environment|Sets the environment callback for frontend communication.|
|retro_set_video_refresh|Sets the callback for rendering video frames.|
|retro_set_input_poll|Sets the callback for polling input devices.|
|retro_set_input_state|Sets the callback for querying input state.|
|retro_set_audio_sample|Sets the callback for single audio samples (optional for batch audio).|
|retro_set_audio_sample_batch|Sets the callback for batch audio samples.|
|retro_init|Initializes the core.|
|retro_deinit|Deinitializes the core.|
|retro_get_system_info|Provides metadata about the core (e.g., name, version, supported extensions).|
|retro_get_system_av_info|Provides audio/video parameters (e.g., resolution, frame rate).|
|retro_set_controller_port_device|Configures input devices for a controller port.|
|retro_reset|Resets the core to its initial state.|
|retro_load_game|Loads a game or content file.|
|retro_run|Executes one frame of the core.|
|retro_load_game_special|Loads special content (e.g., multi-disc games).|
|retro_unload_game|Unloads the current game or content.|
|retro_get_region|Returns the core's region (NTSC or PAL).|
|retro_serialize|Serializes the core's state for save states.|
|retro_unserialize|Restores the core's state from a save state.|
|retro_serialize_size|Returns the size of the serialized state.|
|retro_cheat_reset|Resets all cheats.|
|retro_cheat_set|Applies a cheat code.|
|retro_get_memory_data|Returns a pointer to a memory region (e.g., RAM).|
|retro_get_memory_size|Returns the size of a memory region.|
|retro_api_version|Returns the libretro API version.|

## Sample Implementation

Below is a minimal libretro core implementation with comments explaining each function. This example assumes a content-loading core that renders a simple frame and logs basic information.

c

```c
#include "libretro.h"
#include <string.h>
#include <stdio.h>

// Constants
#define WIDTH 320
#define HEIGHT 240
#define RETRO_LOG_DEFAULT fprintf(stderr, "%s\n", msg)

// Global callbacks
static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

// Fallback logging function
static void fallback_log(const char *level, const char *msg) {
   RETRO_LOG_DEFAULT;
}

// Fallback logging function with format
static void fallback_log_format(const char *level, const char *fmt, unsigned value) {
   char msg[256];
   snprintf(msg, sizeof(msg), fmt, value);
   RETRO_LOG_DEFAULT;
}

// Set environment callback
void retro_set_environment(retro_environment_t cb) {
   environ_cb = cb;
   if (!cb) {
      fallback_log("ERROR", "retro_set_environment: Null environment callback");
      return;
   }

   // Indicate that this core requires content (disable contentless mode)
   bool contentless = false;
   environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &contentless);
}

// Set video refresh callback
void retro_set_video_refresh(retro_video_refresh_t cb) {
   video_cb = cb;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Video refresh callback set");
   else
      fallback_log("DEBUG", "Video refresh callback set");
}

// Set input callbacks
void retro_set_input_poll(retro_input_poll_t cb) {
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb) {
   input_state_cb = cb;
}

// Stub audio callbacks (implement if audio is needed)
void retro_set_audio_sample(retro_audio_sample_t cb) { (void)cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { (void)cb; }

// Initialize the core
void retro_init(void) {
   // Set pixel format to RGB565
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
   if (environ_cb && environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt)) {
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "[DEBUG] Pixel format set: RGB565");
      else
         fallback_log("DEBUG", "Pixel format set: RGB565");
   }

   // Initialize logging
   struct retro_log_callback logging;
   if (environ_cb && environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging)) {
      log_cb = logging.log;
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "[DEBUG] Logging callback initialized");
   }
}

// Deinitialize the core
void retro_deinit(void) {
   // Clean up resources here
}

// Provide system information
void retro_get_system_info(struct retro_system_info *info) {
   memset(info, 0, sizeof(*info));
   info->library_name = "Hello World Core";
   info->library_version = "1.0";
   info->need_fullpath = true; // Core requires full path to content
   info->block_extract = true; // Prevent extraction of archives
   info->valid_extensions = "zip"; // Supported file extensions
  //  info->valid_extensions = "zip|7z"; // more types extensions
}

// Provide audio/video information
void retro_get_system_av_info(struct retro_system_av_info *info) {
   memset(info, 0, sizeof(*info));
   info->geometry.base_width = WIDTH;
   info->geometry.base_height = HEIGHT;
   info->geometry.max_width = WIDTH;
   info->geometry.max_height = HEIGHT;
   info->geometry.aspect_ratio = (float)WIDTH / HEIGHT;
   info->timing.fps = 60.0;
   info->timing.sample_rate = 48000.0;
}

// Configure controller port
void retro_set_controller_port_device(unsigned port, unsigned device) {
   (void)port;
   (void)device;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Controller port %u set to device %u", port, device);
}

// Reset the core
void retro_reset(void) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Core reset");
}

// Load a game or content
bool retro_load_game(const struct retro_game_info *game) {
   if (!game || !game->path) {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "[ERROR] No game provided");
      return false;
   }
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Loading game: %s", game->path);
   // Implement game loading logic here
   return true;
}

// Run one frame
void retro_run(void) {
   // Poll inputs
   input_poll_cb();

   // Example: Check for button press (e.g., Joypad A)
   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A)) {
      if (log_cb)
         log_cb(RETRO_LOG_INFO, "[DEBUG] Joypad A pressed");
   }

   // Render a dummy frame
   static uint16_t frame[WIDTH * HEIGHT];
   memset(frame, 0xFF, sizeof(frame)); // White frame
   video_cb(frame, WIDTH, HEIGHT, WIDTH * sizeof(uint16_t));
}

// Load special content (e.g., multi-disc)
bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info) {
   (void)game_type;
   (void)info;
   (void)num_info;
   return false; // Not implemented
}

// Unload the game
void retro_unload_game(void) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Game unloaded");
}

// Get region
unsigned retro_get_region(void) {
   return RETRO_REGION_NTSC;
}

// Stub serialization functions
bool retro_serialize(void *data, size_t size) {
   (void)data;
   (void)size;
   return false;
}

bool retro_unserialize(const void *data, size_t size) {
   (void)data;
   (void)size;
   return false;
}

size_t retro_serialize_size(void) {
   return 0;
}

// Stub cheat functions
void retro_cheat_reset(void) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Cheats reset");
}

void retro_cheat_set(unsigned index, bool enabled, const char *code) {
   (void)index;
   (void)enabled;
   (void)code;
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Cheat set: index=%u, enabled=%d, code=%s", index, enabled, code);
}

// Stub memory functions
void *retro_get_memory_data(unsigned id) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Memory data: id=%u (stubbed)", id);
   return NULL;
}

size_t retro_get_memory_size(unsigned id) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] Memory size: id=%u (stubbed)", id);
   return 0;
}

// Get API version
unsigned retro_api_version(void) {
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "[DEBUG] API version: %u", RETRO_API_VERSION);
   return RETRO_API_VERSION;
}
```

## Key Improvements in the Sample

- Consistent Logging: Added fallback logging functions and consistent use of log_cb for debugging.
- Error Handling: Added checks for null pointers in retro_load_game and retro_set_environment.
- Simplified Stubs: Stubbed functions include logging for debugging purposes.
- Clear Constants: Defined WIDTH and HEIGHT at the top for easy modification.
- Basic Input Handling: Demonstrates polling and checking for a button press in retro_run.
- Dummy Frame: Renders a white frame as a placeholder for video output.

## Building and Testing

To build a libretro core:

1. Set Up Dependencies: Use the libretro-deps repository for common dependencies (e.g., headers).
2. Compile the Core: Link against the libretro API and compile as a shared library (.so, .dll, or .dylib).
3. Test with RetroArch:
    - Place the compiled core in RetroArch's cores directory.
    - Load the core via RetroArch's menu and test with a supported ROM or as a contentless core.
4. Debugging: Use RetroArch's logging (--verbose command-line flag) to view core logs.

For a skeleton project, refer to the skeletor repository or libretro-samples.

## References
- Official Documentation:
    - [Libretro Core Development](https://docs.libretro.com/development/cores/developing-cores/)
    - [Libretro API Doxygen](https://bot.libretro.com/doxygen/a04904.html)
- Example Projects:
    - [Skeletor](https://github.com/libretro/skeletor): Minimal libretro core template.
    - [GameboyCore Retro](https://github.com/nnarain/gameboycore-retro): Game Boy emulator core.
    - [Vectrexia Emulator](https://github.com/beardypig/vectrexia-emulator/): Vectrex emulator core.
    - [TIC-80 Libretro](https://github.com/nesbox/TIC-80/blob/main/src/system/libretro/tic80_libretro.c): TIC-80 core implementation.
    - [Libretro Samples](https://github.com/libretro/libretro-samples): Sample cores for learning.
        
- Tutorials:
    - [RetroReversing: Libretro Overview](https://www.retroreversing.com/libRetro)
    - [GameboyCore as a Libretro Core](https://nnarain.github.io/2017/07/13/GameboyCore-as-a-libretro-core!.html)
    - [Emulator Build-Along Part 1](https://web.archive.org/web/20190219134430/http://www.beardypig.com/2016/01/15/emulator-build-along-1/)
    - [Emulator Build-Along Part 2](https://web.archive.org/web/20190219134028/http://www.beardypig.com/2016/01/22/emulator-build-along-2/)
    - [Arduous Endeavor Part 1](https://www.jroeder.net/2021/08/10/arduous-endeavor-part-1/)

Additional Resources
- [Libretro Dependencies](https://github.com/libretro/libretro-deps/): Common dependencies for libretro cores.
- [RetroArch Documentation](https://docs.libretro.com/): Frontend usage and configuration.
- [Libretro Forum](https://forums.libretro.com/): Community support for core development.

---

## Improvements Made

1. Structure: Organized the document with a clear table of contents and sections for better navigation.
2. Clarity: Rewrote explanations to be concise and beginner-friendly, avoiding jargon where possible.
3. Code Enhancements:
    - Added consistent logging and error handling.
    - Included a basic input and video rendering example.
    - Fixed minor issues (e.g., missing includes, undefined RETRO_API_VERSION).
4. Comprehensive References: Consolidated and categorized all provided links for easy access.
5. Building Instructions: Added a section on compiling and testing the core.
6. Table for Functions: Included a table to summarize the 25 required functions.