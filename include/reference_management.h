#pragma once

#include "hash_interface.h"

#include <map>
#include <memory>

//reference_management.h

/*
 * This file provides an interface to allow the "locking" of pages which have
 * been access by secondary-storage hash algorithms.  The page identifiers (in
 * this project: size_t) are stored in a separate, in memory hash table with the
 * amount of references to them.  The incremementing and decrementing of the
 * reference count is done through shared pointers, which are handed out upon
 * request, and when deleted use a lamba function to call back to the manager to
 * decrement the reference count.
 */

/*
 * This class helps the Hash_Interface count references to pages which have
 * references pointed to them.  The purpose of this is to prevent releasing
 * pages which to which the user has a managed reference.  See the
 * implementations to see how it may be used.
 */
namespace data_org_project_names {

template<typename Key, typename Data, typename PageId = size_t>
class ReferenceManager {
  using PageLocker = std::shared_ptr<ReferenceManager>;
 public:
  PageLocker GetPageLocker(PageId pageId) {
    ++counter[pageId];
    /*
     * Here, a lambda function passes the pageId that was used to get a
     * PageLocker, as well as a pointer to the ReferenceManager, so that when
     * the shared pointer is destroyed it can access the ReferenceManager and
     * have it decrement the count of references to pageId.
     */
    return PageLocker(this, [pageId](ReferenceManager* p) { Decrement(p, pageId); })
  }

  bool GetCount(PageId pageId) {
    return counter.count(pageId);
  }

 private:
  std::map<PageId, size_t> counter;

  static void Decrement(ReferenceManager* manager, PageId pageId) {
    if (manager->counter.count(pageId) == 0) {
      assert(false && "Decrementing a non-manages PageId");
    } else if (manager->counter.count(pageId) == 1) {
      manager->counter.erase(pageId);
    } else {
      --manager->counter[pageId];
    }
  }
}

template<typename Key, typename Data, typename PageId = size_t>
class ManagedRecordReference {
  using PageLocker = std::shared_ptr<size_t>
 public:

  ManagedRecordReference() : pRecord(nullptr) {}

  ManagedRecordReference(Record<Key, Data>* pRecord, 
      std::shared_ptr<PageId> pageLocker = std::shared_ptr<PageId>(nullptr)) :
      pRecord_(pRecord), pageLocker_(pageLocker) {}

  const Key& key() const {
    assert(pRecord);
    return pRecord_->key;
  }
  
  Data& Data() {
    assert(pRecord);
    return pRecord_->data;
  }

  const Data& Data() const {
    assert(pRecord);
    return pRecord_->data;
  }

  bool HasRecord() const {
    return pRecord_;
  }

 private:
  Record<Key, Data>* pRecord_;
  /*
   * This may be used to send a signal back to the Hash_Interface for keeping
   * track of references to a specific page or record.
   */
  PageLocker pageLocker_;
};


}; // namespace data_org_project_names
