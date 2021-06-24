//===--- PrivateFunctionNameCheck.cpp - clang-tidy ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "PrivateFunctionNameCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/Decl.h" 
#include "clang/AST/DeclarationName.h"
#include "clang/AST/Type.h"

using namespace clang::ast_matchers;
using namespace llvm; 

namespace clang {
namespace tidy {
namespace petsc {


std::string validPetscPrivateFunctionName(){
  std::string NameMatcher =  std::string("([A-Z][a-zA-Z0-9]+)_([A-Z][A-Za-z0-9]*)"); 
  return std::string("::(") + NameMatcher + ")$";
}

void PrivateFunctionNameCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(functionDecl(
    unless(matchesName(validPetscPrivateFunctionName()))
  ).bind("function"), this); 
}

void PrivateFunctionNameCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl  = Result.Nodes.getNodeAs<FunctionDecl>("function");
  if (!MatchedDecl)
    return;

  diag(MatchedDecl -> getLocation(), "Functions that are private to PETSc "
                                     "Either have an appended _PRIVATE "
                                     "or have appended _Subtype (for example: MatMultSeq_AIJ) "
                                    ) << MatchedDecl; 
}

} // namespace petsc
} // namespace tidy
} // namespace clang
