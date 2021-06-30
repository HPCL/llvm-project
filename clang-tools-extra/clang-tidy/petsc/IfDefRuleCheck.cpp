//===--- IfDefRuleCheck.cpp - clang-tidy ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "IfDefRuleCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/MemoryBuffer.h"
#include "clang/AST/Decl.h" 
#include "clang/AST/DeclarationName.h"
#include "clang/AST/Type.h"
#include "llvm/ADT/STLExtras.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/Lexer.h"
#include <memory>
#include <type_traits>
#include <string>
#include <vector>
#include <sstream>

using namespace clang::ast_matchers;
using namespace llvm; 


namespace clang {
namespace tidy {
namespace petsc {

class IfDefRuleCallBack : public PPCallbacks
{
private:
    Preprocessor *PP;
    const SourceManager &SM; 
    ClangTidyCheck *CHECK;
public:
    IfDefRuleCallBack(Preprocessor *PP, const SourceManager &SM
                                    , ClangTidyCheck *CHECK);
    void Ifdef(SourceLocation Loc
                    , const Token &MacroNameTok 
                    , const MacroDefinition &MD) override; 

    void Elifdef(SourceLocation Loc
                    , const Token &MacroNameTok 
                    , const MacroDefinition &MD) override;

    void Ifndef(SourceLocation Loc 
                    , const Token &MacroNameTok 
                    , const MacroDefinition &MD) override; 

    void Elifndef(SourceLocation Loc 
                    , const Token &MacroNameTok 
                    , const MacroDefinition &MD) override;
};

IfDefRuleCallBack::IfDefRuleCallBack(Preprocessor *PP, const SourceManager &SM
                                                 , ClangTidyCheck *CHECK) 
                : PP(PP), SM(SM), CHECK(CHECK)
{
}

void 
IfDefRuleCallBack::Ifdef(SourceLocation Loc 
                        , const Token &MacroNameTok 
                        , const MacroDefinition &MD)
{
    this -> CHECK -> diag(Loc, "Do not use #ifdef or #ifndef. " 
                               "Rather, use #if defined(... or " 
                               "#if !defined(.... Better, use " 
                               "PetscDefined()"); 
}

void 
IfDefRuleCallBack::Elifdef(SourceLocation Loc 
                        , const Token &MacroNameTok 
                        , const MacroDefinition &MD)
{
    this -> CHECK -> diag(Loc, "Do not use #ifdef or #ifndef. " 
                               "Rather, use #if defined(... or " 
                               "#if !defined(.... Better, use " 
                               "PetscDefined()");
}

void 
IfDefRuleCallBack::Ifndef(SourceLocation Loc 
                       , const Token &MacroNameTok 
                       , const MacroDefinition &MD)
{
    this -> CHECK -> diag(Loc, "Do not use #ifdef or #ifndef. " 
                               "Rather, use #if defined(... or " 
                               "#if !defined(.... Better, use " 
                               "PetscDefined()");

}

void 
IfDefRuleCallBack::Elifndef(SourceLocation Loc 
                       , const Token &MacroNameTok 
                       , const MacroDefinition &MD)
{
    this -> CHECK -> diag(Loc, "Do not use #ifdef or #ifndef. " 
                               "Rather, use #if defined(... or " 
                               "#if !defined(.... Better, use " 
                               "PetscDefined()");

}

void 
IfDefRuleCheck::registerPPCallbacks(const SourceManager &SM, Preprocessor *PP, 
                                        Preprocessor *ModuleExpanderPP){
    IfDefRuleCallBack dd(PP, SM, this);
    PP -> addPPCallbacks(std::make_unique<IfDefRuleCallBack>(dd));
    
}




} // namespace petsc
} // namespace tidy
} // namespace clang
