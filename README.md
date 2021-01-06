<p align="center">
  <img src="https://img.shields.io/badge/License-MIT-blue.svg" alt="License: MIT">
  <a href="https://github.com/Smertig/rcmp/actions"><img src="https://github.com/Smertig/rcmp/workflows/Tagged%20Release/badge.svg" alt="GitHub Actions"></a>
  <a href="https://github.com/Smertig/rcmp/actions"><img src="https://github.com/Smertig/rcmp/workflows/Build%20On%20Push/badge.svg" alt="GitHub Actions"></a>
  <a href="https://travis-ci.org/Smertig/rcmp"><img src="https://travis-ci.org/Smertig/rcmp.svg?branch=master" alt="Travis CI"></a>
</p>

<p align="center">
  <b>rcmp</b> - C++17, multi-architecture cross-platform hooking library with clean API.
</p>

## Features

- Intuitive, modern, compiler/platform-independent API
- **x86/x86-64 support** (more soon)
- **Windows/Linux support**
- Calling convention support (`cdecl`, `stdcall`, `thiscall`, `fastcall`, `native-x64`)

## Building

### With CMake (as a subproject)

Clone repository to subfolder and link `rcmp` to your project:
```cmake
add_subdirectory(path/to/rcmp)
target_link_libraries(your-project-name PRIVATE rcmp)
```

## Examples

