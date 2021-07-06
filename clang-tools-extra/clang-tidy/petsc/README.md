Based on [this style guide](https://petsc.org/release/developers/style/), the following checks are supported: 
    * `petsc-function-name-check` for checking whether function names are using camel casing as suggested in the documentations
    * `petsc-private-function-name-check` for functions private to PETSc
    * `petsc-macro-name-check` for macros defined in PETSc
    * `petsc-if-stmt-rule-check` for the structure of if statements as dictated by the guide.
    * `petsc-function-impl-name-check` for actual function implementations usually found the `impl` directories
    * `petsc-function-in-struct-name-check` for function pointers (prototypes) defined in the structs meant to emulate abstract classes.
    * `petsc-enum-ty-name-check` For enum types naming conventions
    * `petsc-enum-elem-name-check` For naming conventions of enum elements 

After building and installing `clang-tidy` (to do so, make sure `clang-tools-extra` is provided in the list of projects to be built when `cmake`'s `-DLLVM_ENABLE_PROJECTS` is provided a list of projects to build ), any of the above checks can then be run so: 
```
clang-tidy -checks=-*,petsc-macro-name-check <path-to-petsc-header-or-source-file>
```

This above will disable all other clang-tidy checks and only enable `petsc-macro-name-check`. This is will work provided a "compilation database" has been generated and is stored at the root directory of the project. This allowed clang-tidy to find the appropriate compilation instruction for the source file.

For any of these checks, if a violation of the style guide is encountered, an appropriate warning message will be reported by clang-tidy. Counters that will add up these mistakes and export appropriate statistics to csv files are yet to be implemented.