/*
 * Copyright ©2018 Hal Perkins.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Autumn Quarter 2018 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <iostream>
#include <algorithm>

#include "./QueryProcessor.h"

extern "C" {
  #include "./libhw1/CSE333.h"
}

namespace hw3 {

// given two lists of docid element headers,
// finds the intersection between them and returns it
// as a list. an intersection is defined as an element
// having the same docid. A resulting intersected list
// element has its two ranks (num positions) summed up
static list<docid_element_header> GetListIntersection(
                                    const list<docid_element_header>& l1,
                                    const list<docid_element_header>& l2);

QueryProcessor::QueryProcessor(list<string> indexlist, bool validate) {
  // Stash away a copy of the index list.
  indexlist_ = indexlist;
  arraylen_ = indexlist_.size();
  Verify333(arraylen_ > 0);

  // Create the arrays of DocTableReader*'s. and IndexTableReader*'s.
  dtr_array_ = new DocTableReader *[arraylen_];
  itr_array_ = new IndexTableReader *[arraylen_];

  // Populate the arrays with heap-allocated DocTableReader and
  // IndexTableReader object instances.
  list<string>::iterator idx_iterator = indexlist_.begin();
  for (HWSize_t i = 0; i < arraylen_; i++) {
    FileIndexReader fir(*idx_iterator, validate);
    dtr_array_[i] = new DocTableReader(fir.GetDocTableReader());
    itr_array_[i] = new IndexTableReader(fir.GetIndexTableReader());
    idx_iterator++;
  }
}

QueryProcessor::~QueryProcessor() {
  // Delete the heap-allocated DocTableReader and IndexTableReader
  // object instances.
  Verify333(dtr_array_ != nullptr);
  Verify333(itr_array_ != nullptr);
  for (HWSize_t i = 0; i < arraylen_; i++) {
    delete dtr_array_[i];
    delete itr_array_[i];
  }

  // Delete the arrays of DocTableReader*'s and IndexTableReader*'s.
  delete[] dtr_array_;
  delete[] itr_array_;
  dtr_array_ = nullptr;
  itr_array_ = nullptr;
}

vector<QueryProcessor::QueryResult>
QueryProcessor::ProcessQuery(const vector<string> &query) {
  Verify333(query.size() > 0);
  vector<QueryProcessor::QueryResult> finalresult;

  // MISSING:
  // loop through each index of the QueryProcessor
  for (uint32_t i = 0; i < arraylen_; i++) {
      // get the doc and index table readers for each given index file
      DocTableReader* doc_reader = dtr_array_[i];
      IndexTableReader* index_reader = itr_array_[i];

      // get documents that contain the first word of query
      DocIDTableReader* docid_reader = index_reader->LookupWord(query[0]);

      // if no match in current index file, search next one
      if (docid_reader == NULL) {
          continue;
      }

      // we have a match! get list of docIds containing first
      // query word, along with its ranks
      list<docid_element_header> docid_header = docid_reader->GetDocIDList();

      // delete docid_reader as we are done with it for now
      delete docid_reader;

      // if we have no results, move on to next index
      if (docid_header.size() == 0) {
          continue;
      }

      // we have results! now check if next word's docid_header in query
      // intersected (since we only want to include documents that contain
      // all the words) result list is greater than 0 if not, move on to
      // next index
      list<docid_element_header> running_intersection = docid_header;
      for (uint32_t i = 1; i < query.size(); i++) {
          // pretty much do the same thing we did with query[0].
          // we just did that separate because it is a fencepost situation
          docid_reader = index_reader->LookupWord(query[i]);
          if (docid_reader == NULL) {
              running_intersection.clear();
              break;
          }

          list<docid_element_header> current_list = docid_reader
                                                    ->GetDocIDList();
          delete docid_reader;

          running_intersection = GetListIntersection(running_intersection,
                                                     current_list);

          if (running_intersection.size() == 0) {
              break;
          }
      }

      // now iterate through intersection of all query words for
      // this current index, creating query results out of it
      if (running_intersection.size() != 0) {
          string doc_name;
          list<docid_element_header>::iterator itr;
          for (itr = running_intersection.begin();
               itr != running_intersection.end(); itr++) {
            Verify333(doc_reader->LookupDocID(itr->docid, &doc_name));
            QueryResult query_res;
            query_res.document_name = doc_name;
            query_res.rank = itr->num_positions;
            finalresult.push_back(query_res);
          }
      }
  }


  // Sort the final results.
  std::sort(finalresult.begin(), finalresult.end());
  return finalresult;
}

static list<docid_element_header> GetListIntersection(
                                const list<docid_element_header>& l1,
                                const list<docid_element_header>& l2) {
    list<docid_element_header>::const_iterator itr1;
    list<docid_element_header>::const_iterator itr2;
    list<docid_element_header> result;

    // loop through, checking if element in 1st list has a match
    // in the second list. if it does, add ranks and add to result
    for (itr1 = l1.begin(); itr1 != l1.end(); itr1++) {
        for (itr2 = l2.begin(); itr2 != l2.end(); itr2++) {
            if (itr1->docid == itr2->docid) {
                docid_element_header sum_header;
                sum_header.docid = itr1->docid;
                sum_header.num_positions = itr1->num_positions +
                                           itr2->num_positions;
                result.push_back(sum_header);
                break;
            }
        }
    }
    return result;
}

}  // namespace hw3
