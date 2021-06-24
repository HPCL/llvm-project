//===--- IfStamentRuleCheck.cpp - clang-tidy ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "IfStatementRuleCheck.h"
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


void IfStatementRuleCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(ifStmt(
      hasCondition(
          binaryOperator(
              hasOperatorName("=="),
              hasEitherOperand(
                  anyOf(
                    integerLiteral(equals(0)),
                    nullPointerConstant(),
                    ignoringImpCasts(declRefExpr(to(varDecl(
                        anyOf(
                            hasName("PETSC_TRUE"),
                            hasName("PETSC_FALSE")
                        )
                    ))))     
                )
              )

          )
      )
  ).bind("ifstmt"), this);
}

void IfStatementRuleCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *MatchedDecl  = Result.Nodes.getNodeAs<IfStmt>("ifstmt");
  if (!MatchedDecl)
    return; 
  diag(MatchedDecl -> getBeginLoc(), "Do not use *if (rank == 0)* or "
                                     "*if (v == NULL)* or *if (flg == PETSC_TRUE)* "
                                     "*or if (flg == PETSC_FALSE)*. Instead, use *if (!rank)* "
                                     "or *if (!v)* or *if (flg)* or *if (!flg)* ");  
}

} // namespace petsc
} // namespace tidy
} // namespace clang
