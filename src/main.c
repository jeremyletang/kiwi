#include <stdio.h>
#include <stdbool.h>
#include <string.h>

const char *out = "./kiwi/";
const char *hout = "kiwi.h";
const char *cout = "kiwi.c";

FILE *_open(const char *name) {
  FILE *f = 0;
  char path[256];
  memset(path, 0, 256);
  strcat(path, out);
  strcat(path, name);
  if ((f = fopen(path, "w")) == 0) {
    printf("error: unable to open '%s'\n", path);
    return 0;
  }
  return f;
}

int write_structs_decl(FILE *f, int ac, char *av[]) {
  const char *fmt =
    "typedef struct %s_value {\n"
    "  %s value;\n"
    "  struct %s_value *next;\n"
    "} %s_value;\n"
    "\n"
    "typedef struct %s_list{\n"
    "  struct %s_value *first;\n"
    "  struct %s_value *last;\n"
    "  unsigned int len;\n"
    "} %s_list;\n"
    "\n"
    "static const %s_list *_%s_list;\n\n";
  int err = 0;
  for (int i = 1; i != ac; i += 1) {
    err |= fprintf(f, fmt, av[i], av[i], av[i], av[i], av[i], av[i], av[i], av[i], av[i], av[i]);
  }
  return err;
}

