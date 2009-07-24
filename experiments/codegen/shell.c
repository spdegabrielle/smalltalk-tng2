#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#include "scheme-private.h"
#include "scheme.h"

#include "libdis.h"

static void die(char const *format, ...) {
  va_list vl;
  va_start(vl, format);
  vfprintf(stderr, format, vl);
  va_end(vl);
  exit(1);
}

static pointer scm_build_native(scheme *sc, pointer args) {
  if (!is_pair(args)) {
    return sc->F;
  } else {
    pointer s = pair_car(args);
    char *bytes;
    int len;
    char *newbytes;

    if (!is_string(s)) {
      return sc->F;
    }

    len = s->_object._string._length;
    bytes = s->_object._string._svalue;
    newbytes = malloc(len);
    memcpy(newbytes, bytes, len);

    return mk_foreign_func(sc, (foreign_func) newbytes);
  }
}

static pointer scm_disassemble(scheme *sc, pointer args) {
  char line[128];
  pointer s;
  char *bytes;
  int len;
  int pos = 0;
  x86_insn_t i;

  if (!is_pair(args)) return sc->F;
  s = pair_car(args);
  if (!is_string(s)) return sc->F;

  len = s->_object._string._length;
  bytes = s->_object._string._svalue;

  x86_init(opt_none, NULL, NULL);

  while (pos < len) {
    int i_size = x86_disasm((unsigned char *) bytes, len, 0, pos, &i);
    if (i_size) {
      x86_format_insn(&i, line, sizeof(line), att_syntax);
      printf("%s\n", line);
      pos += i_size;
    } else {
      printf("Invalid instruction\n");
      pos++;
    }
  }

  x86_cleanup();

  return sc->T;
}

int main(int argc, char *argv[]) {
  scheme *sc;
  FILE *f;
  scheme_registerable fns[] = {
    { &scm_build_native, "build-native-function" },
    { &scm_disassemble, "disassemble" },
  };

  sc = scheme_init_new();
  if (!sc) die("Could not initialise scheme\n");

  f = fopen("codegen.scm", "rt");
  if (!f) die("Could not open codegen.scm: %s\n", strerror(errno));

  scheme_set_input_port_file(sc, stdin);
  scheme_set_output_port_file(sc, stdout);
  scheme_register_foreign_func_list(sc, &fns[0], sizeof(fns) / sizeof(fns[0]));
  scheme_load_file(sc, f);
  fclose(f);
  scheme_load_file(sc, stdin);
  scheme_deinit(sc);
  free(sc);

  return 0;
}
