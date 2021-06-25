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

std::vector<std::string> split_string(std::string str, char to_split_by){
    std::stringstream str_stream(str);
    std::string segment; 
    std::vector<std::string> to_return; 
    while (std::getline(str_stream, segment, to_split_by))
    {
        to_return.push_back(segment);
    }
    return to_return; 
    
}

class DefinesCallBack : public PPCallbacks
{
private:
    Preprocessor *PP;
    const SourceManager &SM; 
public:
    DefinesCallBack(Preprocessor *PP, const SourceManager &SM);
    void MacroDefined(const Token &MacroNameTok, const MacroDirective *MD) override; 
};

DefinesCallBack::DefinesCallBack(Preprocessor *PP, const SourceManager &SM) : PP(PP), SM(SM)
{
}

void DefinesCallBack::MacroDefined(const Token &MacroNameTok, const MacroDirective *MD)
{
    const LangOptions &LO = this -> PP -> getLangOpts(); 
    const MacroInfo* macro_i = MD -> getMacroInfo(); 
    SourceLocation begin_loc = macro_i -> getDefinitionLoc(); 
    SourceLocation end_loc   = macro_i -> getDefinitionEndLoc(); 
    SourceRange def_range(begin_loc, end_loc); 
    CharSourceRange charRange = Lexer::getAsCharRange(def_range, this -> SM, LO); 
    charRange.setEnd(charRange.getEnd().getLocWithOffset(1)); 
    std::string text = Lexer::getSourceText(charRange, this -> SM, LO).str(); 
    if (!(macro_i -> isBuiltinMacro() || MD -> isFromPCH()))
    {
        if (!macro_i -> isFunctionLike())
        {
            if (macro_i -> isObjectLike())
            {
                std::vector<std::string> vec = split_string(text, ' '); 
                std::string front = vec[0];
                outs() << front << "\n";
            }
        } 
    }
}



std::string validPetscMacroName(){
  std::string NameMatcher = "[A-Z]*" + std::string("[A-Z][a-zA-Z0-9]*"); 
  return std::string("::(") + NameMatcher + ")$"; 
}

void MacroNameCheck::registerPPCallbacks(const SourceManager &SM, Preprocessor *PP, 
                                        Preprocessor *ModuleExpanderPP){
    DefinesCallBack dd(PP, SM);
    PP -> addPPCallbacks(std::make_unique<DefinesCallBack>(dd));
    
}




} // namespace petsc
} // namespace tidy
} // namespace clang
