//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// transaction.cpp
//
// Identification: src/concurrency/transaction.cpp
//
// Copyright (c) 2015-16, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//


#include "concurrency/transaction.h"

#include "common/logger.h"
#include "common/platform.h"
#include "common/macros.h"

#include <chrono>
#include <thread>
#include <iomanip>

namespace peloton {
namespace concurrency {

/*
 * Transaction state transition:
 *                r           r/ro            u/r/ro
 *              +--<--+     +---<--+        +---<--+
 *           r  |     |     |      |        |      |     d
 *  (init)-->-- +-> Read  --+-> Read Own ---+--> Update ---> Delete (final)
 *                    |   ro             u  |
 *                    |                     |
 *                    +----->--------->-----+
 *                              u
 *              r/ro/u
 *            +---<---+
 *         i  |       |     d
 *  (init)-->-+---> Insert ---> Ins_Del (final)
 *
 *    r: read
 *    ro: read_own
 *    u: update
 *    d: delete
 *    i: insert
 */

RWType Transaction::GetRWType(const ItemPointer &location) {
  oid_t tile_group_id = location.block;
  oid_t tuple_id = location.offset;
  auto itr = rw_set_.find(tile_group_id);
  if (itr == rw_set_.end()) {
    return RW_TYPE_INVALID;
  }

  auto inner_itr = itr->second.find(tuple_id);
  if (inner_itr == itr->second.end()) {
    return RW_TYPE_INVALID;
  }

  return inner_itr->second;
}

void Transaction::RecordRead(const ItemPointer &location) {
  oid_t tile_group_id = location.block;
  oid_t tuple_id = location.offset;

  if (rw_set_.find(tile_group_id) != rw_set_.end() &&
      rw_set_.at(tile_group_id).find(tuple_id) !=
          rw_set_.at(tile_group_id).end()) {
    PL_ASSERT(rw_set_.at(tile_group_id).at(tuple_id) != RW_TYPE_DELETE &&
              rw_set_.at(tile_group_id).at(tuple_id) != RW_TYPE_INS_DEL);
    return;
  } else {
    rw_set_[tile_group_id][tuple_id] = RW_TYPE_READ;
  }
}

void Transaction::RecordReadOwn(const ItemPointer &location) {
  oid_t tile_group_id = location.block;
  oid_t tuple_id = location.offset;

  if (rw_set_.find(tile_group_id) != rw_set_.end() &&
      rw_set_.at(tile_group_id).find(tuple_id) !=
      rw_set_.at(tile_group_id).end()) {
    RWType &type = rw_set_.at(tile_group_id).at(tuple_id);
    if (type == RW_TYPE_READ) {
      type = RW_TYPE_READ_OWN;
      // record write.
      return;
    }
    PL_ASSERT(type != RW_TYPE_DELETE && type != RW_TYPE_INS_DEL);
  } else {
    rw_set_[tile_group_id][tuple_id] = RW_TYPE_READ_OWN;
  }
}

void Transaction::RecordUpdate(const ItemPointer &location) {
  oid_t tile_group_id = location.block;
  oid_t tuple_id = location.offset;

  if (rw_set_.find(tile_group_id) != rw_set_.end() &&
      rw_set_.at(tile_group_id).find(tuple_id) !=
          rw_set_.at(tile_group_id).end()) {
    RWType &type = rw_set_.at(tile_group_id).at(tuple_id);
    if (type == RW_TYPE_READ || type == RW_TYPE_READ_OWN) {
      type = RW_TYPE_UPDATE;
      // record write.
      is_written_ = true;

      return;
    }
    if (type == RW_TYPE_UPDATE) {
      return;
    }
    if (type == RW_TYPE_INSERT) {
      return;
    }
    if (type == RW_TYPE_DELETE) {
      PL_ASSERT(false);
      return;
    }
    PL_ASSERT(false);
  }
}

void Transaction::RecordInsert(const ItemPointer &location) {
  oid_t tile_group_id = location.block;
  oid_t tuple_id = location.offset;

  if (rw_set_.find(tile_group_id) != rw_set_.end() &&
      rw_set_.at(tile_group_id).find(tuple_id) !=
          rw_set_.at(tile_group_id).end()) {
    PL_ASSERT(false);
  } else {
    rw_set_[tile_group_id][tuple_id] = RW_TYPE_INSERT;
    ++insert_count_;

  }
}

bool Transaction::RecordDelete(const ItemPointer &location) {
  oid_t tile_group_id = location.block;
  oid_t tuple_id = location.offset;

  if (rw_set_.find(tile_group_id) != rw_set_.end() &&
      rw_set_.at(tile_group_id).find(tuple_id) !=
          rw_set_.at(tile_group_id).end()) {
    RWType &type = rw_set_.at(tile_group_id).at(tuple_id);
    if (type == RW_TYPE_READ || type == RW_TYPE_READ_OWN) {
      type = RW_TYPE_DELETE;
      // record write.
      is_written_ = true;

      return false;
    }
    if (type == RW_TYPE_UPDATE) {
      type = RW_TYPE_DELETE;

      return false;
    }
    if (type == RW_TYPE_INSERT) {
      type = RW_TYPE_INS_DEL;
      --insert_count_;

      return true;
    }
    if (type == RW_TYPE_DELETE) {
      PL_ASSERT(false);
      return false;
    }
    PL_ASSERT(false);
  } else {
    PL_ASSERT(false);
  }
  return false;
}

const std::string Transaction::GetInfo() const {
  std::ostringstream os;

  os << "\tTxn :: @" << this << " ID : " << std::setw(4) << txn_id_
     << " Begin Commit ID : " << std::setw(4) << begin_cid_
     << " End Commit ID : " << std::setw(4) << end_cid_
     << " Result : " << result_;

  return os.str();
}

}  // End concurrency namespace
}  // End peloton namespace
