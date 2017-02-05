# kiwi
Generate statically typed list for C

Kiwi is a simple tool to generate user defined statically typed list for C.

Kiwi is developed using only C11 standard feature and without any external dependencies so you can easily build it on any platform using a modern enough C compiler.
Also it is well integrated with CMake so you can add kiwi in a few line of code to your CMake build.

# usage
Kiwi accepts those differents parameters:
* `--out=OUT_DIR`: the output path for C, headers generated files (mandatory).
* `-I=FILE_NAME`: filename to include in the generated file, used with user defined types (optional)
* types: any other parameters represent you want to create a list for, those types accept two different syntax:
 * `int`: which just generate a list implementation for the type `int`
 * `"str:char *"`: which produce a list implementation for the type `char *`, using `str` as an alias in generated code naming.

Simple usage example:
```Shell
kiwi --out=./kiwi_gen -I=toto.h int "str:char *" toto_t
```
Executing this command will generate list definitions for `int, char *, toto_t`, in the output folder `./kiwi_gen`, using the header file `toto.h` to define the type `toto_t`.

# features
Here is a list of functions generate with each lists types:
* make
* drop
* append
* copy
* map
* mapi
* iter
* iteri
* filter
* any
* all
* rev
* find

Kiwi use the '_Generic' macro from C11 extensively, so it allows you to use the same API for any types, it means that you do not have to call verbose API like `int_list_map_float`, without losing typeness by using `void*` to a single function.

Example:

```C
#include <kiwi.h>

float int_to_float(const int *i) {/*...*/}
int int_to_int(const int *i) {/*...*/}
float float_float(const float *i) {/*...*/}

int main() {
    int_list *li = make(_int_list);
    float_list *lf = make(_float_list);
    // ... initialize your lists, do stuff with them
    int_list *li2 = map(li, int_to_int);
    float_list *lf2 = map(li, int_to_float);
    float_list *lf3 = map(li, float_to_float);
    // drop you lists
    drop(li);
    return 0;
}
```

# build
Kiwi requires only a C11 compiler and CMake to build.
```Shell
git clone https://github.com/jeremyletang/kiwi.git && cd kiwi
mkdir build && cmake .. && make
```
Kiwi should be available in the build/bin directory.

# integrate kiwi in your project.
You can easily integrate kiwi to your C project, a CMake module is available at the root of the repository in the CMake folder.
You just need to include it in your project configuration, and this will allow you to call kiwi during the build of your own project and generate the lists you need.
You can find a basic CMakeLists.txt configuration to integrate kiwi in the example folder.