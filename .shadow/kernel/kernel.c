#include <am.h>
#include <amdev.h>
#include <klib.h>
#include <klib-macros.h>
#include "p2.h"

#define SIDE 1
#define P __2_bmp
#define Len __2_bmp_len

static int w, h;  // Screen size

#define KEYNAME(key) \
  [AM_KEY_##key] = #key,
static const char *key_names[] = { AM_KEYS(KEYNAME) };

static inline void puts(const char *s) {
  for (; *s; s++) putch(*s);
}

void print_key() {
  AM_INPUT_KEYBRD_T event = { .keycode = AM_KEY_NONE };
  ioe_read(AM_INPUT_KEYBRD, &event);
  if (event.keycode != AM_KEY_NONE && event.keydown) {
    puts("Key pressed: ");
    puts(key_names[event.keycode]);
    puts("\n");
  }
}

static void draw_tile(int x, int y, int w, int h, uint32_t color) {
  uint32_t pixels[w * h]; // WARNING: large stack-allocated memory
  AM_GPU_FBDRAW_T event = {
    .x = x, .y = y, .w = w, .h = h, .sync = 1,
    .pixels = pixels,
  };
  for (int i = 0; i < w * h; i++) {
    pixels[i] = color;
  }
  ioe_write(AM_GPU_FBDRAW, &event);
}


void splash() {
  AM_GPU_CONFIG_T info = {0};
  ioe_read(AM_GPU_CONFIG, &info);
  w = info.width;
  h = info.height;
  printf("w:%d\th:%d\n", w, h);
  
  unsigned offset = *(unsigned*)(P + 0xa);
  unsigned i = offset;
  unsigned W = *(unsigned*)(P + 0x12), H = (*(unsigned*)(P + 0x16));
  unsigned bitcount = (unsigned)(((unsigned)(P[0x1c])) | ((unsigned)(P[0x1d]) << 8));
  unsigned bytecount = bitcount >> 3;

  printf("offset: %d\tW: %d\tH: %d\tbitcount: %d\tbytecount: %d\n", offset, W, H, bitcount, bytecount);

  for (int x = 0; x  < h; x ++) {
    for (int y = 0; y < w; y++) {
      // 32-bit Bmp;
      unsigned blue = (unsigned)P[i], green = (unsigned)P[i + 1], red = (unsigned)P[i + 2];
      i += (char)bytecount;
      unsigned rgb = (red << 16) | (green << 8) | blue;
      draw_tile(y * SIDE, x * SIDE, SIDE, SIDE, rgb); // white
    }
  }
}


// Operating system is a C program!
int main(const char *args) {
  ioe_init();

  puts("mainargs = \"");
  puts(args);  // make run mainargs=xxx
  puts("\"\n");

  splash();

  puts("Press any key to see its key code...\n");
  while (1) {
    AM_INPUT_KEYBRD_T event = { .keycode = AM_KEY_NONE };
    ioe_read(AM_INPUT_KEYBRD, &event);
    if (event.keycode ==  AM_KEY_ESCAPE) break;
  }

  return 0;
}
