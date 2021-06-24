//===--- FunctionInStructNameCheck.cpp - clang-tidy ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "FunctionInStructNameCheck.h"
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

std::string validPetscFunctionInStructName(){
  std::string NameMatcher = std::string("[a-z0-9]+"); 
  return std::string("::(") + NameMatcher + ")$"; 
}

void FunctionInStructNameCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(fieldDecl(
      allOf(
          unless(matchesName(validPetscFunctionInStructName())),
          hasType(
              pointerType(
                  pointee(
                      ignoringParens(
                          functionType()
                      )
                  )
              )
          )
      )
    ).bind("function"), this); 
}

void FunctionInStructNameCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl  = Result.Nodes.getNodeAs<Decl>("function");
  if (!MatchedDecl)
    return;

  diag(MatchedDecl -> getLocation(), "Function names in structures are "
                                     "the same as the base application function name " 
                                     "without the object prefix and are in small letters. "
                                     "For example, *MatMultTranspose()* includes the structure name "
                                     "multtranspose. "
                                    ); 
}

} // namespace petsc
} // namespace tidy
} // namespace clang
