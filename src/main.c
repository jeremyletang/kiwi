// Copyright 2017 Jeremy Letang.
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

const char *hout = "kiwi.h";
const char *cout = "kiwi.c";

typedef struct {
  char *name;
  char *summary;
  bool optional;
  bool has_param;
} opt_def;

typedef struct {
  char *name;
  char *arg;
} opt;

typedef struct opt_list {
  opt *opt;
  struct opt_list *next;
} opt_list;

typedef struct parsed_opts {
  bool success;
  opt_list *opts;
  char **files;
} parsed_opts;

void print_help();
const opt *has_opt(parsed_opts *opts, char *str);
void parsed_opts_free(parsed_opts *p);
unsigned int str_array_len(const char **arr);
parsed_opts *parse_opts(int args, char *argv[]);
bool has_all_required_opt(parsed_opts *op);

FILE *_open(const char *out, const char *name) {
  FILE *f = 0;
  char path[256];
  memset(path, 0, 256);
  strcat(path, out);
  strcat(path, "/");
  strcat(path, name);
  if ((f = fopen(path, "w")) == 0) {
    printf("error: unable to open '%s'\n", path);
    return 0;
  }
  return f;
}

typedef struct split_ty {
  char *ty;
  char *alias;
  int sep;
} split_ty;

split_ty split_ty_make(char *s) {
  split_ty st = {.ty = s, .alias = s, .sep = -1};
  char *result = s;
  if ((result = strchr(result, ':')) != 0) {
    st.ty = (result + 1);
    *result = '\0';
    st.sep = (result - s);
  }

  return st;
}

void split_ty_reset(split_ty s) {
  if (s.sep != -1) {
    s.alias[s.sep] = ':';
  }
}

int write_structs_decl(FILE *fh, FILE *fc, int ac, split_ty *sts) {
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
  for (int i = 0; i != ac; i += 1) {
    //split_ty s = split_ty_make(av[i]);
    // header
    split_ty s = sts[i];
    err |= fprintf(fh, fmt, s.alias, s.ty, s.alias, s.alias, s.alias, s.alias, s.alias,
                   s.alias, s.alias, s.alias);
    // c
    err |= fprintf(fc, "const %s_list *_%s_list = 0;\n", s.alias, s.alias);
    /// split_ty_reset(s);
  }
  err |= fprintf(fc, "\n");
  return err;
}

