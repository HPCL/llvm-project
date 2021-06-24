//===--- MacroNameCheck.cpp - clang-tidy ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MacroNameCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/Decl.h" 
#include "clang/AST/DeclarationName.h"
#include "clang/AST/Type.h"
#include "clang/Lex/Preprocessor.h"

using namespace clang::ast_matchers;
using namespace llvm; 

namespace clang {
namespace tidy {
namespace petsc {

std::string validPetscMacroName(){
  std::string NameMatcher = "[A-Z]*" + std::string("[A-Z][a-zA-Z0-9]*"); 
  return std::string("::(") + NameMatcher + ")$"; 
}

void MacroNameCheck::registerPPCallbacks(const SourceManager &SM, Preprocessor *PP, 
                                        Preprocessor *ModuleExpanderPP){
    IdentifierTable& id_table = PP -> getIdentifierTable(); 
    Module* module = PP -> getCurrentLexerSubmodule(); 
    for (auto entryP = id_table.begin(); entryP != id_table.end(); entryP++)
    {
        StringRef val_ref = entryP -> getKey();
        IdentifierInfo &val_info = id_table.get(val_ref); 
        const IdentifierInfo *val_info_const = &val_info;
        std::string val = val_ref.str(); 
        if (PP -> isMacroDefinedInLocalModule(val_info_const, module))
        {
            outs() << val << "\n";
        }
        
         
    }
    
}

} // namespace petsc
} // namespace tidy
} // namespace clang
