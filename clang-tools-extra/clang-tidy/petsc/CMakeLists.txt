set(LLVM_LINK_COMPONENTS
  FrontendOpenMP
  Support
  )

add_clang_library(clangTidyPetscModule 
  FunctionNameCheck.cpp
  EnumTypeNameCheck.cpp 
  PrivateFunctionNameCheck.cpp 
  MacroNameCheck.cpp 
  EnumElementNameCheck.cpp 
  FunctionInStructNameCheck.cpp 
  FunctionImplNameCheck.cpp 
  IfStatementRuleCheck.cpp 
  IfDefRuleCheck.cpp 
  PetscTidyModule.cpp 
  LINK_LIBS
  clangTidy
  clangTidyUtils

  DEPENDS
  omp_gen
  )

clang_target_link_libraries(clangTidyPetscModule
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
  )