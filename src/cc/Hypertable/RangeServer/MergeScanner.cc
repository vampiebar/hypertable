/** -*- c++ -*-
 * Copyright (C) 2008 Doug Judd (Zvents, Inc.)
 *
 * This file is part of Hypertable.
 *
 * Hypertable is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2 of the
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

#include "Common/Compat.h"
#include <re2/re2.h>
#include <cassert>

#include "Common/Logger.h"

#include "Hypertable/Lib/Key.h"

#include "MergeScanner.h"

using namespace Hypertable;


MergeScanner::MergeScanner(ScanContextPtr &scan_ctx, bool return_deletes, bool ag_scanner,
    bool debug) : CellListScanner(scan_ctx), m_done(false), m_initialized(false),
    m_scanners(), m_queue(), m_delete_present(false), m_deleted_row(0),
    m_deleted_column_family(0), m_deleted_cell(0), m_deleted_cell_version(0),
    m_return_deletes(return_deletes),
    m_no_forward(false), m_count_present(false), m_skip_remaining_counter(false),
    m_counted_value(12), m_ag_scanner(ag_scanner), m_row_count(0), m_row_limit(0),
    m_cell_count(0), m_cell_limit_per_family(0), m_revs_count(0), m_revs_limit(0), m_cell_cutoff(0),
    m_bytes_input(0), m_bytes_output(0), m_cells_input(0), m_cells_output(0),
    m_cur_bytes(0), m_prev_key(0), m_prev_cf(-1), m_debug(debug) {

  if (scan_ctx->spec != 0) {
    m_row_limit = scan_ctx->spec->row_limit;
    m_cell_limit_per_family = scan_ctx->spec->cell_limit_per_family;
  }

  m_start_timestamp = scan_ctx->time_interval.first;
  m_end_timestamp = scan_ctx->time_interval.second;
  m_revision = scan_ctx->revision;
}


void MergeScanner::add_scanner(CellListScanner *scanner) {
  m_scanners.push_back(scanner);
}


MergeScanner::~MergeScanner() {
  try {
    for (size_t i=0; i<m_scanners.size(); i++)
      delete m_scanners[i];
    if (m_release_callback)
      m_release_callback();
  }
  catch (Hypertable::Exception &e) {
    HT_ERROR_OUT << "Problem destroying MergeScanner : " << e << HT_END;
  }
}


void MergeScanner::forward() {
  ScannerState sstate;
  Key key;
  size_t len;
  bool counter;
  bool new_row, new_cf, new_cq;

  if (m_queue.empty()) {
    if (m_count_present)
      finish_count();
    else
      // scan is done
      m_no_forward = false;
    return;
  }
  sstate = m_queue.top();

  /**
   * Pop the top element, forward it, re-insert it back into the queue;
   */
  while (true) {
    while (true) {

      m_queue.pop();

      new_row = new_cf = new_cq = false;
      /**
       * In some cases the forward might already be done and so the scanner shdn't be forwarded
       * again. For example you know a counter is done only after forwarding to the 1st post
       * counter cell or reaching the end of the scan.
       */
      if (m_no_forward)
        m_no_forward = false;
      else
        sstate.scanner->forward();

      if (sstate.scanner->get(sstate.key, sstate.value))
        m_queue.push(sstate);

      if (m_queue.empty()) {
        // scan ended on a counter
        if (m_count_present)
          finish_count();
        return;
      }
      sstate = m_queue.top();

      // I/O tracking
      m_cur_bytes = sstate.key.length + sstate.value.length();
      m_bytes_input += m_cur_bytes;
      m_cells_input++;

      // we only need to care about counters for a MergeScanner which is merging over
      // a single access group since no counter will span multiple access groups
      counter = m_scan_context_ptr->family_info[sstate.key.column_family_code].counter &&
        m_ag_scanner;

      m_cell_cutoff = m_scan_context_ptr->family_info[
        sstate.key.column_family_code].cutoff_time;

      if(sstate.key.timestamp < m_cell_cutoff )
        continue;

      if (sstate.key.timestamp < m_start_timestamp) {
        continue;
      }
      else if (sstate.key.revision > m_revision
          || (sstate.key.timestamp >= m_end_timestamp && (!m_return_deletes || sstate.key.flag == FLAG_INSERT))) {
        continue;
      }
      else if (sstate.key.flag == FLAG_DELETE_ROW) {
        len = sstate.key.len_row();
        if (matches_deleted_row(sstate.key)) {
          if (m_deleted_row_timestamp < sstate.key.timestamp)
            m_deleted_row_timestamp = sstate.key.timestamp;
        }
        else {
          m_deleted_row.clear();
          m_deleted_row.ensure(len);
          memcpy(m_deleted_row.base, sstate.key.row, len);
          m_deleted_row.ptr = m_deleted_row.base + len;
          m_deleted_row_timestamp = sstate.key.timestamp;
          m_delete_present = true;
        }
        if (m_return_deletes)
          break;
      }
      else if (sstate.key.flag == FLAG_DELETE_COLUMN_FAMILY) {
        len = sstate.key.len_column_family();
        if (matches_deleted_column_family(sstate.key)) {
          if (m_deleted_column_family_timestamp < sstate.key.timestamp)
            m_deleted_column_family_timestamp = sstate.key.timestamp;
        }
        else {
          m_deleted_column_family.clear();
          m_deleted_column_family.ensure(len);
          memcpy(m_deleted_column_family.base, sstate.key.row, len);
          m_deleted_column_family.ptr = m_deleted_column_family.base + len;
          m_deleted_column_family_timestamp = sstate.key.timestamp;
          m_delete_present = true;
        }
        if (m_return_deletes)
          break;
      }
      else if (sstate.key.flag == FLAG_DELETE_CELL) {
        len = sstate.key.len_cell();
        if (matches_deleted_cell(sstate.key)) {
          if (m_deleted_cell_timestamp < sstate.key.timestamp)
            m_deleted_cell_timestamp = sstate.key.timestamp;
        }
        else {
          m_deleted_cell.clear();
          m_deleted_cell.ensure(len);
          memcpy(m_deleted_cell.base, sstate.key.row, len);
          m_deleted_cell.ptr = m_deleted_cell.base + len;
          m_deleted_cell_timestamp = sstate.key.timestamp;
          m_delete_present = true;
        }
        if (m_return_deletes)
          break;
      }
      else if (sstate.key.flag == FLAG_DELETE_CELL_VERSION) {
        len = sstate.key.len_cell();
        if (matches_deleted_cell_version(sstate.key))
          m_deleted_cell_version_set.insert(sstate.key.timestamp);
        else {
          m_deleted_cell_version.clear();
          m_deleted_cell_version_set.clear();
          m_deleted_cell_version.ensure(len);
          memcpy(m_deleted_cell_version.base, sstate.key.row, len);
          m_deleted_cell_version.ptr = m_deleted_cell_version.base + len;
          m_deleted_cell_version_set.insert(sstate.key.timestamp);
          m_delete_present = true;
        }
        if (m_return_deletes)
          break;
      }
      else {
        // this cell is not a delete and it is within the requested time interval.
        if (m_delete_present) {
          if (m_deleted_cell_version.fill() > 0) {
            if(!matches_deleted_cell_version(sstate.key)) {
              // we wont see the previously seen deleted cell version again
              m_deleted_cell_version.clear();
              m_deleted_cell_version_set.clear();
            }
            else if (m_deleted_cell_version_set.find(sstate.key.timestamp) !=
                     m_deleted_cell_version_set.end())
              // apply previously seen delete cell version to this cell
              continue;
          }
          if (m_deleted_cell.fill() > 0) {
            if(!matches_deleted_cell(sstate.key))
              // we wont see the previously seen deleted cell again
              m_deleted_cell.clear();
            else if (sstate.key.timestamp <= m_deleted_cell_timestamp)
              // apply previously seen delete cell to this cell
              continue;
          }
          if (m_deleted_column_family.fill() > 0) {
            if(!matches_deleted_column_family(sstate.key))
              // we wont see the previously seen deleted column family again
              m_deleted_column_family.clear();
            else if (sstate.key.timestamp <= m_deleted_column_family_timestamp)
              // apply previously seen delete column family to this cell
              continue;
          }
          if (m_deleted_row.fill() > 0) {
            if(!matches_deleted_row(sstate.key))
              // we wont see the previously seen deleted row family again
              m_deleted_row.clear();
            else if (sstate.key.timestamp <= m_deleted_row_timestamp)
              // apply previously seen delete row family to this cell
              continue;
          }
          if (m_deleted_cell_version.fill() == 0 && m_deleted_cell.fill() == 0 &&
              m_deleted_column_family.fill() == 0 && m_deleted_row.fill() == 0)
            m_delete_present = false;
        }

        // keep track of revisions
        const uint8_t *latest_key = (const uint8_t *)sstate.key.row;
        size_t latest_key_len = sstate.key.flag_ptr - (const uint8_t *)sstate.key.row + 1;

        if (m_prev_key.fill()==0) {
          new_row = new_cf = new_cq = true;
          m_cell_count = 1;
          m_prev_key.set(latest_key, latest_key_len);
          m_prev_cf = sstate.key.column_family_code;
          m_revs_count=0;
          m_revs_limit = m_scan_context_ptr->family_info[
            sstate.key.column_family_code].max_versions;
        }
        else if (m_prev_key.fill() != latest_key_len ||
            memcmp(latest_key, m_prev_key.base, latest_key_len)) {
          new_cq = true;

          if (strcmp(sstate.key.row, (const char *)m_prev_key.base)) {
            new_row = true;
            new_cf = true;
            m_cell_count = 1;
          }
          else if (sstate.key.column_family_code != m_prev_cf) {
            new_cf = true;
            m_cell_count = 1;
          }

          m_prev_key.set(latest_key, latest_key_len);
          m_prev_cf = sstate.key.column_family_code;
          m_revs_count=0;
          m_revs_limit = m_scan_context_ptr->family_info[
            sstate.key.column_family_code].max_versions;
        }
        m_revs_count++;
        if (m_revs_limit && m_revs_count > m_revs_limit && !counter)
          continue;

        // test for filter matching
        if (m_ag_scanner) {
          // row set .. we only need to do this in ag scanners
          if (!m_scan_context_ptr->rowset.empty()) {
            int cmp = 1;
            while (!m_scan_context_ptr->rowset.empty() && (cmp = strcmp(*m_scan_context_ptr->rowset.begin(), sstate.key.row)) < 0)
              m_scan_context_ptr->rowset.erase(m_scan_context_ptr->rowset.begin());
            if (cmp > 0)
              continue;
          }
          // row regexp .. we only need to do this in ag scanners
          if (m_scan_context_ptr->row_regexp) {
            bool cached, match;
            m_regexp_cache.check_rowkey(sstate.key.row, &cached, &match);
            if (!cached) {
              match = RE2::PartialMatch(sstate.key.row, *(m_scan_context_ptr->row_regexp));
              m_regexp_cache.set_rowkey(sstate.key.row, match);
            }
            if (!match)
              continue;
          }
          // column qualifier match
          if(!m_scan_context_ptr->family_info[
              sstate.key.column_family_code].has_qualifier_regexp_filter()) {
            bool cached, match;
            m_regexp_cache.check_column(sstate.key.column_family_code,
                sstate.key.column_qualifier, &cached, &match);
            if (!cached) {
              match = m_scan_context_ptr->family_info[
                  sstate.key.column_family_code].qualifier_matches(sstate.key.column_qualifier);
              m_regexp_cache.set_column(sstate.key.column_family_code,
                  sstate.key.column_qualifier, match);
            }
            if (!match)
              continue;
          }
          else if (!m_scan_context_ptr->family_info[
              sstate.key.column_family_code].qualifier_matches(sstate.key.column_qualifier)) {
            continue;
          }

          // filter but value regexp last since its probly the most expensive
          if (m_scan_context_ptr->value_regexp &&
              !m_scan_context_ptr->family_info[sstate.key.column_family_code].counter) {
            String value(sstate.value.str(), sstate.value.length());
            if (!RE2::PartialMatch(value, *(m_scan_context_ptr->value_regexp)))
              continue;
          }
        }
        break;
      }
    }

    bool incr_cf_count = false;

    // deal with counters. apply row_limit but not revs/cell_limit_per_family
    if (m_count_present) {
      if(counter && matches_counted_key(sstate.key)) {
        if (sstate.key.flag == FLAG_INSERT) {
          // keep incrementing
          increment_count(sstate.key, sstate.value);
          continue;
        }
      }
      else {
        // count done, new count seen but not started
        finish_count();
        break;
      }
    }
    else if (counter && sstate.key.flag == FLAG_INSERT) {
      // new counter, check for row limit
      if (new_row) {
        m_row_count++;
        if (!m_return_deletes && m_row_limit && m_row_count > m_row_limit) {
          m_done = true;
          break;
        }
      }

      // start new count and loop
      start_count(sstate.key, sstate.value);
      continue;
    }

    if (m_row_limit || m_cell_limit_per_family) {
      if (new_row) {
        m_row_count++;
        if (!m_return_deletes && m_row_limit && m_row_count > m_row_limit)
          m_done = true;
        break;
      }
      else if (m_cell_limit_per_family) {
        if (!new_cf)
          incr_cf_count = true;
      }
    }

    if (incr_cf_count) {
      m_cell_count++;
      if (m_cell_count > m_cell_limit_per_family)
        continue;
    }

    break;
  }
  m_bytes_output += m_cur_bytes;
  m_cells_output++;
}

