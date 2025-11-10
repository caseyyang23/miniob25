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
// Created by OceanBase AI Lab.
//

#pragma once

#include "sql/operator/logical_operator.h"
#include "common/value.h"
#include "storage/field/field_meta.h"
#include "storage/table/table.h"

class UpdateLogicalOperator : public LogicalOperator
{
public:
  UpdateLogicalOperator(Table *table, std::vector<const FieldMeta *> &&fields, std::vector<Value> &&values);
  virtual ~UpdateLogicalOperator() = default;

  LogicalOperatorType type() const override { return LogicalOperatorType::UPDATE; }

  OpType get_op_type() const override { return OpType::LOGICALUPDATE; }

  Table *table() const { return table_; }

  const std::vector<const FieldMeta *> &update_fields() const { return update_fields_; }
  std::vector<const FieldMeta *>       &update_fields() { return update_fields_; }

  const std::vector<Value> &update_values() const { return update_values_; }
  std::vector<Value>       &update_values() { return update_values_; }

private:
  Table                              *table_ = nullptr;
  std::vector<const FieldMeta *>      update_fields_;
  std::vector<Value>                  update_values_;
};

