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
#include <iostream>
#include <fstream>

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
    std::ofstream &stats_file; 
public:
    static int faults_counter;
    IfDefRuleCallBack(std::ofstream &stats_file, Preprocessor *PP
                    , const SourceManager &SM
                    , ClangTidyCheck *CHECK);
    virtual void Ifdef(SourceLocation Loc
                    , const Token &MacroNameTok 
                    , const MacroDefinition &MD) override; 

    virtual void Elifdef(SourceLocation Loc
                    , const Token &MacroNameTok 
                    , const MacroDefinition &MD) override;

    virtual void Ifndef(SourceLocation Loc 
                    , const Token &MacroNameTok 
                    , const MacroDefinition &MD) override; 

    virtual void Elifndef(SourceLocation Loc 
                    , const Token &MacroNameTok 
                    , const MacroDefinition &MD) override; 
};

IfDefRuleCallBack::IfDefRuleCallBack(std::ofstream &stats_file, 
                                    Preprocessor *PP, const SourceManager &SM
                                    , ClangTidyCheck *CHECK) 
                : PP(PP), SM(SM), CHECK(CHECK), stats_file(stats_file)
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
    faults_counter++;
    // this -> stats_file << "Ifdef, " << faults_counter << "\n"; 
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
    faults_counter++;
    // this -> stats_file << "Elifdef, " << faults_counter << "\n";
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
    faults_counter++;
    // this -> stats_file << "Ifndef, " << faults_counter << "\n";

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
    faults_counter++;
    // this -> stats_file << "Elifndef, " << faults_counter << "\n";

}

int IfDefRuleCallBack::faults_counter = 0;
void 
IfDefRuleCheck::registerPPCallbacks(const SourceManager &SM, Preprocessor *PP, 
                                        Preprocessor *ModuleExpanderPP){
    std::ofstream stats_file_stream("IfDefStats.csv");
    IfDefRuleCallBack dd(stats_file_stream, PP, SM, this);  
    PP -> addPPCallbacks(std::make_unique<IfDefRuleCallBack>(dd));
    
}

} // namespace petsc
} // namespace tidy
} // namespace clang
