//===--- FunctionNameCheck.cpp - clang-tidy ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "FunctionNameCheck.h"
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

std::string validPetscFunctionName(){
  std::string NameMatcher = "[A-Z]*" + std::string("[A-Z][a-zA-Z0-9]*"); 
  return std::string("::(") + NameMatcher + ")$"; 
}

void FunctionNameCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(functionDecl(
    unless(matchesName(validPetscFunctionName()))
  ).bind("function"), this); 
}

void FunctionNameCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl  = Result.Nodes.getNodeAs<FunctionDecl>("function");
  if (!MatchedDecl)
    return;

  diag(MatchedDecl -> getLocation(), "All function names consist of acronyms or words "
                                     ", each of which is capitalized " 
                                      "for example: KSPSolve() or MatGetOrdering()"
                                    ) << MatchedDecl; 
}

} // namespace petsc
} // namespace tidy
} // namespace clang
