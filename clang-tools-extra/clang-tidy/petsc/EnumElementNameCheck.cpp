//===--- FunctionInStructNameCheck.cpp - clang-tidy ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "EnumElementNameCheck.h"
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

std::string validPetscEnumElementName(){
  std::string NameMatcher = std::string("([A-Z]+)(_([A-Z]+))*"); 
  return std::string("::(") + NameMatcher + ")$"; 
}

void EnumElementNameCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(enumConstantDecl(
      unless(matchesName(validPetscEnumElementName()))
  ).bind("enumElem"), this); 
}

void EnumElementNameCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl  = Result.Nodes.getNodeAs<Decl>("enumElem");
  if (!MatchedDecl)
    return;

  diag(MatchedDecl -> getLocation(), "All enum elements are named with all capital "
                                     "letters. When they consist of several complete "
                                     "words, there is an underscore between each word. "
                                     "For example, *MAT_FINAL_ASSEMBLY* "
                                    ); 
}

} // namespace petsc
} // namespace tidy
} // namespace clang
