#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
/* char -> int */
static unsigned hexval(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return ~0;
}
/* 16 -> sha1 */
int get_sha1_hex(char *hex, unsigned char *sha1) {
  int i;
  for (i = 0; i < 20; i++) {
    unsigned int val = (hexval(hex[0]) << 4) | hexval(hex[1]);
    if (val & ~0xff) /* if val & 11111100 >0  == val >256 */
      return -1;
    *sha1++ = val;
    hex += 2;
  }
  return 0;
}

char *sha1_to_hex(unsigned char *sha1) {
  static char buffer[50];
  static const char hex[] = "0123456789abcdef";
  char *buf = buffer;
  int i;

  for (i = 0; i < 20; i++) {
    unsigned int val = *sha1++;
    *buf++ = hex[val >> 4];
    *buf++ = hex[val & 0xf];
  }
  return buffer;
}
char *sha1_file_name(unsigned char *sha1) {
  int i;
  static char *name, *base;

  if (!base) {
    char *sha1_file_directory = ".dircache/objects";
    int len = strlen(sha1_file_directory);
    base = malloc(len + 60);
    memcpy(base, sha1_file_directory, len);
    memset(base + len, 0, 60);
    base[len] = '/';
    base[len + 3] = '/';
    name = base + len + 1;
  }
  for (i = 0; i < 20; i++) {
    static char hex[] = "0123456789abcdef";
    unsigned int val = sha1[i];
    char *pos = name + i * 2 + (i > 0);
    *pos++ = hex[val >> 4];
    *pos = hex[val & 0xf];
  }
  return base;
}
static int verify_path(char *path) {
  char c;

  goto inside;
  for (;;) {
    if (!c)
      return 1;
    if (c == '/') {
    inside:
      c = *path++;
      if (c != '/' && c != '.' && c != '\0')
        continue;
      return 0;
    }
    c = *path++;
  }
}
void test1() {
  unsigned char *sha1 = malloc(sizeof(unsigned char) * 20);
  get_sha1_hex("a5307a3ac32b7a8e2dd5ac63eabad5c515429b5d", sha1);
  // for (size_t i = 0; i < 20; i++) {
  //   printf("%u\n", sha1[i]);
  // }
  printf("%d", verify_path("abc"));

  // printf("%s\n", sha1_to_hex(sha1));
  // printf("%s\n", sha1_file_name(sha1));
}
// void test2() {
// z_stream stream;


// }
int main(int argc, char const *argv[]) { return 0; }
