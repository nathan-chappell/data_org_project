#pragma once
/*
 * Fagin's extendible hashing
 */

#include <array>
#include <cassert>
#include <cstring>
#include <iterator>
#include <set>
#include <vector>

#include "hash_interface.h"
#include "header_array.h"
#include "storage_model.h"
#include "universal_hash.h"

namespace data_org_project_names {

using PageId = size_t;

struct FaginHeader : HeaderBase {
  size_t localDepth;

  std::string ToString() const 
  {
    std::string base = HeaderBase::ToString();
    base.pop_back();
    return base + std::to_string(localDepth) + "}";
  }
};

template<typename Key, typename Data>
using FaginPage = HeaderArray<FaginHeader, Entry<Key,Data>>;


template<
  typename Key,
  typename Hash = UniHash<Key>
  >
class FaginDirectory {
 public:
  using Directory = std::vector<PageId>;
  FaginDirectory() : directory_(0) {}

	/*
	 * Contract
	 */
  void
  Contract()
  {
    directory_.resize(directory_.size() / 2);
  }
	/*
	 * Expand
	 */
  void
  Expand(const Key& key)
  {
    size_t oldSize = directory_.size();
    directory_.resize(oldSize*2);

    std::copy(
        directory_.begin(),           //first
        directory_.begin() + oldSize, //last
        directory_.begin() + oldSize  //d_first
    );
  }
	/*
	 * GlobalDepth
	 */
  size_t
  GlobalDepth() const
  {
    return llrint(log2(directory_.size()));
  }
	/*
	 * GetPageId
	 */
  PageId
  GetPageId(const Key& key) const
  {
    return directory_[hash_(key) % directory_.size()];
  }
	/*
	 * Initialize
	 */
  void
  Initialize(PageId pageId, size_t n)
  {
    directory_.resize(n, pageId);
  }
	/*
	 * SetNewPage
	 */
  void
  SetNewPage(
      const Key& key,
      size_t localDepth,
      size_t pageId)
  {
    size_t i = hash_(key) % localDepth + localDepth;
    while (i < directory_.size())
    {
      directory_[i] = pageId;
      i += localDepth;
    }
  }
  /*
   * DirBegin
   */
  Directory::iterator DirBegin()  { return directory_.begin(); }
  Directory::iterator DirEnd()    { return directory_.end(); }
  Directory::reverse_iterator DirRBegin() { return directory_.rbegin(); }
  Directory::reverse_iterator DirREnd()   { return directory_.rend(); }

 private:

  Hash      hash_;
  Directory directory_;
};

template<
  typename Key,
  typename Data,
  typename Hash = UniHash<Key>
  >
class FaginTable /*: public HashInterface<Key,Data>*/ {
 public:
  using Page      = FaginPage<Key, Data>;
  using PageEntry = Entry<Key, Data>;
  using Header    = FaginHeader;
  using Table     = FaginTable<Key, Data, Hash>;

  class PageIterator;
  using iterator  = TableIterator<Key, Data, Entry, PageIterator>;
  friend iterator;

  /*
   * FaginTable
   */
  FaginTable(storage_model* model, size_t n = 0) : model_(model)
  {
    PageId initId = model_->create_page();
    auto initPage = (FaginHeader*)model_->load_page(initId);

    InitializeHeader<FaginHeader, Entry<const Key,Data> >(
        initPage,
        model_->get_page_size(),
        initId
    );

    model_->release_page(initId);
    directory_.Initialize(initId, n);
  }
  /*
   * erase
   */
  bool
  erase(const Key& key) //override
  {
    PageId pageId = directory_.GetPageId(key);
    auto page = (Page*)model_->load_page(pageId);

    auto ePoint = page->find(
        [key](const PageEntry& entry) { return key == entry.key; }
    );

    if (ePoint == page->end()) return false;

    --size_;
    page->erase(ePoint);

    return true;
  }
  /*
   * find
   */
  iterator
  find(const Key& key) //override
  {
    PageId pageId = directory_.GetPageId(key);
    auto page = (Page*)model_->load_page(pageId);

    auto keyLocation = page->find(
        [key](const PageEntry& entry) { return key == entry.key; }
    );

    return {keyLocation, PageIterator(page, this)};
  }
  /*
   * insert
   */
  void
  insert(const Key& key, const Data& data) //override
  {
    PageId pageId = directory_.GetPageId(key);
    auto page = (Page*)model_->load_page(pageId);

    while (page->full()) 
    {
      SplitPage(page, pageId, key);
      pageId = directory_.GetPageId(key);
      page = (Page*)model_->load_page(pageId);
    }

    auto iPoint = page->find(
        [key] (const Entry<Key, Data>& entry) { 
        return entry.key == key;
    });

    page->insert(iPoint, {key, data});
    ++size_;
  }
  /*
   * begin
   */
  iterator 
  begin()
  {
    Page* page = model_->load_page(*directory_->DirBegin());
    return {page->begin(), page, this};
  }
  /*
   * end
   */
  iterator end() { return {nullptr, {nullptr, this}}; }
  const iterator end() const { return {nullptr, nullptr, this}; }