int _write_make_decl(FILE *f, int ac, split_ty *sts) {
  int err = 0;
  err |= fprintf(f, "#define make(lst) _Generic((lst)");
  // write the macro first
  for (int i = 0; i != ac; i += 1) {
    split_ty s = sts[i];
    err |= fprintf(f, ", \\\n  const %s_list*: %s_list_make", s.alias, s.alias);
  }
  err |= fprintf(f, ")()\n\n");
  // then func declarations
  for (int i = 0; i != ac; i += 1) {
    split_ty s = sts[i];
    err |= fprintf(f, "%s_list *%s_list_make();\n", s.alias, s.alias);
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_make_def(FILE *f, int ac, split_ty *sts) {
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
  for (int i = 0; i != ac; i +=1) {
    err |= fprintf(f, fmt, sts[i].alias, sts[i].alias, sts[i].alias, sts[i].alias);
  }

  return err;
}

int write_make(FILE *fh, FILE *fc, int ac, split_ty *sts) {
  return _write_make_decl(fh, ac, sts) | _write_make_def(fc, ac, sts);
}

int _write_append_decl(FILE *f, int ac, split_ty *sts) {
  int err = 0;
  err |= fprintf(f, "#define append(lst, v) _Generic((lst)");
  // write the macro first
  for (int i = 0; i != ac; i += 1) {
    err |= fprintf(f, ", \\\n  %s_list*: %s_list_append", sts[i].alias, sts[i].alias);
  }
  err |= fprintf(f, ")(lst, v)\n\n");
  // then func declarations
  for (int i = 0; i != ac; i += 1) {
    err |= fprintf(f, "%s_list *%s_list_append(%s_list *, %s);\n",
                   sts[i].alias, sts[i].alias, sts[i].alias, sts[i].ty);
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_append_def(FILE *f, int ac, split_ty *sts) {
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
  for (int i = 0; i != ac; i +=1) {
    err |= fprintf(f, fmt, sts[i].alias, sts[i].alias, sts[i].alias, sts[i].ty,
                   sts[i].alias, sts[i].alias);
  }

  return err;
}

int write_append(FILE *fh, FILE *fc, int ac, split_ty *sts) {
  return _write_append_decl(fh, ac, sts) | _write_append_def(fc, ac, sts);
}

int _write_drop_decl(FILE *f, int ac, split_ty *sts) {
  int err = 0;
  err |= fprintf(f, "#define drop(lst) _Generic((lst)");
  // write the macro first
  for (int i = 0; i != ac; i += 1) {
    err |= fprintf(f, ", \\\n  %s_list*: %s_list_drop", sts[i].alias, sts[i].alias);
  }
  err |= fprintf(f, ")(lst)\n\n");
  // then func declarations
  for (int i = 0; i != ac; i += 1) {
    err |= fprintf(f, "void %s_list_drop(%s_list *);\n", sts[i].alias, sts[i].alias);
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_drop_def(FILE *f, int ac, split_ty *sts) {
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
  for (int i = 0; i != ac; i +=1) {
    err |= fprintf(f, fmt, sts[i].alias, sts[i].alias, sts[i].alias);
  }

  return err;
}

int write_drop(FILE *fh, FILE *fc, int ac, split_ty *sts) {
  return _write_drop_decl(fh, ac, sts) | _write_drop_def(fc, ac, sts);
}

int _write_copy_decl(FILE *f, int ac, split_ty *sts) {
  int err = 0;
  err |= fprintf(f, "#define copy(lst) _Generic((lst)");
  // write the macro first
  for (int i = 0; i != ac; i += 1) {
    err |= fprintf(f, ", \\\n  %s_list*: %s_list_copy", sts[i].alias, sts[i].alias);
  }
  err |= fprintf(f, ")(lst)\n\n");
  // then func declarations
  for (int i = 0; i != ac; i += 1) {
    err |= fprintf(f, "%s_list *%s_list_copy(%s_list *);\n",
                   sts[i].alias, sts[i].alias, sts[i].alias);
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_copy_def(FILE *f, int ac, split_ty *sts) {
    const char *fmt =
      "%s_list *%s_list_copy(%s_list *lst) {\n"
      "  if (!lst) { return 0; }\n"
      "  %s_list *_new = make(_%s_list);\n"
      "  if (!_new) { return 0; }\n"
      "  %s_value *_first = lst->first;\n"
      "  for (; _first; _first = _first->next) {\n"
      "    append(_new, _first->value);\n"
      "  }\n"
      "  _new->len = lst->len;\n"
      "  return _new;\n"
      "}\n\n";

  int err = 0;
  for (int i = 0; i != ac; i +=1) {
    err |= fprintf(f, fmt, sts[i].alias, sts[i].alias, sts[i].alias,
                   sts[i].alias, sts[i].alias,
                   sts[i].alias);
  }

  return err;
}

int write_copy(FILE *fh, FILE *fc, int ac, split_ty *sts) {
  return _write_copy_decl(fh, ac, sts) | _write_copy_def(fc, ac, sts);
}

int _write_map_decl(FILE *f, int ac, split_ty *sts) {
  int err = 0;
  err |= fprintf(f, "#define map(lst, f) _Generic((f)");
  // write the macro first
  for (int i = 0; i != ac; i += 1) {
    for (int j = 0; j != ac; j += 1) {
      err |= fprintf(f, ", \\\n  %s (*)(const %s*): %s_list_map_%s",
                     sts[j].ty, sts[i].ty, sts[i].alias, sts[j].alias);
    }
  }
  err |= fprintf(f, ")(lst, f)\n\n");
  const char *fmt = "%s_list *%s_list_map_%s(%s_list *, %s (*)(const %s*));\n";
  // then func declarations
  for (int i = 0; i != ac; i += 1) {
    for (int j = 0; j != ac; j += 1) {
      err |= fprintf(f, fmt,
                     sts[j].alias, sts[i].alias, sts[j].alias, sts[i].alias, sts[j].ty, sts[i].ty);
    }
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_map_def(FILE *f, int ac, split_ty *sts) {
    const char *fmt =
      "%s_list *%s_list_map_%s(%s_list *lst, %s (*f)(const %s*)) {\n"
      "  if (!lst) { return 0; }\n"
      "  %s_list *_new = make(_%s_list);\n"
      "  if (!_new) { return 0; }\n"
      "  %s_value *_first = lst->first;\n"
      "  for (; _first; _first = _first->next) {\n"
      "    append(_new, f((const %s*)&_first->value));\n"
      "  }\n"
      "  _new->len = lst->len;\n"
      "  return _new;\n"
      "}\n\n";

  int err = 0;
  for (int i = 0; i != ac; i +=1) {
    for (int j = 0; j != ac; j += 1) {
      err |= fprintf(f, fmt,
                     sts[j].alias, sts[i].alias, sts[j].alias, sts[i].alias, sts[j].ty, sts[i].ty,
                     sts[j].alias, sts[j].alias,
                     sts[i].alias, sts[i].ty);
    }
  }

  return err;
}

int write_map(FILE *fh, FILE *fc, int ac, split_ty *sts) {
  return _write_map_decl(fh, ac, sts) | _write_map_def(fc, ac, sts);
}

int _write_mapi_decl(FILE *f, int ac, split_ty *sts) {
  int err = 0;
  err |= fprintf(f, "#define mapi(lst, f) _Generic((f)");
  // write the macro first
  for (int i = 0; i != ac; i += 1) {
    for (int j = 0; j != ac; j += 1) {
      err |= fprintf(f, ", \\\n  %s (*)(unsigned int, const %s*): %s_list_mapi_%s",
                     sts[j].ty, sts[i].ty, sts[i].alias, sts[j].alias);
    }
  }
  err |= fprintf(f, ")(lst, f)\n\n");
  const char *fmt = "%s_list *%s_list_mapi_%s(%s_list *, %s (*)(unsigned int, const %s*));\n";
  // then func declarations
  for (int i = 0; i != ac; i += 1) {
    for (int j = 0; j != ac; j += 1) {
      err |= fprintf(f, fmt,
                     sts[j].alias, sts[i].alias, sts[j].alias, sts[i].alias, sts[j].ty, sts[i].ty);
    }
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_mapi_def(FILE *f, int ac, split_ty *sts) {
    const char *fmt =
      "%s_list *%s_list_mapi_%s(%s_list *lst, %s (*f)(unsigned int, const %s*)) {\n"
      "  if (!lst) { return 0; }\n"
      "  %s_list *_new = make(_%s_list);\n"
      "  if (!_new) { return 0; }\n"
      "  %s_value *_first = lst->first;\n"
      "  unsigned int i = 0;\n"
      "  for (; _first; _first = _first->next) {\n"
      "    append(_new, f(i, (const %s*)&_first->value));\n"
      "    i += 1;\n"
      "  }\n"
      "  _new->len = lst->len;\n"
      "  return _new;\n"
      "}\n\n";

  int err = 0;
  for (int i = 0; i != ac; i +=1) {
    for (int j = 0; j != ac; j += 1) {
      err |= fprintf(f, fmt,
                     sts[j].alias, sts[i].alias, sts[j].alias, sts[i].alias, sts[j].ty, sts[i].ty,
                     sts[j].alias, sts[j].alias,
                     sts[i].alias, sts[i].ty);
    }
  }

  return err;
}

int write_mapi(FILE *fh, FILE *fc, int ac, split_ty *sts) {
  return _write_mapi_decl(fh, ac, sts) | _write_mapi_def(fc, ac, sts);
}

int _write_iter_decl(FILE *f, int ac, split_ty *sts) {
  int err = 0;
  err |= fprintf(f, "#define iter(lst, f) _Generic((lst)");
  // write the macro first
  for (int i = 0; i != ac; i += 1) {
    err |= fprintf(f, ", \\\n  %s_list*: %s_list_iter", sts[i].alias, sts[i].alias);
  }
  err |= fprintf(f, ")(lst, f)\n\n");
  // then func declarations
  for (int i = 0; i != ac; i += 1) {
    err |= fprintf(f, "void %s_list_iter(%s_list *, void (*)(const %s*));\n",
                   sts[i].alias, sts[i].alias, sts[i].ty);
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_iter_def(FILE *f, int ac, split_ty *sts) {
    const char *fmt =
      "void %s_list_iter(%s_list *lst, void (*f)(const %s*)) {\n"
      "  if (lst) {\n"
      "    %s_value *_first = lst->first;\n"
      "    for (; _first; _first = _first->next) {\n"
      "      f((const %s*)&_first->value);\n"
      "    }\n"
      "  }\n"
      "}\n\n";

  int err = 0;
  for (int i = 0; i != ac; i +=1) {
    err |= fprintf(f, fmt, sts[i].alias, sts[i].alias, sts[i].ty, sts[i].alias, sts[i].ty);
  }

  return err;
}

int write_iter(FILE *fh, FILE *fc, int ac, split_ty *sts) {
  return _write_iter_decl(fh, ac, sts) | _write_iter_def(fc, ac, sts);
}

int _write_iteri_decl(FILE *f, int ac, split_ty *sts) {
  int err = 0;
  err |= fprintf(f, "#define iteri(lst, f) _Generic((lst)");
  // write the macro first
  for (int i = 0; i != ac; i += 1) {
    err |= fprintf(f, ", \\\n  %s_list*: %s_list_iteri", sts[i].alias, sts[i].alias);
  }
  err |= fprintf(f, ")(lst, f)\n\n");
  // then func declarations
  for (int i = 0; i != ac; i += 1) {
    err |= fprintf(f, "void %s_list_iteri(%s_list *, void (*)(unsigned int, const %s*));\n",
                   sts[i].alias, sts[i].alias, sts[i].ty);
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_iteri_def(FILE *f, int ac, split_ty *sts) {
    const char *fmt =
      "void %s_list_iteri(%s_list *lst, void (*f)(unsigned int, const %s*)) {\n"
      "  if (lst) {\n"
      "    %s_value *_first = lst->first;\n"
      "    unsigned int i = 0;\n"
      "    for (; _first; _first = _first->next) {\n"
      "      f(i, (const %s*)&_first->value);\n"
      "      i += 1;\n"
      "    }\n"
      "  }\n"
      "}\n\n";

  int err = 0;
  for (int i = 0; i != ac; i +=1) {
    err |= fprintf(f, fmt, sts[i].alias, sts[i].alias, sts[i].ty, sts[i].alias, sts[i].ty);
  }

  return err;
}

int write_iteri(FILE *fh, FILE *fc, int ac, split_ty *sts) {
  return _write_iteri_decl(fh, ac, sts) | _write_iteri_def(fc, ac, sts);
}

int _write_any_decl(FILE *f, int ac, split_ty *sts) {
  int err = 0;
  err |= fprintf(f, "#define any(lst, f) _Generic((lst)");
  // write the macro first
  for (int i = 0; i != ac; i += 1) {
    err |= fprintf(f, ", \\\n  %s_list*: %s_list_any", sts[i].alias, sts[i].alias);
  }
  err |= fprintf(f, ")(lst, f)\n\n");
  // then func declarations
  for (int i = 0; i != ac; i += 1) {
    err |= fprintf(f, "bool %s_list_any(%s_list *, bool (*)(const %s*));\n",
                   sts[i].alias, sts[i].alias, sts[i].ty);
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_any_def(FILE *f, int ac, split_ty *sts) {
    const char *fmt =
      "bool %s_list_any(%s_list *lst, bool (*f)(const %s*)) {\n"
      "  if (lst) {\n"
      "    %s_value *_first = lst->first;\n"
      "    bool r = false;\n"
      "    for (; _first; _first = _first->next) {\n"
      "      r |= f((const %s*)&_first->value);\n"
      "    }\n"
      "    return r;\n"
      "  }\n"
      "  return false;\n"
      "}\n\n";

  int err = 0;
  for (int i = 0; i != ac; i +=1) {
    err |= fprintf(f, fmt, sts[i].alias, sts[i].alias, sts[i].ty, sts[i].alias, sts[i].ty);
  }

  return err;
}

int write_any(FILE *fh, FILE *fc, int ac, split_ty *sts) {
  return _write_any_decl(fh, ac, sts) | _write_any_def(fc, ac, sts);
}

int _write_all_decl(FILE *f, int ac, split_ty *sts) {
  int err = 0;
  err |= fprintf(f, "#define all(lst, f) _Generic((lst)");
  // write the macro first
  for (int i = 0; i != ac; i += 1) {
    err |= fprintf(f, ", \\\n  %s_list*: %s_list_all", sts[i].alias, sts[i].alias);
  }
  err |= fprintf(f, ")(lst, f)\n\n");
  // then func declarations
  for (int i = 0; i != ac; i += 1) {
    err |= fprintf(f, "bool %s_list_all(%s_list *, bool (*)(const %s*));\n",
                   sts[i].alias, sts[i].alias, sts[i].ty);
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_all_def(FILE *f, int ac, split_ty *sts) {
    const char *fmt =
      "bool %s_list_all(%s_list *lst, bool (*f)(const %s*)) {\n"
      "  if (lst) {\n"
      "    %s_value *_first = lst->first;\n"
      "    bool r = true;\n"
      "    for (; _first; _first = _first->next) {\n"
      "      r &= f((const %s*)&_first->value);\n"
      "    }\n"
      "    return r;\n"
      "  }\n"
      "  return false;\n"
      "}\n\n";

  int err = 0;
  for (int i = 0; i != ac; i +=1) {
    err |= fprintf(f, fmt, sts[i].alias, sts[i].alias, sts[i].ty, sts[i].alias, sts[i].ty);
  }

  return err;
}

int write_all(FILE *fh, FILE *fc, int ac, split_ty *sts) {
  return _write_all_decl(fh, ac, sts) | _write_all_def(fc, ac, sts);
}

int _write_rev_decl(FILE *f, int ac, split_ty *sts) {
  int err = 0;
  err |= fprintf(f, "#define rev(lst) _Generic((lst)");
  // write the macro first
  for (int i = 0; i != ac; i += 1) {
    err |= fprintf(f, ", \\\n  %s_list*: %s_list_rev", sts[i].alias, sts[i].alias);
  }
  err |= fprintf(f, ")(lst)\n\n");
  // then func declarations
  for (int i = 0; i != ac; i += 1) {
    err |= fprintf(f, "%s_list *%s_list_rev(%s_list *);\n",
                   sts[i].alias, sts[i].alias, sts[i].alias);
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_rev_def(FILE *f, int ac, split_ty *sts) {
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
  for (int i = 0; i != ac; i +=1) {
    err |= fprintf(f, fmt, sts[i].alias, sts[i].alias, sts[i].alias,
                   sts[i].alias, sts[i].alias,
                   sts[i].alias,
                   sts[i].alias);
  }

  return err;
}

int write_rev(FILE *fh, FILE *fc, int ac, split_ty *sts) {
  return _write_rev_decl(fh, ac, sts) | _write_rev_def(fc, ac, sts);
}

int _write_find_decl(FILE *f, int ac, split_ty *sts) {
  int err = 0;
  err |= fprintf(f, "#define find(lst, f) _Generic((lst)");
  // write the macro first
  for (int i = 0; i != ac; i += 1) {
    err |= fprintf(f, ", \\\n  %s_list*: %s_list_find", sts[i].alias, sts[i].alias);
  }
  err |= fprintf(f, ")(lst, f)\n\n");
  // then func declarations
  for (int i = 0; i != ac; i += 1) {
    err |= fprintf(f, "const %s *%s_list_find(%s_list *, bool (*)(const %s*));\n",
                   sts[i].ty, sts[i].alias, sts[i].alias, sts[i].ty);
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_find_def(FILE *f, int ac, split_ty *sts) {
    const char *fmt =
      "const %s *%s_list_find(%s_list *lst, bool (*f)(const %s*)) {\n"
      "  if (lst) {\n"
      "    %s_value *_first = lst->first;\n"
      "    for (; _first; _first = _first->next) {\n"
      "      if (f((const %s*)&_first->value)) {\n"
      "        return (const %s*)&_first->value;\n"
      "      }\n"
      "    }\n"
      "  }\n"
      "  return 0;\n"
      "}\n\n";

  int err = 0;
  for (int i = 0; i != ac; i +=1) {
    err |= fprintf(f, fmt, sts[i].ty, sts[i].alias, sts[i].alias, sts[i].ty,
                   sts[i].alias,
                   sts[i].ty,
                   sts[i].ty);
  }

  return err;
}

int write_find(FILE *fh, FILE *fc, int ac, split_ty *sts) {
  return _write_find_decl(fh, ac, sts) | _write_find_def(fc, ac, sts);
}

int _write_filter_decl(FILE *f, int ac, split_ty *sts) {
  int err = 0;
  err |= fprintf(f, "#define filter(lst) _Generic((lst)");
  // write the macro first
  for (int i = 0; i != ac; i += 1) {
    err |= fprintf(f, ", \\\n  %s_list*: %s_list_filter", sts[i].alias, sts[i].alias);
  }
  err |= fprintf(f, ")(lst)\n\n");
  // then func declarations
  for (int i = 0; i != ac; i += 1) {
    err |= fprintf(f, "%s_list *%s_list_filter(%s_list *, bool (*)(const %s*));\n",
                   sts[i].alias, sts[i].alias, sts[i].alias, sts[i].ty);
  }
  err |= fprintf(f, "\n");

  return err;
}

int _write_filter_def(FILE *f, int ac, split_ty *sts) {
    const char *fmt =
      "%s_list *%s_list_filter(%s_list *lst, bool (*f)(const %s*)) {\n"
      "  if (!lst) { return 0; }\n"
      "  %s_list *_new = make(_%s_list);\n"
      "  if (!_new) { return 0; }\n"
      "  %s_value *_first = lst->first;\n"
      "  for (; _first; _first = _first->next) {\n"
      "    if (f((const %s*)&_first->value) == true) {\n"
      "      append(_new, _first->value);\n"
      "    }\n"
      "  }\n"
      "  _new->len = lst->len;\n"
      "  return _new;\n"
      "}\n\n";

  int err = 0;
  for (int i = 0; i != ac; i +=1) {
    err |= fprintf(f, fmt, sts[i].alias, sts[i].alias, sts[i].alias, sts[i].ty,
                   sts[i].alias, sts[i].alias,
                   sts[i].alias,
                   sts[i].ty);
  }

  return err;
}

int write_filter(FILE *fh, FILE *fc, int ac, split_ty *sts) {
  return _write_filter_decl(fh, ac, sts) | _write_filter_def(fc, ac, sts);
}

int write_includes(FILE *fh, FILE *fc, parsed_opts *opts) {
  const char *fmtc =
    "#include \"kiwi.h\"\n"
    "#include <stdlib.h>\n"
    "#include <stdio.h>\n\n";
  const char *fmth =
    "#include <stdbool.h>\n"
    "#include <stdint.h>\n";
  int err = fprintf(fh, "%s", fmth) | fprintf(fc, "%s", fmtc);

  if (has_opt(opts, "-I")) {
    fprintf(fh, "\n// user defined types headers\n");
    opt_list *o = opts->opts;
    for (; o; o = o->next) {
      if (strcmp(o->opt->name, "-I") == 0) {
        err |= fprintf(fh, "#include <%s>\n", o->opt->arg);
      }
    }
  }
  fprintf(fh, "\n");

  return err;
}

int check_types(int ac, char *av[]) {
  int err = 0;
  for (int i = 0; i != ac; i += 1) {
    const char *p = av[i];
    if ((p = strchr(av[i], ':')) != 0) {
      // we found :, now check position
      if (p - av[i] == 0) {
        // : is in first position, so alias is empty string
        printf("error: invalid type '%s', empty type alias\n", av[i]);
        err |= 1;
      } else if ((p - av[i]) == (long)strlen(av[i]) - 1) {
        // : is at last position, type is empty
        printf("error: invalid type '%s', empty type\n", av[i]);
        err |= 1;
      }
    }
  }

  return err;
}

int do_stuff(parsed_opts *opts) {
  FILE *fh = 0;
  FILE *fc = 0;
  int ac = str_array_len((const char **)opts->files);
  char **av = opts->files;
  const opt *opt = has_opt(opts, "--out");

  // first check if all types are valids
  int err = 0;
  if ((err = check_types(ac, av)) != 0) {
    return err;
  }

  if ((fh = _open(opt->arg, hout)) == 0 || (fc = _open(opt->arg, cout)) == 0) {
    return 1;
  }

  // make split_ty
  split_ty sts[ac];
  for (int i = 0; i != ac; i +=1) {
    sts[i] = split_ty_make(av[i]);
  }

  fprintf(fh, "/* this is a generated file, do not edit. */\n\n");
  fprintf(fc, "/* this is a generated file, do not edit. */\n\n");

  // include guards
  fprintf(fh, "#ifndef KIWI_H\n#define KIWI_H\n\n");
  write_includes(fh, fc, opts);
  write_structs_decl(fh, fc, ac, sts);
  write_make(fh, fc, ac, sts);
  write_drop(fh, fc, ac, sts);
  write_append(fh, fc, ac, sts);
  write_copy(fh, fc, ac, sts);
  write_map(fh, fc, ac, sts);
  write_mapi(fh, fc, ac, sts);
  write_iter(fh, fc, ac, sts);
  write_iteri(fh, fc, ac, sts);
  write_any(fh, fc, ac, sts);
  write_all(fh, fc, ac, sts);
  write_rev(fh, fc, ac, sts);
  write_find(fh, fc, ac, sts);
  write_filter(fh, fc, ac, sts);
  // endif include guards
  fprintf(fh, "#endif // KIWI_H\n");

  // remove change from split_ty
  for (int i = 0; i != ac; i +=1) {
    split_ty_reset(sts[i]);
  }

  fclose(fh);
  fclose(fc);

  return 0;
}

int check_opts(parsed_opts *opts) {
  int err = opts->success ? -1 : 1;
  if (has_opt(opts, "--help")) {
    print_help();
    err = 0;
    goto exit;
  }
  if (!has_all_required_opt(opts)) {
    err = 1;
  }
  if (str_array_len((const char**)opts->files) == 0) {
    printf("error: no input types\n");
    err = 1;
  }

 exit:
  return err;
}

int main(int ac, char *av[]) {
  parsed_opts *opts = parse_opts(ac, av);
  int err = check_opts(opts);
  if (err != -1) {
    return err;
  }
  err = do_stuff(opts);
  parsed_opts_free(opts);
  return err;
}

unsigned int str_array_len(const char **arr) {
  unsigned int i = 0;
  while (arr && arr[i]) {
    i += 1;
  }
  return i;
}

char **str_array_append(const char **arr, char *str) {
  unsigned int len = str_array_len(arr) + 2; // return len, we add \0 + new file
  char **new_arr = malloc(sizeof(char*) * len);
  new_arr[len-1] = 0;
  char **out = new_arr;
  while (arr && *arr) {
    *new_arr = malloc(sizeof(char) * (strlen(*arr) + 1));
    strcpy(*new_arr, *arr);
    arr++;new_arr++;
  }
  *new_arr = malloc(sizeof(char) * (strlen(str) + 1));
  strcpy(*new_arr, str);
  return out;
}

void str_array_free(char **arr) {
  char **_arr = arr;
  while (arr && *arr) {
    free(*arr);
    *arr = 0;
    arr++;
  }
  free(_arr);
  _arr = 0;
}

opt *opt_make(const char *name, const char *arg);
void opt_free();
opt_list *opt_list_make();
opt_list *opt_list_append(opt_list *lst, opt *o);
void opt_list_free(opt_list*);

#define OPTS_SIZE 3

static const opt_def opts[OPTS_SIZE] = {
  {
    .name = "-I",
    .summary = "Add file to generate includes",
    .optional = true,
    .has_param = true,
  },
  {
    .name = "--out",
    .summary = "Output folder for kiwi generated files",
    .optional = false,
    .has_param = true,
  },
  {
    .name = "--help",
    .summary = "Display available options",
    .optional = true,
    .has_param = false,
  },
};

void print_help() {
  printf("Usage: kiwi [options] type...\n");
  printf("Options:\n");
  for (int i = 0; i < OPTS_SIZE; i+=1) {
    printf("  %-10s %s\n", opts[i].name, opts[i].summary);
  }
}

char *extract_opt_arg(const char *raw_opt) {
  char *space_pos = strchr(raw_opt, '=');
  char *arg = 0;

  if (space_pos) {
    arg = malloc(sizeof(char) * ((strlen(raw_opt) - (space_pos - raw_opt))));
    strcpy(arg, space_pos+1);
  }

  return arg;
}

char *extract_opt(const char *raw_opt) {
  char *space_pos = strchr(raw_opt, '=');
  char *opt = 0;

  if (space_pos) {
    // the string contains = so need to split the string
    opt = malloc(sizeof(char) * ((space_pos - raw_opt) + 1));
    strncpy(opt, raw_opt, (space_pos - raw_opt));
    opt[(space_pos - raw_opt)] = '\0';
  } else {
    opt = malloc(sizeof(char) * (strlen(raw_opt) + 1));
    strcpy(opt, raw_opt);
  }

  return opt;
}

const opt_def *match_opt_to_def(const char *opt) {
  for (int i = 0; i < OPTS_SIZE; i += 1) {
    if (strcmp(opts[i].name, opt) == 0) {
      return &opts[i];
    }
  }
  return 0;
}

parsed_opts *parse_opts(int args, char *argv[]) {
  parsed_opts *opts = malloc(sizeof(parsed_opts));
  char **files = 0;
  opts->opts = 0;
  opts->success = true;

  int i = 1;// first argument is bin name
  while (i < args) {
    char *current_opt = 0;
    char *current_opt_arg = 0;
    const opt_def *def = 0;

    // get opt
    current_opt = extract_opt(argv[i]);

    // is it a valid opt ?
    def = match_opt_to_def(current_opt);
    // unknown option
    if (!def && current_opt[0] == '-') {
      printf("warning: unrecognized command line option '%s'\n", current_opt);
    } else if (!def) { // type
      char **_files = str_array_append((const char**)files, current_opt);
      str_array_free(files);
      files = _files;
    } else {

      // if needed try to get opt argument
      if (def->has_param) {
        // maybe specified with =
        current_opt_arg = extract_opt_arg(argv[i]);
        if (!current_opt_arg) {
          if ((i+1) < args) {
            current_opt_arg = malloc(sizeof(char) * (strlen(argv[i+1]) + 1));
            strcpy(current_opt_arg, argv[i+1]);
            i += 1;
          } else {
            opts->success = false;
            printf("error: missing argument to '%s'\n", current_opt);
          }
        } else if (strlen(current_opt_arg) == 0) {
          opts->success = false;
          printf("error: missing argument to '%s'\n", current_opt);
        }
      }

      opt *o = opt_make(current_opt, current_opt_arg);
      opts->opts = opt_list_append(opts->opts, o);
      free(current_opt_arg);
    }
    free(current_opt);
    i += 1;
  }

  opts->files = files;
  return opts;
}

const opt *has_opt(parsed_opts *opts, char *str) {
  if (opts) {
    opt_list *iter = opts->opts;
    while (iter) {
      if (strcmp(iter->opt->name, str) == 0) {
        return iter->opt;
      }
      iter = iter->next;
    }
  }
  return 0;
}

bool has_all_required_opt(parsed_opts *op) {
  for (int i = 0; i < OPTS_SIZE; i += 1) {
    if (opts[i].optional == false) {
      if (has_opt(op, opts[i].name) == 0) {
        printf("error: missing required argument '%s'\n", opts[i].name);
        return false;
      }
    }
  }
  return true;
}

void parsed_opts_free(parsed_opts *p) {
  if (p) {
    opt_list_free(p->opts);
    str_array_free(p->files);
    free(p);
  }
}

opt *opt_make(const char *name, const char *arg) {
  opt *o = malloc(sizeof(opt_list));
  o->name = 0;
  o->arg = 0;
  o->name = malloc(sizeof(char) * (strlen(name) + 1));
  strcpy(o->name, name);
  // not mandatory
  if (arg) {
    o->arg = malloc(sizeof(char) * (strlen(arg) + 1));
    strcpy(o->arg, arg);
  }
  return o;
}

void opt_free(opt *o) {
  if (o) {
    free(o->name);
    free(o->arg);
    free(o);
  }
}

opt_list *opt_list_make() {
  opt_list *lst = malloc(sizeof(opt_list));
  lst->opt = 0;
  lst->next = 0;
  return lst;
}

opt_list *opt_list_append(opt_list *lst, opt *e) {
  opt_list *ret = lst;

  if (!lst) {
    ret = opt_list_make();
    ret->opt = e;
  } else {
    while (lst->next) {
      lst = lst->next;
    }
    lst->next = opt_list_make();
    lst->next->opt = e;
  }

  return ret;
}

void opt_list_free(opt_list *lst) {
  opt_list *next = 0;
  opt_list *head = lst;
  while (head) {
    next = head->next;
    opt_free(head->opt);
    free(head);
    head = next;
  }
}
