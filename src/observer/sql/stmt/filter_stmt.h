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

#pragma once

#include "common/lang/unordered_map.h"
#include "common/lang/vector.h"
#include "sql/expr/expression.h"
#include "sql/parser/parse_defs.h"
#include "sql/stmt/stmt.h"

class Db;
class Table;
class FieldMeta;
class ExpressionBinder;

struct FilterObj
{
  FilterObj() = default;

  void init_expression(Expression *expr) { expression_ = expr; }

  Expression *expression() const { return expression_; }

private:
  Expression *expression_ = nullptr;  ///< non-owning pointer, owned by the parent comparison expression
};

class FilterUnit
{
public:
  FilterUnit() = default;
  ~FilterUnit() {}

  void set_comp(CompOp comp) { comp_ = comp; }

  CompOp comp() const { return comp_; }

  void set_comparison(unique_ptr<ComparisonExpr> comparison_expr);

  FilterObj &left() { return left_; }
  FilterObj &right() { return right_; }

  const FilterObj &left() const { return left_; }
  const FilterObj &right() const { return right_; }

  unique_ptr<ComparisonExpr> take_comparison_expr()
  {
    left_.init_expression(nullptr);
    right_.init_expression(nullptr);
    return std::move(comparison_expr_);
  }

private:
  CompOp                         comp_ = NO_OP;
  FilterObj                      left_;
  FilterObj                      right_;
  unique_ptr<ComparisonExpr>     comparison_expr_;
};

/**
 * @brief Filter/谓词/过滤语句
 * @ingroup Statement
 */
class FilterStmt
{
public:
  FilterStmt() = default;
  virtual ~FilterStmt();

public:
  const vector<FilterUnit *> &filter_units() const { return filter_units_; }

public:
  static RC create(Db *db, Table *default_table, unordered_map<string, Table *> *tables,
      const ConditionSqlNode *conditions, int condition_num, FilterStmt *&stmt);

  static RC create_filter_unit(Db *db, Table *default_table, unordered_map<string, Table *> *tables,
      const ConditionSqlNode &condition, ExpressionBinder &expression_binder, FilterUnit *&filter_unit);

private:
  vector<FilterUnit *> filter_units_;  // 默认当前都是AND关系
};
