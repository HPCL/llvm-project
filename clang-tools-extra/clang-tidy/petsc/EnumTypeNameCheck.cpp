//===--- EnumTypeNameCheck.cpp - clang-tidy ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "EnumTypeNameCheck.h"
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


std::string validEnumTypeNameMatcher(){
  std::string NameMatcher = "[A-Z]+" + std::string("([A-Z][a-zA-Z0-9]+)$");
  return std::string("::(") + NameMatcher + ")$"; 
}

void EnumTypeNameCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      tagDecl(
        isEnum(),
        matchesName(validEnumTypeNameMatcher())
      ).bind("enumTy"), this); 
}

void EnumTypeNameCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl  = Result.Nodes.getNodeAs<Decl>("enumTy");
  if (!MatchedDecl)
    return;

  diag(MatchedDecl -> getLocation(), "All enum types consist of acronyms or words "
                                                        ", each of which is capitalized " 
                                                        "for example: KSPSolve() or MatGetOrdering()"
                                                        ); 
}

} // namespace petsc
} // namespace tidy
} // namespace clang
