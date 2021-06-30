#include "../ClangTidy.h" 
#include "../ClangTidyModule.h" 
#include "../ClangTidyModuleRegistry.h" 
#include "FunctionNameCheck.h"
#include "PrivateFunctionNameCheck.h" 
#include "StructNameCheck.h" 
#include "MacroNameCheck.h" 
#include "IfStatementRuleCheck.h" 
#include "IfDefRuleCheck.h" 
#include "FunctionInStructNameCheck.h" 
#include "FunctionImplNameCheck.h" 
#include "EnumTypeNameCheck.h" 
#include "EnumElementNameCheck.h"


namespace clang {
namespace tidy {
namespace petsc {

class PetscModule : public ClangTidyModule {
public:
  void addCheckFactories(ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<FunctionNameCheck>("petsc-function-name-check");
    CheckFactories.registerCheck<PrivateFunctionNameCheck>("petsc-private-function-name-check");
    CheckFactories.registerCheck<MacroNameCheck>("petsc-macro-name-check");
    CheckFactories.registerCheck<IfStatementRuleCheck>("petsc-if-stmt-rule-check");
    CheckFactories.registerCheck<IfDefRuleCheck>("petsc-if-def-rule-check");
    CheckFactories.registerCheck<FunctionImplNameCheck>("petsc-function-impl-name-check");
    CheckFactories.registerCheck<FunctionInStructNameCheck>("petsc-function-in-struct-name-check");
    CheckFactories.registerCheck<EnumTypeNameCheck>("petsc-enum-ty-name-check");
    CheckFactories.registerCheck<EnumElementNameCheck>("petsc-enum-elem-name-check");
  }
};

} // namespace petsc 

// Register the MPITidyModule using this statically initialized variable.
static ClangTidyModuleRegistry::Add<petsc::PetscModule>
    X("petsc-module", "Adds Petsc clang-tidy checks.");
// This anchor is used to force the linker to link in the generated object file
// and thus register the PetscModule.
volatile int PetscModuleAnchorSource = 0;

} // namespace tidy
} // namespace clang