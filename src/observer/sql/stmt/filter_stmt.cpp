/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2022/5/22.
//

#include "sql/stmt/filter_stmt.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "common/sys/rc.h"
#include "sql/parser/expression_binder.h"
#include "storage/db/db.h"
#include "storage/table/table.h"

void FilterUnit::set_comparison(unique_ptr<ComparisonExpr> comparison_expr)
{
  comparison_expr_ = std::move(comparison_expr);
  Expression *left  = comparison_expr_ ? comparison_expr_->left().get() : nullptr;
  Expression *right = comparison_expr_ ? comparison_expr_->right().get() : nullptr;
  left_.init_expression(left);
  right_.init_expression(right);
}

FilterStmt::~FilterStmt()
{
  for (FilterUnit *unit : filter_units_) {
    delete unit;
  }
  filter_units_.clear();
}

RC FilterStmt::create(Db *db, Table *default_table, unordered_map<string, Table *> *tables,
    const ConditionSqlNode *conditions, int condition_num, FilterStmt *&stmt)
{
  RC rc = RC::SUCCESS;
  stmt  = nullptr;

  FilterStmt *tmp_stmt = new FilterStmt();

  if (condition_num > 0 && conditions == nullptr) {
    LOG_WARN("invalid argument: conditions is null while condition_num=%d", condition_num);
    delete tmp_stmt;
    return RC::INVALID_ARGUMENT;
  }

  BinderContext binder_context;
  if (tables != nullptr) {
    for (const auto &entry : *tables) {
      binder_context.add_table(entry.second);
    }
  } else if (default_table != nullptr) {
    binder_context.add_table(default_table);
  }

  ExpressionBinder expression_binder(binder_context);

  for (int i = 0; i < condition_num; i++) {
    FilterUnit *filter_unit = nullptr;

    rc = create_filter_unit(db, default_table, tables, conditions[i], expression_binder, filter_unit);
    if (rc != RC::SUCCESS) {
      delete tmp_stmt;
      LOG_WARN("failed to create filter unit. condition index=%d", i);
      return rc;
    }
    tmp_stmt->filter_units_.push_back(filter_unit);
  }

  stmt = tmp_stmt;
  return rc;
}

RC FilterStmt::create_filter_unit(Db *db, Table *default_table, unordered_map<string, Table *> *tables,
    const ConditionSqlNode &condition, ExpressionBinder &expression_binder, FilterUnit *&filter_unit)
{
  RC rc = RC::SUCCESS;

  (void)db;
  (void)default_table;
  (void)tables;

  CompOp comp = condition.comp;
  if (comp < EQUAL_TO || comp >= NO_OP) {
    LOG_WARN("invalid compare operator : %d", comp);
    return RC::INVALID_ARGUMENT;
  }

  filter_unit = new FilterUnit;
  unique_ptr<ComparisonExpr> comparison_expr(
      new ComparisonExpr(condition.comp, condition.left->copy(), condition.right->copy()));

  vector<unique_ptr<Expression>> bound_expressions;
  unique_ptr<Expression>         comparison_base(comparison_expr.release());
  rc = expression_binder.bind_expression(comparison_base, bound_expressions);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  if (bound_expressions.size() != 1) {
    LOG_WARN("invalid comparison expression binding result size: %zu", bound_expressions.size());
    return RC::INVALID_ARGUMENT;
  }

  comparison_expr.reset(static_cast<ComparisonExpr *>(bound_expressions[0].release()));
  filter_unit->set_comparison(std::move(comparison_expr));

  // 检查两个类型是否能够比较
  return rc;
}
