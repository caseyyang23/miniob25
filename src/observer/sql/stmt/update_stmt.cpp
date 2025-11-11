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

#include "sql/stmt/update_stmt.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "common/type/attr_type.h"
#include "sql/stmt/filter_stmt.h"
#include "common/value.h"
#include "storage/db/db.h"
#include "storage/table/table.h"

#include <string>
#include <unordered_map>

UpdateStmt::UpdateStmt(Table *table, std::vector<const FieldMeta *> &&fields, std::vector<Value> &&values,
    FilterStmt *filter_stmt)
    : table_(table), update_fields_(std::move(fields)), update_values_(std::move(values)), filter_stmt_(filter_stmt)
{}

UpdateStmt::~UpdateStmt()
{
  if (nullptr != filter_stmt_) {
    delete filter_stmt_;
    filter_stmt_ = nullptr;
  }
}

RC UpdateStmt::create(Db *db, const UpdateSqlNode &update, Stmt *&stmt)
{
  // TODO
  stmt = nullptr;

  const char *table_name = update.relation_name.c_str();
  if (nullptr == db || common::is_blank(table_name)) {
    LOG_WARN("invalid argument. db=%p, table_name=%s", db, table_name ? table_name : "");
    return RC::INVALID_ARGUMENT;
  }

  Table *table = db->find_table(table_name);
  if (nullptr == table) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  const TableMeta &table_meta = table->table_meta();
  const FieldMeta *field_meta = table_meta.field(update.attribute_name.c_str());
  if (nullptr == field_meta || !field_meta->visible()) {
    LOG_WARN("no such field. table=%s, field=%s", table->name(), update.attribute_name.c_str());
    return RC::SCHEMA_FIELD_NOT_EXIST;
  }

  Value target_value;
  const Value &origin_value = update.value;
  if (field_meta->type() != origin_value.attr_type()) {
    RC rc = Value::cast_to(origin_value, field_meta->type(), target_value);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to cast value. table=%s, field=%s, value=%s, target type=%s", table->name(),
          field_meta->name(), origin_value.to_string().c_str(), attr_type_to_string(field_meta->type()));
      return rc;
    }
  } else {
    target_value.set_value(origin_value);
  }

  std::unordered_map<std::string, Table *> table_map;
  table_map.emplace(table->name(), table);

  FilterStmt *filter_stmt = nullptr;
  RC rc = FilterStmt::create(db,
      table,
      &table_map,
      update.conditions.empty() ? nullptr : update.conditions.data(),
      static_cast<int>(update.conditions.size()),
      filter_stmt);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to create filter statement. rc=%s", strrc(rc));
    return rc;
  }

  std::vector<const FieldMeta *> fields;
  std::vector<Value>             values;
  fields.push_back(field_meta);
  values.push_back(std::move(target_value));

  stmt = new UpdateStmt(table, std::move(fields), std::move(values), filter_stmt);
  return RC::SUCCESS;
}