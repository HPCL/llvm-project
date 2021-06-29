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

Regex 
validPetscMacroName(){
  std::string NameMatcher     = std::string("[A-Z][A-Z0-9_]+$"); 
  const std::string to_return = //"::(" + 
                                NameMatcher 
                                //+ ")$"
                                ; 
  StringRef to_return_ref(to_return); 
  return Regex(to_return_ref); 

}

Regex 
validPetscFunctionMacroName(){
    std::string NameMatcher = "[A-Z][A-Za-z0-9_]+$" ; 
    const std::string to_return = // std::string("::(") + 
                                  NameMatcher 
                                // + ")$"
                                ;  
    StringRef to_return_ref(to_return); 
    return Regex(to_return_ref); 
}

std::vector<std::string> 
split_string(std::string str, char to_split_by){
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
    ClangTidyCheck *CHECK;
public:
    DefinesCallBack(Preprocessor *PP, const SourceManager &SM
                                    , ClangTidyCheck *CHECK);
    void MacroDefined(const Token &MacroNameTok
                    , const MacroDirective *MD) override; 
};

DefinesCallBack::DefinesCallBack(Preprocessor *PP, const SourceManager &SM
                                                 , ClangTidyCheck *CHECK) 
                : PP(PP), SM(SM), CHECK(CHECK)
{
}

void 
DefinesCallBack::MacroDefined(const Token &MacroNameTok
                            , const MacroDirective *MD)
{
    Regex petsc_macro_regex = validPetscMacroName();
    Regex petsc_macro_function_regex = validPetscFunctionMacroName(); 
    const LangOptions &LO = this -> PP -> getLangOpts(); 
    const MacroInfo* macro_i = MD -> getMacroInfo(); 
    SourceLocation begin_loc = macro_i -> getDefinitionLoc(); 
    SourceLocation end_loc   = macro_i -> getDefinitionEndLoc(); 
    SourceRange def_range(begin_loc, end_loc); 
    CharSourceRange charRange = Lexer::getAsCharRange(def_range, this -> SM
                                                               , LO); 
    charRange.setEnd(charRange.getEnd().getLocWithOffset(1)); 
    std::string text = Lexer::getSourceText(charRange, this -> SM
                                                     , LO).str(); 
    if (!(macro_i -> isBuiltinMacro() 
          || MD -> isFromPCH()
          || macro_i -> isUsedForHeaderGuard()
          || macro_i -> isC99Varargs() 
          || macro_i -> isGNUVarargs()
          || this -> SM.isInSystemMacro(begin_loc) 
          || this -> SM.isInSystemHeader(begin_loc) 
          || this -> SM.isInExternCSystemHeader(begin_loc)
          )
        )
    {
        if (!macro_i -> isFunctionLike())
        {
            std::vector<std::string> vec = split_string(text, ' '); 
            std::string front = vec[0];
            std::string snd; 
            if (vec.size() > 1)
            {
                std::string snd   = vec[1];
            } 
            front.erase(remove_if(front.begin(), front.end(), isspace), front.end());
            if (!petsc_macro_regex.match(StringRef(front)))
            { 
                if (begin_loc.isValid())
                {
                    this -> CHECK -> diag(begin_loc, "All Petsc Macro variables "
                                                 "are named with all capital "
                                                 "letters. When they consist "
                                                 "of several complete words "
                                                 ", there is an underscore "
                                                 "between each word. For "
                                                 "example *MAT_FINAL_ASSEMBLY*"); 
                }else{
                    outs() << front << ": This macro violates PETsc's naming " 
                                       "convention for function-like " 
                                       " macros. In addition, the preprocessor deemed invalid " 
                                       "location where it is defined valid. \n";
                }
            }
        }else
        {
            std::vector<std::string> vec = split_string(text, '('); 
            std::string front = vec[0]; 
            front.erase(remove_if(front.begin(), front.end(), isspace), front.end());
            if (!petsc_macro_function_regex.match(StringRef(front)))  // put back !
            {

                outs() << front << " (function-like) \n"; 
                this -> CHECK -> diag(begin_loc, "All Petsc Macro variables "
                                                 "are named with all capital "
                                                 "letters. When they consist "
                                                 "of several complete words "
                                                 ", there is an underscore "
                                                 "between each word. For "
                                                 "example *MAT_FINAL_ASSEMBLY*");
            }
        }
         
    }
}

void 
MacroNameCheck::registerPPCallbacks(const SourceManager &SM, Preprocessor *PP, 
                                        Preprocessor *ModuleExpanderPP){
    DefinesCallBack dd(PP, SM, this);
    PP -> addPPCallbacks(std::make_unique<DefinesCallBack>(dd));
    
}




} // namespace petsc
} // namespace tidy
} // namespace clang