- The most common case: hook function to modify its argument and/or result 
```c++
int foo(float arg) { /* body */ }

rcmp::hook_function<&foo>([](auto original_foo, float arg) {
    return original_foo(arg * 2) + 1;
});
```
- However, in most cases you probably want to hook function **knowing only its address and signature** (in fact, that's everything you need to make hook)
```c++
rcmp::hook_function<0xDEADBEEF, int(float)>([](auto original_foo, float arg) {
    return original_foo(arg * 2) + 1;
});
```

- Trace function calls
```c++
void do_something(int id, const char* action) { /* body */ }

rcmp::hook_function<&do_something>([](auto original_function, int id, const char* action) {
    std::cout << "do_something(" << id << ", " << action << ") called..\n";
    original_function(id, action);
});
```

- Replace return value

```c++
bool check_license() { /* body */ }

rcmp::hook_function<&check_license>([](auto /* original_function */) {
    return true;
});
``` 

- Accept arguments as a variadic pack
```c++
template <class... Args> void print(const Args& ...) { /* implementation */ }

rcmp::hook_function<0xDEADBEEF, unsigned int(int, float, bool, double, void*, long)>([](auto original, auto... args) {
    print("args are: ", args...);
    return original(args...);
});
```

- Function address can be known at runtime or compile-time
```c++
// compile-time address
rcmp::hook_function<0xDEADBEEF, int(int)>([](auto original, int arg) { ... });

// runtime address (i.e. from GetProcAddress/dlopen)
rcmp::hook_function<int(int)>(0xDEADBEEF, [](auto original, int arg) { ... });
```

- Calling convention support 
```c++
/// x86, the following calls are synonyms

// good:
rcmp::hook_function<void(int)>(...);                // default convention
rcmp::hook_function<void(*)(int)>(...);             // default convention, but 3 more symbols
rcmp::hook_function<rcmp::cdecl_t<void(int)>>(...); // explicit convention (rcmp::cdecl_t<S> is an alias for rcmp::generic_signature_t<S, rcmp::cconv::cdecl_>)

// bad, compiler-specific
rcmp::hook_function<void(__cdecl*)(int)>(...);               // MSVC
rcmp::hook_function<void(__attributes((cdecl))*)(int)>(...); // gcc/clang

// x86 supported conventions
rcmp::cdecl_t   <void(int)> // same as rcmp::generic_signature_t<void(int), rcmp::cconv::cdecl_>
rcmp::stdcall_t <void(int)> // same as rcmp::generic_signature_t<void(int), rcmp::cconv::stdcall_>
rcmp::thiscall_t<void(int)> // same as rcmp::generic_signature_t<void(int), rcmp::cconv::thiscall_>
rcmp::fastcall_t<void(int)> // same as rcmp::generic_signature_t<void(int), rcmp::cconv::fastcall_>

// x64
rcmp::hook_function<void(int)>(...);                                                     // default convention
rcmp::hook_function<void(*)(int)>(...);                                                  // default convention, but more letters
rcmp::hook_function<rcmp::generic_signature_t<void(int), rcmp::cconv::native_x64>>(...); // explicit convention

```

- VTable hooking (`hook_indirect_function`)
```c++
// Let's assume:
// 5             - index of function in vtable
// int A::f(int) - function signature
using signature_t = rcmp::thiscall_t<int(A*, int)>; // x86, MSVC
using signature_t = rcmp::cdecl_t<int(A*, int)>;    // x86, gcc/clang
using signature_t = int(A*, int);                   // x64

// vtable address can be known at compile-time (0xDEADBEEF)
rcmp::hook_indirect_function<0xDEADBEEF + 5 * sizeof(void*), signature_t>([](auto original, A* self, int arg) { ... });

// ..or at runtime
rcmp::hook_indirect_function<signature_t>(get_vtable_address() + 5 * sizeof(void*), [](auto original, A* self, int arg) { ... });
```

## Motivation

Why *yet another* hooking library?

There are too many libraries with similar or even more powerful features. Most of them have been perfectly designed; however, they don't provide all the features I need.

### Mods, cheats, plugins etc 

I like to develop unofficial mods (plugins) ~~and cheats~~ for various games (both for client and server-side).
This is a very specific area of development that requires continuous experimentation with function hooking. 

Suppose you want to hook a function. All you need to know about this function - its address and signature, that's it.
Just write an interceptor (replacement function) and call something like `cool_lib::hook_function`. Very simple, isn't it? Of course it's not.
Most libraries require:
- Create global variable, that holds pointer to replaced original function.
- Write global function, that contains interceptor logic.
- Write same function signature/name multiple times ([DRY](https://ru.wikipedia.org/wiki/Don%E2%80%99t_repeat_yourself)).
- Use C-style casts or MACRO to call original function from the interceptor.
- Manually create and destroy auxiliary context (i.e. disassembler backend) required for hooking.

That's really annoying. **I want to express my intentions in a single expression without boilerplate, code repetitions and ugly C-style code**. 

So I ended up with developing my own library - `rcmp`.

### Cross-platform and compiler support

At work I need both windows (`.dll`) and linux (`.so`) support, but most libraries aren't cross-platform (some of them also use compiler-specific extensions, that's not portable). 
Moreover, there are [moddable games for Oculus Quest VR](https://beatsaberquest.com/bmbf/bmbf-mods/bmbf-mods/) that works on ARM64 architecture. 
`rcmp` was designed to be easily extendable, so I was able to use it even for Android apps (ARM64 support comes soon!).

### Modern C++ and canonical code

Most libraries use _not-so-modern_ C++ standards (C++11 and below), so they have limited capabilities.
Modern C++ features allow developer to write compact and type-safe code without boilerplate and repetitions (especially in case of hooking).
Due to C++17, `rcmp` has convenient API as well as minimalistic and readable implementation. 
 
### Dependencies

`rcmp` has single-header bundled lightweight dependency - [nmd](https://github.com/Nomade040/nmd) by [Nomade040](https://github.com/Nomade040) (only as a length disassembler for x86/x86-64). 
Most of hooking libraries depend on big, verbose or even deprecated frameworks.

### Build & Install

`rcmp` can be easily added to any cmake-based project. No external requirements or dependencies, no installation or manual non-trivial actions to build - just add two lines in `CMakeLists.txt`. 

## Missing features

- No documentation (yet)
- No way to disable hook
- No ellipsis (`...`) support


## References

- [catch2](https://github.com/catchorg/Catch2) for unit-testing
- [nmd library](https://github.com/Nomade040/nmd) for x86/x86-64 length disassembly
- [x86](http://ref.x86asm.net/coder32.html) and [x86-64](http://ref.x86asm.net/coder64.html) opcode and instruction reference

## License

- MIT
