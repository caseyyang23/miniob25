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

#include "sql/operator/physical_operator.h"
#include "common/value.h"
#include "storage/field/field_meta.h"
#include "storage/table/table.h"
#include <vector>

class UpdatePhysicalOperator : public PhysicalOperator
{
public:
  UpdatePhysicalOperator(Table *table, std::vector<const FieldMeta *> &&fields, std::vector<Value> &&values);
  virtual ~UpdatePhysicalOperator() = default;

  PhysicalOperatorType type() const override { return PhysicalOperatorType::UPDATE; }

  OpType get_op_type() const override { return OpType::UPDATE; }

  RC open(Trx *trx) override;
  RC next() override;
  RC close() override;

private:
  struct UpdateItem
  {
    Record old_record;
    Record new_record;
  };

  RC apply_updates(Record &record) const;

private:
  Table                              *table_ = nullptr;
  std::vector<const FieldMeta *>      update_fields_;
  std::vector<Value>                  update_values_;
  std::vector<UpdateItem>             update_items_;
  Trx                                *trx_ = nullptr;
};