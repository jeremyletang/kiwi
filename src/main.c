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

int write_structs_decl(FILE *fh, FILE *fc, int ac, char *av[]) {
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
    "extern const %s_list *_%s_list;\n\n";
  int err = 0;
  for (int i = 1; i != ac; i += 1) {
    err |= fprintf(fh, fmt, av[i], av[i], av[i], av[i], av[i], av[i], av[i], av[i], av[i], av[i]);
  }
  for (int i = 1; i != ac; i += 1) {
    err |= fprintf(fc, "const %s_list *_%s_list = 0;\n", av[i], av[i]);
  }
  err |= fprintf(fc, "\n");
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
      "    n->next = 0;\n"
      "    if (lst->first == _first) {\n"
      "      _new->first = n;\n"
      "    } else {\n"
      "      current->next = n;\n"
      "    }\n"
      "    current = n;\n"
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

int _write_map_decl(FILE *f, int ac, char *av[]) {
  int err = 0;
  err |= fprintf(f, "#define map(lst, f) _Generic((f)");
  // write the macro first
  for (int i = 1; i != ac; i += 1) {
    for (int j = 1; j != ac; j += 1) {
      err |= fprintf(f, ", \\\n  %s (*)(const %s*): %s_list_map_%s", av[j], av[i], av[i], av[j]);
    }
  }
  err |= fprintf(f, ")(lst, f)\n\n");
  const char *fmt = "%s_list *%s_list_map_%s(%s_list *, %s (*)(const %s*));\n";
  // then func declarations
  for (int i = 1; i != ac; i += 1) {
    for (int j = 1; j != ac; j += 1) {
      err |= fprintf(f, fmt, av[j], av[i], av[j], av[i], av[j], av[i]);
    }
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_map_def(FILE *f, int ac, char *av[]) {
    const char *fmt =
      "%s_list *%s_list_map_%s(%s_list *lst, %s (*f)(const %s*)) {\n"
      "  if (!lst) { return 0; }\n"
      "  %s_list *_new = make(_%s_list);\n"
      "  if (!_new) { return 0; }\n"
      "  %s_value *_first = lst->first;\n"
      "  %s_value *n = 0, *current = 0;\n"
      "  for (; _first; _first = _first->next) {\n"
      "    n = malloc(sizeof(%s_value));\n"
      "    n->value = f(&_first->value);\n"
      "    n->next = 0;\n"
      "    if (lst->first == _first) {\n"
      "      _new->first = n;\n"
      "    } else {\n"
      "      current->next = n;\n"
      "    }\n"
      "    current = n;\n"
      "  }\n"
      "  _new->last = n;\n"
      "  _new->len = lst->len;\n"
      "  return _new;\n"
      "}\n\n";

  int err = 0;
  for (int i = 1; i != ac; i +=1) {
    for (int j = 1; j != ac; j += 1) {
      err |= fprintf(f, fmt, av[j], av[i], av[j], av[i], av[j], av[i],
                     av[j], av[j],
                     av[i], av[j],
                     av[j]);
    }
  }

  return err;
}

int write_map(FILE *fh, FILE *fc, int ac, char *av[]) {
  return _write_map_decl(fh, ac, av) | _write_map_def(fc, ac, av);
}

int _write_mapi_decl(FILE *f, int ac, char *av[]) {
  int err = 0;
  err |= fprintf(f, "#define mapi(lst, f) _Generic((f)");
  // write the macro first
  for (int i = 1; i != ac; i += 1) {
    for (int j = 1; j != ac; j += 1) {
      err |= fprintf(f, ", \\\n  %s (*)(unsigned int, const %s*): %s_list_mapi_%s",
                     av[j], av[i], av[i], av[j]);
    }
  }
  err |= fprintf(f, ")(lst, f)\n\n");
  const char *fmt = "%s_list *%s_list_mapi_%s(%s_list *, %s (*)(unsigned int, const %s*));\n";
  // then func declarations
  for (int i = 1; i != ac; i += 1) {
    for (int j = 1; j != ac; j += 1) {
      err |= fprintf(f, fmt, av[j], av[i], av[j], av[i], av[j], av[i]);
    }
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_mapi_def(FILE *f, int ac, char *av[]) {
    const char *fmt =
      "%s_list *%s_list_mapi_%s(%s_list *lst, %s (*f)(unsigned int, const %s*)) {\n"
      "  if (!lst) { return 0; }\n"
      "  %s_list *_new = make(_%s_list);\n"
      "  if (!_new) { return 0; }\n"
      "  %s_value *_first = lst->first;\n"
      "  %s_value *n = 0, *current = 0;\n"
      "  unsigned int i = 0;\n"
      "  for (; _first; _first = _first->next) {\n"
      "    n = malloc(sizeof(%s_value));\n"
      "    n->value = f(i, &_first->value);\n"
      "    i += 1;\n"
      "    n->next = 0;\n"
      "    if (lst->first == _first) {\n"
      "      _new->first = n;\n"
      "    } else {\n"
      "      current->next = n;\n"
      "    }\n"
      "    current = n;\n"
      "  }\n"
      "  _new->last = n;\n"
      "  _new->len = lst->len;\n"
      "  return _new;\n"
      "}\n\n";

  int err = 0;
  for (int i = 1; i != ac; i +=1) {
    for (int j = 1; j != ac; j += 1) {
      err |= fprintf(f, fmt, av[j], av[i], av[j], av[i], av[j], av[i],
                     av[j], av[j],
                     av[i], av[j],
                     av[j]);
    }
  }

  return err;
}

int write_mapi(FILE *fh, FILE *fc, int ac, char *av[]) {
  return _write_mapi_decl(fh, ac, av) | _write_mapi_def(fc, ac, av);
}

int _write_iter_decl(FILE *f, int ac, char *av[]) {
  int err = 0;
  err |= fprintf(f, "#define iter(lst, f) _Generic((lst)");
  // write the macro first
  for (int i = 1; i != ac; i += 1) {
    err |= fprintf(f, ", \\\n  %s_list*: %s_list_iter", av[i], av[i]);
  }
  err |= fprintf(f, ")(lst, f)\n\n");
  // then func declarations
  for (int i = 1; i != ac; i += 1) {
    err |= fprintf(f, "void %s_list_iter(%s_list *, void (*)(const %s*));\n",
                   av[i], av[i], av[i]);
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_iter_def(FILE *f, int ac, char *av[]) {
    const char *fmt =
      "void %s_list_iter(%s_list *lst, void (*f)(const %s*)) {\n"
      "  if (lst) {\n"
      "    %s_value *_first = lst->first;\n"
      "    for (; _first; _first = _first->next) {\n"
      "      f(&_first->value);\n"
      "    }\n"
      "  }\n"
      "}\n\n";

  int err = 0;
  for (int i = 1; i != ac; i +=1) {
    err |= fprintf(f, fmt, av[i], av[i], av[i], av[i]);
  }

  return err;
}

int write_iter(FILE *fh, FILE *fc, int ac, char *av[]) {
  return _write_iter_decl(fh, ac, av) | _write_iter_def(fc, ac, av);
}

int _write_iteri_decl(FILE *f, int ac, char *av[]) {
  int err = 0;
  err |= fprintf(f, "#define iteri(lst, f) _Generic((lst)");
  // write the macro first
  for (int i = 1; i != ac; i += 1) {
    err |= fprintf(f, ", \\\n  %s_list*: %s_list_iteri", av[i], av[i]);
  }
  err |= fprintf(f, ")(lst, f)\n\n");
  // then func declarations
  for (int i = 1; i != ac; i += 1) {
    err |= fprintf(f, "void %s_list_iteri(%s_list *, void (*)(unsigned int, const %s*));\n",
                   av[i], av[i], av[i]);
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_iteri_def(FILE *f, int ac, char *av[]) {
    const char *fmt =
      "void %s_list_iteri(%s_list *lst, void (*f)(unsigned int, const %s*)) {\n"
      "  if (lst) {\n"
      "    %s_value *_first = lst->first;\n"
      "    unsigned int i = 0;\n"
      "    for (; _first; _first = _first->next) {\n"
      "      f(i, &_first->value);\n"
      "      i += 1;\n"
      "    }\n"
      "  }\n"
      "}\n\n";

  int err = 0;
  for (int i = 1; i != ac; i +=1) {
    err |= fprintf(f, fmt, av[i], av[i], av[i], av[i]);
  }

  return err;
}

int write_iteri(FILE *fh, FILE *fc, int ac, char *av[]) {
  return _write_iteri_decl(fh, ac, av) | _write_iteri_def(fc, ac, av);
}

int _write_any_decl(FILE *f, int ac, char *av[]) {
  int err = 0;
  err |= fprintf(f, "#define any(lst, f) _Generic((lst)");
  // write the macro first
  for (int i = 1; i != ac; i += 1) {
    err |= fprintf(f, ", \\\n  %s_list*: %s_list_any", av[i], av[i]);
  }
  err |= fprintf(f, ")(lst, f)\n\n");
  // then func declarations
  for (int i = 1; i != ac; i += 1) {
    err |= fprintf(f, "bool %s_list_any(%s_list *, bool (*)(const %s*));\n",
                   av[i], av[i], av[i]);
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_any_def(FILE *f, int ac, char *av[]) {
    const char *fmt =
      "bool %s_list_any(%s_list *lst, bool (*f)(const %s*)) {\n"
      "  if (lst) {\n"
      "    %s_value *_first = lst->first;\n"
      "    bool r = false;\n"
      "    for (; _first; _first = _first->next) {\n"
      "      r |= f(&_first->value);\n"
      "    }\n"
      "    return r;\n"
      "  }\n"
      "  return false;\n"
      "}\n\n";

  int err = 0;
  for (int i = 1; i != ac; i +=1) {
    err |= fprintf(f, fmt, av[i], av[i], av[i], av[i]);
  }

  return err;
}

int write_any(FILE *fh, FILE *fc, int ac, char *av[]) {
  return _write_any_decl(fh, ac, av) | _write_any_def(fc, ac, av);
}

int _write_all_decl(FILE *f, int ac, char *av[]) {
  int err = 0;
  err |= fprintf(f, "#define all(lst, f) _Generic((lst)");
  // write the macro first
  for (int i = 1; i != ac; i += 1) {
    err |= fprintf(f, ", \\\n  %s_list*: %s_list_all", av[i], av[i]);
  }
  err |= fprintf(f, ")(lst, f)\n\n");
  // then func declarations
  for (int i = 1; i != ac; i += 1) {
    err |= fprintf(f, "bool %s_list_all(%s_list *, bool (*)(const %s*));\n",
                   av[i], av[i], av[i]);
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_all_def(FILE *f, int ac, char *av[]) {
    const char *fmt =
      "bool %s_list_all(%s_list *lst, bool (*f)(const %s*)) {\n"
      "  if (lst) {\n"
      "    %s_value *_first = lst->first;\n"
      "    bool r = true;\n"
      "    for (; _first; _first = _first->next) {\n"
      "      r &= f(&_first->value);\n"
      "    }\n"
      "    return r;\n"
      "  }\n"
      "  return false;\n"
      "}\n\n";

  int err = 0;
  for (int i = 1; i != ac; i +=1) {
    err |= fprintf(f, fmt, av[i], av[i], av[i], av[i]);
  }

  return err;
}

int write_all(FILE *fh, FILE *fc, int ac, char *av[]) {
  return _write_all_decl(fh, ac, av) | _write_all_def(fc, ac, av);
}

int _write_rev_decl(FILE *f, int ac, char *av[]) {
  int err = 0;
  err |= fprintf(f, "#define rev(lst) _Generic((lst)");
  // write the macro first
  for (int i = 1; i != ac; i += 1) {
    err |= fprintf(f, ", \\\n  %s_list*: %s_list_rev", av[i], av[i]);
  }
  err |= fprintf(f, ")(lst)\n\n");
  // then func declarations
  for (int i = 1; i != ac; i += 1) {
    err |= fprintf(f, "%s_list *%s_list_rev(%s_list *);\n", av[i], av[i], av[i]);
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_rev_def(FILE *f, int ac, char *av[]) {
    const char *fmt =
      "%s_list *%s_list_rev(%s_list *lst) {\n"
      "  if (!lst) { return 0; }\n"
      "  %s_list *_new = make(_%s_list);\n"
      "  if (!_new) { return 0; }\n"
      "  %s_value *_first = lst->first, *n = 0, *current = 0;\n"
      "  for (; _first; _first = _first->next) {\n"
      "    n = malloc(sizeof(%s_value));\n"
      "    n->value = _first->value;\n"
      "    if (lst->first == _first) {\n"
      "      n->next = 0;\n"
      "      _new->last = n;\n"
      "    } else {\n"
      "      n->next = current;\n"
      "    }\n"
      "    current = n;\n"
      "  }\n"
      "  _new->first = n;\n"
      "  _new->len = lst->len;\n"
      "  return _new;\n"
      "}\n\n";

  int err = 0;
  for (int i = 1; i != ac; i +=1) {
    err |= fprintf(f, fmt, av[i], av[i], av[i], av[i], av[i], av[i], av[i]);
  }

  return err;
}

int write_rev(FILE *fh, FILE *fc, int ac, char *av[]) {
  return _write_rev_decl(fh, ac, av) | _write_rev_def(fc, ac, av);
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

  // include guards
  fprintf(fh, "#ifndef KIWI_H\n#define KIWI_H\n\n");
  write_includes(fh, fc);
  write_structs_decl(fh, fc, ac, av);
  write_make(fh, fc, ac, av);
  write_drop(fh, fc, ac, av);
  write_append(fh, fc, ac, av);
  write_copy(fh, fc, ac, av);
  write_map(fh, fc, ac, av);
  write_mapi(fh, fc, ac, av);
  write_iter(fh, fc, ac, av);
  write_iteri(fh, fc, ac, av);
  write_any(fh, fc, ac, av);
  write_all(fh, fc, ac, av);
  write_rev(fh, fc, ac, av);
  // endif include guards
  fprintf(fh, "#endif // KIWI_H\n");

  return 0;
}

int main(int ac, char *av[]) {
  if (!(ac > 1)) {
    printf("error: one type required at least");
    return 1;
  }
  return do_stuff(ac, av);
}
