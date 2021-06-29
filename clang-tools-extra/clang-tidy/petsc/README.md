Based on [this style guide](https://petsc.org/release/developers/style/), the following checks are supported: 
    * `petsc-function-name-check` for checking whether function names are using camel casing as suggested in the documentations
    * `petsc-private-function-name-check`
    * `petsc-macro-name-check`
    * `petsc-if-stmt-rule-check`
    * `petsc-function-impl-name-check`
    * `petsc-function-in-struct-name-check`
    * `petsc-enum-ty-name-check`
    * `petsc-enum-elem-name-check`

After building and installing `clang-tidy`, any of the above checks can then be run so: 
```
clang-tidy -checks=-*,petsc-macro-name-check <path-to-petsc-header-or-source-file>
```

This above will disable all other clang-tidy checks and only enable `petsc-macro-name-check`

For any of these checks, if a violation of the style guide is encountered, an appropriate warning message will be reported by clang-tidy. Counters that will add up these mistakes and export appropriate statistics to csv files are yet to be implemented.