bool MergeScanner::get(Key &key, ByteString &value) {
  if (!m_initialized)
    initialize();

  if (m_done)
    return false;

  // check if we have a counter result ready
  if (m_no_forward) {
    key = m_counted_key;
    value.ptr = m_counted_value.base;
    return true;
  }

  if (!m_queue.empty()) {
    const ScannerState &sstate = m_queue.top();
    // check for row or cell limit
    key = sstate.key;
    value = sstate.value;
    return true;
  }

  return false;
}

void MergeScanner::finish_count() {
  uint8_t *ptr = m_counted_value.base;

  *ptr++ = 8;  // length
  Serialization::encode_i64(&ptr, m_count);

  m_prev_key.set(m_counted_key.row, m_counted_key.len_cell());
  m_prev_cf = m_counted_key.column_family_code;
  m_cell_count = 0;
  m_no_forward = true;
  m_count_present = false;
}

void MergeScanner::start_count(const Key &key, const ByteString &value) {
  SerializedKey serial;

  m_count_present = true;
  m_count = 0;

  m_counted_key_buffer.clear();
  m_counted_key_buffer.ensure(key.length);
  memcpy(m_counted_key_buffer.base, key.serial.ptr, key.length);
  serial.ptr = m_counted_key_buffer.base;

  m_counted_key.load(serial);
  increment_count(key, value);

}

