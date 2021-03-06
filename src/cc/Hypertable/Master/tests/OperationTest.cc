/*
 * Copyright (C) 2007-2016 Hypertable, Inc.
 *
 * This file is part of Hypertable.
 *
 * Hypertable is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 3 of the
 * License, or any later version.
 *
 * Hypertable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <Common/Compat.h>

#include "OperationTest.h"

#include "Hypertable/Master/OperationProcessor.h"

#include <Common/Error.h>
#include <Common/FailureInducer.h>
#include <Common/Serialization.h>
#include <Common/StringExt.h>

#include <iostream>
#include <mutex>

using namespace Hypertable;
using namespace std;

OperationTest::OperationTest(ContextPtr &context, std::vector<String> &results, const String &name,
                             DependencySet &dependencies, DependencySet &exclusivities,
                             DependencySet &obstructions)
  : Operation(context, MetaLog::EntityType::OPERATION_TEST), m_results(results), m_name(name), m_is_perpetual(false) {
  m_dependencies = dependencies;
  m_exclusivities = exclusivities;
  m_obstructions = obstructions;
  if (boost::ends_with(name, "[0]"))
    set_state(OperationState::STARTED);
}

OperationTest::OperationTest(ContextPtr &context, std::vector<String> &results,
                             const String &name, int32_t state) :
  Operation(context, MetaLog::EntityType::OPERATION_TEST), m_results(results),
  m_name(name), m_is_perpetual(false) {
  set_state(state);
}

void OperationTest::execute() {
  int32_t state = get_state();

  std::cout << m_name << " - " << OperationState::get_text(state) << std::endl;

  if (!is_perpetual()) {
    if (state == OperationState::INITIAL) {
      DependencySet exclusivities;
      DependencySet dependencies;
      DependencySet obstructions;
      stage_subop(make_shared<OperationTest>(m_context, m_results, m_name+"[0]",
                                             dependencies, exclusivities,
                                             obstructions));
      set_state(OperationState::STARTED);
      return;
    }
    else if (state == OperationState::STARTED) {
      HT_ASSERT(validate_subops());
      lock_guard<mutex> lock(m_context->mutex);
      m_results.push_back(m_name);
      set_state(OperationState::COMPLETE);
    }
    else
      HT_ASSERT(!"Unknown state");
  }
  else
    set_state(OperationState::COMPLETE);    
}


void OperationTest::display_state(std::ostream &os) {
  os << " name=" << m_name <<  " ";
}

uint8_t OperationTest::encoding_version_state() const {
  return 1;
}

size_t OperationTest::encoded_length_state() const {
  return Serialization::encoded_length_vstr(m_name);
}

void OperationTest::encode_state(uint8_t **bufp) const {
  Serialization::encode_vstr(bufp, m_name);
}

void OperationTest::decode_state(uint8_t version, const uint8_t **bufp, size_t *remainp) {
  decode_request(bufp, remainp);
}

void OperationTest::decode_state_old(uint8_t version, const uint8_t **bufp, size_t *remainp) {
  decode_request(bufp, remainp);
}

void OperationTest::decode_request(const uint8_t **bufp, size_t *remainp) {
  m_name = Serialization::decode_vstr(bufp, remainp);
}

const String OperationTest::name() {
  return "OperationTest-" + m_name;
}

const String OperationTest::label() {
  return "OperationTest-" + m_name;
}