  /*
   * Successor
   */
  iterator&
  Predecessor(iterator& it)
  {
    if (it.entry_ != it.page_->begin())
    {
      --it.entry_;
    }
    else
    {
      auto dirIt = NextUnique(
          directory_->DirRBegin(),
          directory_->DirREnd(),
          it.page_->header()->pageId
      );

      if (dirIt == directory_->DirREnd())
      {
        it.page_ = nullptr;
        it.entry_ = nullptr;
      }
      else
      {
        it.page_ = (Page*)model_->load_page(*dirIt);
        it.entry_ = it.page_->begin();
      }
    }

    return *this;
  }

 private:

  /*
   * SplitPage
   */
  void SplitPage(Page* page, PageId pageId, const Key& key)
  {
    if (++page->header()->localDepth == directory_.GlobalDepth()) {
      directory_.Expand(key);
    }

    directory_.SetNewPage(key, page->header()->localDepth, pageId);

    ReinsertAllEntries(page);
  }
  /*
   * ReinsertAllEntries
   */
  void ReinsertAllEntries(Page* page) 
  {
    std::vector<PageEntry> entries;

    while (!page->empty())
    {
      entries.push_back(*page->begin());
      page->erase(page->begin());
    }

    while (!entries.empty())
    {
      insert(entries.back().key, entries.back().data);
      entries.pop_back();
    }
  }

 public:

  class PageIterator : public PageIteratorBase<Page, Table> {
    public:
      using Base = PageIteratorBase<Page, Table>;
      using Base::page_;
      using Base::table_;

      PageIterator(Page* page, Table* table) : Base(page, table) {}

      void operator++() override {
        storage_model* model_  = table_->model_;
        FaginDirectory<Key,Hash>& directory_ = table_->directory_;

        if (directory_.DirBegin() == directory_.DirEnd()) {
          page_ = nullptr;
        }
        else if (page_ == nullptr)
        {
          page_ = (Page*)model_->load_page(*directory_.DirBegin());
        }
        else
        {
          auto dirIt = NextUnique(
              directory_.DirBegin(),
              directory_.DirEnd(),
              page_->header()->pageId
              );

          if (dirIt == directory_.DirEnd())
          {
            page_ = nullptr;
          }
          else
          {
            page_ = (Page*)model_->load_page(*dirIt);
          }
        }
      }
      void operator--() override {
        storage_model* model_  = table_->model_;
        FaginDirectory<Key,Hash>& directory_ = table_->directory_;

        if (directory_.DirRBegin() == directory_.DirREnd()) {
          page_ = nullptr;
        }
        else if (page_ == nullptr)
        {
          page_ = (Page*)model_->load_page(*directory_.DirRBegin());
        }
        else
        {
          auto dirIt = NextUnique(
              directory_.DirRBegin(),
              directory_.DirREnd(),
              page_->header()->pageId
              );

          if (dirIt == directory_.DirREnd())
          {
            page_ = nullptr;
          }
          else
          {
            page_ = (Page*)model_->load_page(*dirIt);
          }
        }
      }
 
  };


  storage_model*            model_;
  FaginDirectory<Key, Hash> directory_;
  size_t                    size_;
};


}; //data_org_project_names 