int _write_make_decl(FILE *f, int ac, char *av[]) {
  int err = 0;
  err |= fprintf(f, "#define make(lst) _Generic((lst)");
  // write the macro first
  for (int i = 1; i != ac; i += 1) {
    err |= fprintf(f, ", \\\n  const %s_list*: %s_list_make", av[i], av[i]);
  }
  err |= fprintf(f, ")()\n\n");
  // then func declarations
  for (int i = 1; i != ac; i += 1) {
    err |= fprintf(f, "%s_list *%s_list_make();\n", av[i], av[i]);
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_make_def(FILE *f, int ac, char *av[]) {
  const char *fmt =
    "%s_list *%s_list_make() {\n"
    "  %s_list *lst = malloc(sizeof(%s_list));\n"
    "  if (lst) {\n"
    "    lst->first = 0;\n"
    "    lst->last = 0;\n"
    "    lst->len = 0;\n"
    "  }\n"
    "  return lst;\n"
    "}\n\n";

  int err = 0;
  for (int i = 1; i != ac; i +=1) {
    err |= fprintf(f, fmt, av[i], av[i], av[i], av[i]);
  }

  return err;
}

int write_make(FILE *fh, FILE *fc, int ac, char *av[]) {
  return _write_make_decl(fh, ac, av) | _write_make_def(fc, ac, av);
}

int _write_append_decl(FILE *f, int ac, char *av[]) {
  int err = 0;
  err |= fprintf(f, "#define append(lst, v) _Generic((lst)");
  // write the macro first
  for (int i = 1; i != ac; i += 1) {
    err |= fprintf(f, ", \\\n  %s_list*: %s_list_append", av[i], av[i]);
  }
  err |= fprintf(f, ")(lst, v)\n\n");
  // then func declarations
  for (int i = 1; i != ac; i += 1) {
    err |= fprintf(f, "%s_list *%s_list_append(%s_list *, %s);\n", av[i], av[i], av[i], av[i]);
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_append_def(FILE *f, int ac, char *av[]) {
    const char *fmt =
      "%s_list *%s_list_append(%s_list *lst, %s v) {\n"
      "  if (!lst) { return 0; }\n"
      "  %s_value *_new = malloc(sizeof(%s_value));\n"
      "  if (!_new) { return 0; }\n"
      "  _new->value = v;\n"
      "  _new->next = 0;\n"
      "  if (!lst->first) {\n"
      "    lst->first = _new;\n"
      "  } else {\n"
      "    lst->last->next = _new;\n"
      "  }\n"
      "  lst->last = _new;\n"
      "  return lst;\n"
      "}\n\n";

  int err = 0;
  for (int i = 1; i != ac; i +=1) {
    err |= fprintf(f, fmt, av[i], av[i], av[i], av[i], av[i], av[i]);
  }

  return err;
}

int write_append(FILE *fh, FILE *fc, int ac, char *av[]) {
  return _write_append_decl(fh, ac, av) | _write_append_def(fc, ac, av);
}

int _write_drop_decl(FILE *f, int ac, char *av[]) {
  int err = 0;
  err |= fprintf(f, "#define drop(lst) _Generic((lst)");
  // write the macro first
  for (int i = 1; i != ac; i += 1) {
    err |= fprintf(f, ", \\\n  %s_list*: %s_list_drop", av[i], av[i]);
  }
  err |= fprintf(f, ")(lst)\n\n");
  // then func declarations
  for (int i = 1; i != ac; i += 1) {
    err |= fprintf(f, "void %s_list_drop(%s_list *);\n", av[i], av[i]);
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_drop_def(FILE *f, int ac, char *av[]) {
    const char *fmt =
      "void %s_list_drop(%s_list *lst) {\n"
      "  if (lst) {\n"
      "    while (lst->first) {\n"
      "      %s_value *_next = lst->first->next;\n"
      "      free(lst->first);\n"
      "      lst->first = _next;\n"
      "    }\n"
      "    free(lst);\n"
      "  }\n"
      "}\n\n";

  int err = 0;
  for (int i = 1; i != ac; i +=1) {
    err |= fprintf(f, fmt, av[i], av[i], av[i]);
  }

  return err;
}

int write_drop(FILE *fh, FILE *fc, int ac, char *av[]) {
  return _write_drop_decl(fh, ac, av) | _write_drop_def(fc, ac, av);
}

int _write_copy_decl(FILE *f, int ac, char *av[]) {
  int err = 0;
  err |= fprintf(f, "#define copy(lst) _Generic((lst)");
  // write the macro first
  for (int i = 1; i != ac; i += 1) {
    err |= fprintf(f, ", \\\n  %s_list*: %s_list_copy", av[i], av[i]);
  }
  err |= fprintf(f, ")(lst)\n\n");
  // then func declarations
  for (int i = 1; i != ac; i += 1) {
    err |= fprintf(f, "%s_list *%s_list_copy(%s_list *);\n", av[i], av[i], av[i]);
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_copy_def(FILE *f, int ac, char *av[]) {
    const char *fmt =
      "%s_list *%s_list_copy(%s_list *lst) {\n"
      "  if (!lst) { return 0; }\n"
      "  %s_list *_new = make(_%s_list);\n"
      "  if (!_new) { return 0; }\n"
      "  %s_value *_first = lst->first, *n = 0, *current = 0;\n"
      "  for (; _first; _first = _first->next) {\n"
      "    n = malloc(sizeof(%s_value));\n"
      "    n->value = _first->value;\n"
      "    if (lst->first == _first) {\n"
      "      _new->first = n;\n"
      "      current = n;\n"
      "    } else {\n"
      "      current->next = n;\n"
      "      current = n;\n"
      "    }\n"
      "  }\n"
      "  _new->last = n;\n"
      "  _new->len = lst->len;\n"
      "  return _new;\n"
      "}\n\n";

  int err = 0;
  for (int i = 1; i != ac; i +=1) {
    err |= fprintf(f, fmt, av[i], av[i], av[i], av[i], av[i], av[i], av[i]);
  }

  return err;
}

int write_copy(FILE *fh, FILE *fc, int ac, char *av[]) {
  return _write_copy_decl(fh, ac, av) | _write_copy_def(fc, ac, av);
}

int write_includes(FILE *fh, FILE *fc) {
  const char *fmtc =
    "#include \"kiwi.h\"\n"
    "#include <stdlib.h>\n"
    "#include <stdio.h>\n\n";
  const char *fmth =
    "#include <stdbool.h>\n\n";

  return fprintf(fh, "%s", fmth) | fprintf(fc, "%s", fmtc);
}

int do_stuff(int ac, char *av[]) {
  FILE *fh = 0;
  FILE *fc = 0;

  if ((fh = _open(hout)) == 0 || (fc = _open(cout)) == 0) {
    return 1;
  }

  fprintf(fh, "/* this is a generated file, do not edit. */\n\n");
  fprintf(fc, "/* this is a generated file, do not edit. */\n\n");

  write_includes(fh, fc);
  write_structs_decl(fh, ac, av);
  write_make(fh, fc, ac, av);
  write_append(fh, fc, ac, av);
  write_drop(fh, fc, ac, av);
  write_copy(fh, fc, ac, av);

  return 0;
}

int main(int ac, char *av[]) {
  if (!(ac > 1)) {
    printf("error: one type required at least");
    return 1;
  }
  return do_stuff(ac, av);
}
