// Copyright (c) 2013, Cloudera, inc.
#ifndef KUDU_TPCH_LINE_ITEM_TSV_IMPORTER_H
#define KUDU_TPCH_LINE_ITEM_TSV_IMPORTER_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "benchmarks/tpch/tpch-schemas.h"
#include "common/partial_row.h"
#include "gutil/strings/split.h"
#include "gutil/strings/stringpiece.h"
#include "util/status.h"

namespace kudu {

static const char* const kPipeSeparator = "|";

// Utility class used to parse the lineitem tsv file
class LineItemTsvImporter {
 public:
  explicit LineItemTsvImporter(const string &path) : in_(path.c_str()),
    updated_(false) {
    CHECK(in_.is_open()) << "not able to open input file: " << path;
  }

  bool HasNextLine() {
    if (!updated_) {
      done_ = !getline(in_, line_);
      updated_ = true;
    }
    return !done_;
  }

  // Fills the row builder with a single line item from the file.
  // It returns 0 if it's done or the order number if it got a line
  int GetNextLine(KuduPartialRow* row) {
    if (!HasNextLine()) return 0;
    columns_.clear();

    // grab all the columns_ individually
    // Note that columns_ refers, and does not copy, the data in line_
    columns_ = strings::Split(line_, kPipeSeparator);

    // The row copies all indirect data from columns_. This must be done
    // because callers expect to retrieve lines repeatedly before flushing
    // the accumulated rows in a batch.
    int i = 0;
    int order_number = ConvertToIntAndPopulate(columns_[i++], row, tpch::kOrderKeyColIdx);
    ConvertToIntAndPopulate(columns_[i++], row, tpch::kPartKeyColIdx);
    ConvertToIntAndPopulate(columns_[i++], row, tpch::kSuppKeyColIdx);
    ConvertToIntAndPopulate(columns_[i++], row, tpch::kLineNumberColIdx);
    ConvertToIntAndPopulate(columns_[i++], row, tpch::kQuantityColIdx);
    ConvertDoubleToIntAndPopulate(columns_[i++], row, tpch::kExtendedPriceColIdx);
    ConvertDoubleToIntAndPopulate(columns_[i++], row, tpch::kDiscountColIdx);
    ConvertDoubleToIntAndPopulate(columns_[i++], row, tpch::kTaxColIdx);
    CHECK_OK(row->SetStringCopy(tpch::kReturnFlagColIdx, columns_[i++]));
    CHECK_OK(row->SetStringCopy(tpch::kLineStatusColIdx, columns_[i++]));
    CHECK_OK(row->SetStringCopy(tpch::kShipDateColIdx, columns_[i++]));
    CHECK_OK(row->SetStringCopy(tpch::kCommitDateColIdx, columns_[i++]));
    CHECK_OK(row->SetStringCopy(tpch::kReceiptDateColIdx, columns_[i++]));
    CHECK_OK(row->SetStringCopy(tpch::kShipInstructColIdx, columns_[i++]));
    CHECK_OK(row->SetStringCopy(tpch::kShipModeColIdx, columns_[i++]));
    CHECK_OK(row->SetStringCopy(tpch::kCommentColIdx, columns_[i++]));

    updated_ = false;

    return order_number;
  }

  int ConvertToIntAndPopulate(const StringPiece &chars, KuduPartialRow* row,
                              int col_idx) {
    // TODO: extra copy here, since we don't have a way to parse StringPiece
    // into ints.
    chars.CopyToString(&tmp_);
    int number;
    bool ok_parse = SimpleAtoi(tmp_.c_str(), &number);
    CHECK(ok_parse);
    CHECK_OK(row->SetUInt32(col_idx, number));
    return number;
  }

  void ConvertDoubleToIntAndPopulate(const StringPiece &chars, KuduPartialRow* row,
                                     int col_idx) {
    // TODO: extra copy here, since we don't have a way to parse StringPiece
    // into ints.
    chars.CopyToString(&tmp_);
    char *error = NULL;
    errno = 0;
    const char *cstr = tmp_.c_str();
    double number = strtod(cstr, &error);
    CHECK(errno == 0 &&  // overflow/underflow happened
        error != cstr);
    int new_num = number * 100;
    CHECK_OK(row->SetUInt32(col_idx, new_num));
  }

 private:
  std::ifstream in_;
  vector<StringPiece> columns_;
  string line_, tmp_;
  bool updated_, done_;
};
} // namespace kudu
#endif
