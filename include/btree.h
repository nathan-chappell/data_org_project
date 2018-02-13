//btree.h
#pragma once

#include <algorithm>
#include <list>
#include <string>

#include "header_array.h"

namespace data_org_project_names {

struct BtreeHeader : HeaderBase {

  int    nodeHeight;

  bool IsLeaf() const { return nodeHeight == 0; }
  bool IsFull() const { return size == max_size; }
  bool IsHalf() const { return size >= max_size / 2; }
  /*
   * ToString
   */ 
  std::string ToString() const 
  {
    std::string base = HeaderBase::ToString();
    base.pop_back();
    return base + std::to_string(nodeHeight) + "}";
  }
  /*
   * CopyHeight
   */
  template<typename BtreeHeaderPtr>
  void
  CopyHeight(BtreeHeaderPtr ptr)
  {
    nodeHeight = ((BtreeHeader*)ptr)->nodeHeight;
  }
};

struct PathVertex {
  BtreeHeader* header;
  void* childEntry;
};

using Path = std::list<PathVertex>;

template<typename Key, typename Data>
using BtreePage = HeaderArray<BtreeHeader, Entry<Key,Data>>;


/*
 * SearchPath
 */
template<typename Key, typename InteriorDataType>
Path
GetSearchPath(
    InteriorDataType start, 
    const Key& key, 
    char*(*load_page)(InteriorDataType)) 
{
  using InteriorPage = BtreePage<Key,InteriorDataType>;
  using PageEntry    = typename InteriorPage::value_type;

  Path searchPath;
  auto pageHeader = (BtreeHeader*)load_page(start);

  //descend the tree...
  while (!pageHeader->IsLeaf())
  {
    auto page = (InteriorPage*)pageHeader;


    auto nextEntry = 
      std::lower_bound(
          page->begin(),
          page->end(),
          key,
          [key](const PageEntry& entry) { return entry.key <= key; }
      );

    searchPath.push_back({pageHeader, nextEntry});

    pageHeader = (BtreeHeader*)load_page(nextEntry->data);
  }

  searchPath.push_back({pageHeader, nullptr});
  return searchPath;
}
/*
 * SplitBtreeNode
 */
template<
  typename Key,
  typename Data1,
  typename Data2
  >
void 
SplitBtreeNode(
    BtreePage<Key, Data1>* parentNode,
    Entry<    Key, Data1>* childEntry,
    BtreePage<Key, Data2>*  childPage,
    BtreeHeader* newPage)
{
  newPage->CopyHeight(childPage);

  //last n/2 go from childPage to newPage
  SpliceLastN<Entry>( 
      (BtreePage<Key,Data1>*)newPage,
      childPage,
      childPage->size()/2
  );
  
  parentNode->insert(childEntry, *childEntry);

  childEntry->key = (--childPage->end())->key;

  ++childEntry; //childEntry now points to "new entry"

  childEntry->data = newPage->pageId;
}
/*
 * MergeNode
 */
template<
  typename Key,
  typename Data1,
  typename Data2
  >
void
MergeNode(
    BtreePage<Key, Data1>* parent, 
    Entry<Key, Data1>*     leftEntry,
    BtreePage<Key, Data2>* leftPage,
    BtreePage<Key, Data2>* rightPage)
{
  SpliceLastN(leftPage, rightPage, rightPage->size());

  Entry<Key, Data1>* hiEntry = leftEntry + 1;

  leftEntry->key = hiEntry->key;

  parent->erase(rightPage);
}

}; // data_org_project_names
