Developing GoldenDict is not hard.

If you know some C++ and optionally some Qt, you can start to modify GoldenDict right now:

* Install Qt and QtCreator
* (On Linux, install dependencies)
* Load `goldendict.pro`
* Modify some code
* Hit the `Run`.

A CMake build script is also provided `CMakeLists.txt` is provided which can be used directly in other IDEs like CLion or Visual Studio 2022.

## Coding Standards

Please follow [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) and write modern C++ code.

Commit messages should follow [Conventional Commits](https://www.conventionalcommits.org)

Reformat changes with `clang-format` [how to use clang-format](https://github.com/xiaoyifang/goldendict/blob/staged/howto/how%20to%20use%20.clang-format%20to%20format%20the%20code.md)

Remember to enable `clang-tidy` support on your editor so that `.clang-tidy` will be respected.