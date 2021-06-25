//===--- FunctionImplNameCheck.cpp - clang-tidy ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "FunctionImplNameCheck.h"
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


std::string validPetscFunctionImplName(){
  std::string NameMatcher =  std::string("[A-Z][A-Za-z0-9]+_[A-Za-z]+"); 
  return std::string("::(") + NameMatcher + ")$";
}

void FunctionImplNameCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(functionDecl(
    unless(matchesName(validPetscFunctionImplName()))
  ).bind("function"), this); 
}

void FunctionImplNameCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl  = Result.Nodes.getNodeAs<FunctionDecl>("function");
  if (!MatchedDecl)
    return;

  diag(MatchedDecl -> getLocation(), "Names of implementations of class functions "
                                     "should begin with the function name, an "
                                     "underscore, and the name of the implementation, "
                                     "for example, *KSPSolve_GMRES()*"
                                    ); 
}

} // namespace petsc
} // namespace tidy
} // namespace clang
