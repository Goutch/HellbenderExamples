#extension GL_EXT_debug_printf: enable
#define DEBUG_PRINT
#ifdef DEBUG_PRINT
#define IS_CENTER_PIXEL_CONDITION gl_LaunchIDEXT.x == (gl_LaunchSizeEXT.x) / 2 && gl_LaunchIDEXT.y == (gl_LaunchSizeEXT.y / 2)
#define PRINT debugPrintfEXT
#endif

