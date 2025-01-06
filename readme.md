# SBCS
## Summary
SBCS is a simple build system for programs written in C. SBCS is a small library with the only dependency being the C89 standard library. This library was designed in such a way that any C programmer can write a small build script in C, a language that they are already familiar with, to build a larger program on almost any system. This is done for a variety of reasons:
- C is much faster and direct than other configuration languages such as CMake, makefiles, or shell scripts.
- C has much greater library support, allowing build systems to take advantage of the incredible C ecosystem.
- C programmers are already intimately familiar with C, so using it to build a program has almost no startup cost.
The only thing you have to do to use this library is by creating a C program (I usually name them `build.c`), and including sbcs.h at the top of the file. Make sure to compile the build program and `sbcs.c` and link them together.

## How To Use
SBCS operates on the principle of modules. Each module stores a list of all of their files and dependencies. The first step in an SBCS build script is to create your modules. Creating a module is as simple as calling `Module_new()` and supplying a name for that module. When a module is compiled, each object file will contain the name of the module and file that the object is compiled from, allowing you to easily track down errors. Once you are done with a module, you can free it with `Module_free()`.

Once you have created you modules, you need to add content to them. You can tell modules what files to track with `Module_addFile()` and dependencies with `Module_addDependency()`. Be careful, as once you have added a file or dependency to a module you cannot remove it. This is done both to keep the library simple, and since most build configurations are linear this rarely becomes an issue in practice.

The last step is producing an executable. Once all of your modules have been created and their files and dependencies have been tracked, it is time for compilation. The function `Module_compile()` is used to compile a module and all of its dependencies into their respective object files. To combine all of the object files produced by `Module_compile()`, call the function `Module_link()` with the same module, and supply an executable name. This will link all of the object files together to produce a final executable.

After reading these short few paragraphs, you now know everything you need to know to create a project. Who knew that such a powerful build system could be so simple.

## Future Features
- [ ] Forcing specific versions of C.
- [ ] Forcing specific features to be used on particular compilers.
- [ ] Getting information about the particular system and compiler.
- [ ] Optional multi-threaded compiler calls.
- [ ] Generalized compiler options for speed and memory optimization.

