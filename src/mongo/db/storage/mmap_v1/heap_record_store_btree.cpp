// heap_record_store_btree.cpp

/**
 *    Copyright (C) 2014 MongoDB Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#include "mongo/db/storage/mmap_v1/heap_record_store_btree.h"

#include "mongo/util/log.h"
#include "mongo/util/mongoutils/str.h"

namespace mongo {

    RecordData HeapRecordStoreBtree::dataFor(const DiskLoc& loc) const {
        Records::const_iterator it = _records.find(loc);
        invariant(it != _records.end());
        const Record& rec = it->second;

        return RecordData(rec.data.get(), rec.dataSize);
    }

    void HeapRecordStoreBtree::deleteRecord(OperationContext* txn, const DiskLoc& loc) {
        invariant(_records.erase(loc) == 1);
    }

    StatusWith<DiskLoc> HeapRecordStoreBtree::insertRecord(OperationContext* txn,
                                                           const char* data,
                                                           int len,
                                                           bool enforceQuota) {
        Record rec(len);
        memcpy(rec.data.get(), data, len);

        const DiskLoc loc = allocateLoc();
        _records[loc] = rec;

        return StatusWith<DiskLoc>(loc);
    }

    StatusWith<DiskLoc> HeapRecordStoreBtree::insertRecord(OperationContext* txn,
                                                           const DocWriter* doc,
                                                           bool enforceQuota) {
        Record rec(doc->documentSize());
        doc->writeDocument(rec.data.get());

        const DiskLoc loc = allocateLoc();
        _records[loc] = rec;

        return StatusWith<DiskLoc>(loc);
    }

    DiskLoc HeapRecordStoreBtree::allocateLoc() {
        const int64_t id = _nextId++;
        // This is a hack, but both the high and low order bits of DiskLoc offset must be 0, and the
        // file must fit in 23 bits. This gives us a total of 30 + 23 == 53 bits.
        invariant(id < (1LL << 53));
        return DiskLoc(int(id >> 30), int((id << 1) & ~(1<<31)));
    }

    Status HeapRecordStoreBtree::touch(OperationContext* txn, BSONObjBuilder* output) const {
        // not currently called from the tests, but called from btree_logic.h
        return Status::OK();
    }

} // namespace mongo