void MergeScanner::initialize() {
  ScannerState sstate;
  bool counter, new_row, new_cf, new_cq;

  m_cur_bytes = 0;

  while (!m_queue.empty())
    m_queue.pop();

  for (size_t i=0; i<m_scanners.size(); i++) {
    if (m_scanners[i]->get(sstate.key, sstate.value)) {
      sstate.scanner = m_scanners[i];
      m_queue.push(sstate);
    }
  }
  while (!m_queue.empty()) {
    sstate = m_queue.top();

    new_row = new_cf = new_cq = false;

    // I/O tracking
    m_cur_bytes = sstate.key.length + sstate.value.length();
    m_bytes_input += m_cur_bytes;
    m_cells_input++;


    m_cell_cutoff = m_scan_context_ptr->family_info[
        sstate.key.column_family_code].cutoff_time;

    // Only need to worry about counters if this scanner scans over a single access group
    // since no counter will span multiple access grps
    counter = m_scan_context_ptr->family_info[sstate.key.column_family_code].counter &&
              m_ag_scanner;

    if (sstate.key.timestamp < m_cell_cutoff
        || (sstate.key.timestamp < m_start_timestamp)) {
      m_queue.pop();
      sstate.scanner->forward();
      if (sstate.scanner->get(sstate.key, sstate.value))
        m_queue.push(sstate);
      continue;
    }

    if (sstate.key.flag == FLAG_DELETE_ROW) {
      size_t len = sstate.key.len_row();
      m_deleted_row.clear();
      m_deleted_row.ensure(len);
      memcpy(m_deleted_row.base, sstate.key.row, len);
      m_deleted_row.ptr = m_deleted_row.base + len;
      m_deleted_row_timestamp = sstate.key.timestamp;
      m_delete_present = true;
      if (!m_return_deletes) {
        forward();
        m_initialized = true;
        return;
      }
    }
    else if (sstate.key.flag == FLAG_DELETE_COLUMN_FAMILY) {
      size_t len = sstate.key.len_column_family();
      m_deleted_column_family.clear();
      m_deleted_column_family.ensure(len);
      memcpy(m_deleted_column_family.base, sstate.key.row, len);
      m_deleted_column_family.ptr = m_deleted_column_family.base + len;
      m_deleted_column_family_timestamp = sstate.key.timestamp;
      m_delete_present = true;
      if (!m_return_deletes) {
        forward();
        m_initialized = true;
        return;
      }
    }
    else if (sstate.key.flag == FLAG_DELETE_CELL) {
      size_t len = sstate.key.len_cell();
      m_deleted_cell.clear();
      m_deleted_cell.ensure(len);
      memcpy(m_deleted_cell.base, sstate.key.row, len);
      m_deleted_cell.ptr = m_deleted_cell.base + len;
      m_deleted_cell_timestamp = sstate.key.timestamp;
      m_delete_present = true;
      if (!m_return_deletes) {
        forward();
        m_initialized = true;
        return;
      }
    }
    else if (sstate.key.flag == FLAG_DELETE_CELL_VERSION) {
      size_t len = sstate.key.len_cell();
      m_deleted_cell_version.clear();
      m_deleted_cell_version.ensure(len);
      memcpy(m_deleted_cell_version.base, sstate.key.row, len);
      m_deleted_cell_version.ptr = m_deleted_cell_version.base + len;
      m_deleted_cell_version_set.insert(sstate.key.timestamp);
      m_delete_present = true;
      if (!m_return_deletes) {
        forward();
        m_initialized = true;
        return;
      }
    }
    else {
      if (sstate.key.revision > m_revision
          || (sstate.key.timestamp >= m_end_timestamp && (!m_return_deletes || sstate.key.flag == FLAG_INSERT))) {
        m_queue.pop();
        sstate.scanner->forward();
        if (sstate.scanner->get(sstate.key, sstate.value))
          m_queue.push(sstate);
        continue;
      }

      // keep track of revisions
      const uint8_t *latest_key = (const uint8_t *)sstate.key.row;
      size_t latest_key_len = sstate.key.flag_ptr - (const uint8_t *)sstate.key.row + 1;

      if (m_prev_key.fill()==0) {
        new_row = new_cf = new_cq = true;
        m_cell_count = 1;
        m_prev_key.set(latest_key, latest_key_len);
        m_prev_cf = sstate.key.column_family_code;
        m_revs_count=0;
        m_revs_limit = m_scan_context_ptr->family_info[
          sstate.key.column_family_code].max_versions;
      }
      else if (m_prev_key.fill() != latest_key_len ||
          memcmp(latest_key, m_prev_key.base, latest_key_len)) {
        new_cq = true;

        if (strcmp(sstate.key.row, (const char *)m_prev_key.base)) {
          new_row = true;
          new_cf = true;
          m_cell_count=1;
        }
        else if (sstate.key.column_family_code != m_prev_cf) {
          new_cf = true;
          m_cell_count=1;
        }

        m_prev_key.set(latest_key, latest_key_len);
        m_prev_cf = sstate.key.column_family_code;
        m_revs_count=0;
        m_revs_limit = m_scan_context_ptr->family_info[
          sstate.key.column_family_code].max_versions;
      }
      m_revs_count++;
      if (m_revs_limit && m_revs_count > m_revs_limit && !counter) {
        m_queue.pop();
        sstate.scanner->forward();
        if (sstate.scanner->get(sstate.key, sstate.value))
          m_queue.push(sstate);
        continue;
      }

      // test for filter matching
      if (m_ag_scanner) {
        // row set .. we only need to do this in ag scanners
        if (!m_scan_context_ptr->rowset.empty()) {
          int cmp = 1;
          while (!m_scan_context_ptr->rowset.empty() && (cmp = strcmp(*m_scan_context_ptr->rowset.begin(), sstate.key.row)) < 0)
            m_scan_context_ptr->rowset.erase(m_scan_context_ptr->rowset.begin());
          if (cmp > 0) {
            m_queue.pop();
            sstate.scanner->forward();
            if (sstate.scanner->get(sstate.key, sstate.value))
              m_queue.push(sstate);
            continue;
          }
        }
        // row regexp .. we only need to do this in ag scanners
        if (m_scan_context_ptr->row_regexp)
          if (!RE2::PartialMatch(sstate.key.row, *(m_scan_context_ptr->row_regexp))) {
            m_queue.pop();
            sstate.scanner->forward();
            if (sstate.scanner->get(sstate.key, sstate.value))
              m_queue.push(sstate);
            continue;
          }
        // column qualifier doesn't match
        if (!m_scan_context_ptr->family_info[
            sstate.key.column_family_code].qualifier_matches(sstate.key.column_qualifier)) {
          m_queue.pop();
          sstate.scanner->forward();
          if (sstate.scanner->get(sstate.key, sstate.value))
            m_queue.push(sstate);
          continue;
        }
        // filter but value regexp last since its probly the most expensive
        if (m_scan_context_ptr->value_regexp &&
            !m_scan_context_ptr->family_info[sstate.key.column_family_code].counter) {
          String value(sstate.value.str(), sstate.value.length());
          if (!RE2::PartialMatch(value, *(m_scan_context_ptr->value_regexp))) {
            m_queue.pop();
            sstate.scanner->forward();
            if (sstate.scanner->get(sstate.key, sstate.value))
              m_queue.push(sstate);
            continue;
          }
        }
      }

      m_delete_present = false;
      m_prev_key.set(sstate.key.row, sstate.key.flag_ptr
                     - (const uint8_t *)sstate.key.row + 1);
      m_prev_cf = sstate.key.column_family_code;
      m_revs_limit = m_scan_context_ptr->family_info[
          sstate.key.column_family_code].max_versions;
      m_cell_cutoff = m_scan_context_ptr->family_info[
          sstate.key.column_family_code].cutoff_time;

      // if counter then keep incrementing till we are ready with 1st kv pair
      if (counter) {
        // new counter
        start_count(sstate.key, sstate.value);
        forward();
        m_initialized = true;
        return;
      }
    }
    break;
  }

  m_bytes_output += m_cur_bytes;
  m_cells_input++;
  m_row_count=1;
  m_initialized = true;
}

uint64_t MergeScanner::get_disk_read() {
  uint64_t amount = m_disk_read;
  for (size_t i=0; i<m_scanners.size(); i++)
    amount += m_scanners[i]->get_disk_read();
  return amount;
}
