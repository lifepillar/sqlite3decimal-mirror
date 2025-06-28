/**
 * \file mkversion.c
 *
 * \brief Generates version.h from Fossil's manifest files and Makefile.
 *
 * This C program is used to generate src/version.h. Intended usage:
 *
 *     ./tools/mkversion VERSION >src/version.h
 *
 * \note Adapted by Lifepillar from Fossil's mkversion.c.
 */
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


static FILE* open_for_reading(char const filename[static 1]) {
  FILE* f = fopen(filename, "r");

  if (f == 0) {
    fprintf(stderr, "Cannot open \"%s\" for reading.\n", filename);
    exit(EXIT_FAILURE);
  }

  return f;
}


static void trim(char s[static 1]) { // Trim `s` at the first trailing new line
  for (; *s && *s != '\n' && *s != '\r'; ++s) { }

  *s = '\0';
}


int main(int argc, char* argv[argc + 1]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s VERSION\n", argv[0]);

    return EXIT_FAILURE;
  }

  char text[1000] = {0};

  FILE* version = open_for_reading(argv[1]);

  if (fgets(text, sizeof(text) - 1, version) == nullptr) {
    fprintf(stderr, "Malformed VERSION file: %s\n", argv[3]);

    return EXIT_FAILURE;
  }

  fclose(version);

  trim(text);

  printf("\n/**\n");
  printf(" * \\brief SQLite3 Decimal short version string.\n");
  printf(" */\n");
  printf("#define SQLITE_DECIMAL_SHORT_VERSION \"%s\"\n\n", text);

  char* z = text;
  unsigned int vn[3] = {0};
  unsigned int x = 0;
  size_t j = 0;

  while (true) {
    if (z[0] >= '0' && z[0] <= '9') {
      x = x * 10 + z[0] - '0';
    }
    else {
      if (j < 3)
        vn[j++] = x;

      x = 0;

      if (z[0] == '\0')
        break;
    }

    ++z;
  }

  printf("#define RELEASE_VERSION_NUMBER %d%02d%02d\n", vn[0], vn[1], vn[2]);

  char vx[1000] = {0};
  unsigned int d = 0;

  strncpy(vx, text, 1000);

  for (z = vx; z[0]; ++z) {
    if (z[0] != '.')
      continue;

    if (d < 3) {
      z[0] = ',';
      ++d;
    }
    else {
      z[0] = '\0';
      break;
    }
  }

  printf("#define RELEASE_RESOURCE_VERSION %s", vx);

  while (d < 3) {
    printf(",0");
    ++d;
  }

  printf("\n");

  return EXIT_SUCCESS;
}
