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

#include "sql/operator/update_physical_operator.h"

#include <algorithm>
#include <cstring>

#include "common/log/log.h"
#include "sql/expr/tuple.h"
#include "storage/table/table.h"
#include "storage/table/table_meta.h"
#include "storage/trx/trx.h"

UpdatePhysicalOperator::UpdatePhysicalOperator(
    Table *table, std::vector<const FieldMeta *> &&fields, std::vector<Value> &&values)
    : table_(table), update_fields_(std::move(fields)), update_values_(std::move(values))
{}

RC UpdatePhysicalOperator::apply_updates(Record &record) const
{
  if (nullptr == table_) {
    return RC::INTERNAL;
  }

  for (size_t i = 0; i < update_fields_.size(); i++) {
    const FieldMeta *field_meta = update_fields_[i];
    const Value     &value      = update_values_[i];
    char            *target     = record.data() + field_meta->offset();

    size_t copy_len = field_meta->len();
    size_t data_len = static_cast<size_t>(value.length());
    if (field_meta->type() == AttrType::CHARS) {
      memset(target, 0, field_meta->len());
      if (copy_len > data_len) {
        copy_len = data_len + 1;
      } else {
        copy_len = std::min(copy_len, data_len);
      }
    } else {
      copy_len = std::min(copy_len, data_len);
    }

    memcpy(target, value.data(), copy_len);
  }
  return RC::SUCCESS;
}

RC UpdatePhysicalOperator::open(Trx *trx)
{
  trx_ = trx;
  update_items_.clear();

  if (children_.empty()) {
    LOG_WARN("update operator requires one child to provide records");
    return RC::INTERNAL;
  }

  std::unique_ptr<PhysicalOperator> &child = children_[0];

  RC rc = child->open(trx);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to open child operator. rc=%s", strrc(rc));
    return rc;
  }

  while (OB_SUCC(rc = child->next())) {
    Tuple *tuple = child->current_tuple();
    if (nullptr == tuple) {
      LOG_WARN("failed to get tuple from child operator");
      rc = RC::INTERNAL;
      break;
    }

    RowTuple *row_tuple = static_cast<RowTuple *>(tuple);
    Record   &base      = row_tuple->record();

    UpdateItem item;
    rc = item.old_record.copy_data(base.data(), base.len());
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to copy original record. rc=%s", strrc(rc));
      break;
    }
    item.old_record.set_rid(base.rid());

    rc = item.new_record.copy_data(base.data(), base.len());
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to copy record for update. rc=%s", strrc(rc));
      break;
    }
    item.new_record.set_rid(base.rid());

    rc = apply_updates(item.new_record);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to apply update values. rc=%s", strrc(rc));
      break;
    }

    update_items_.emplace_back(std::move(item));
  }

  RC close_rc = child->close();
  if (OB_FAIL(close_rc)) {
    LOG_WARN("failed to close child operator. rc=%s", strrc(close_rc));
  }

  if (rc == RC::RECORD_EOF) {
    rc = RC::SUCCESS;
  }

  if (OB_FAIL(rc)) {
    return rc;
  }

  for (UpdateItem &item : update_items_) {
    rc = trx_->update_record(table_, item.old_record, item.new_record);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to update record via trx. rc=%s", strrc(rc));
      return rc;
    }
  }

  return RC::SUCCESS;
}

RC UpdatePhysicalOperator::next()
{
  return RC::RECORD_EOF;
}

RC UpdatePhysicalOperator::close()
{
  update_items_.clear();
  return RC::SUCCESS;
}