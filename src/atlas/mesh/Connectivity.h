/*
 * (C) Copyright 1996-2017 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

/// @file Connectivity.h
/// @author Willem Deconinck
/// @date October 2015
/// This file contains Connectivity classes
///   - IrregularConnectivity
///   - BlockConnectivity
///   - MultiBlockConnectivity
///
/// It is important to note that connectivity access functions are
/// inlined for optimal performance. The connectivity itself is internally
/// stored with base 1, to be compatible with Fortran access.
/// C++ access operators however convert the resulting connectivity to base 0.

#ifndef atlas_Connectivity_H
#define atlas_Connectivity_H

#include "eckit/memory/Owned.h"
#include "eckit/memory/SharedPtr.h"
#include "atlas/internals/atlas_config.h"
#include "atlas/array/Array.h"
#include "atlas/array/ArrayView.h"
#include "atlas/array/IndexView.h"
#include "atlas/array/DataType.h"
#include "atlas/array/Vector.h"
#include "atlas/array/array_fwd.h"

namespace atlas {
namespace mesh {

// Classes defined in this file:
class IrregularConnectivity;
class BlockConnectivity;
class MultiBlockConnectivity;

// --------------------------------------------------------------------------

#ifdef ATLAS_HAVE_FORTRAN
#define INDEX_REF Index
#define FROM_FORTRAN -1
#define TO_FORTRAN   +1
#else
#define INDEX_REF *
#define FROM_FORTRAN
#define TO_FORTRAN
#endif

/// @brief Irregular Connectivity Table
/// @author Willem Deconinck
///
/// Container for irregular connectivity tables. This is e.g. the case
/// for a node-to-X connectivity.
///   connectivity = [
///     1   2   3   4   5   6       # node 1
///     7   8                       # node 2
///     9  10  11  12               # node 3
///    13  14  15                   # node 4
///    16  17  18  19  20  21       # node 5
///   ]
/// There are 2 modes of construction:
///   - It wraps existing raw data
///   - It owns the connectivity data
///
/// In case ATLAS_HAVE_FORTRAN is defined (which is usually the case),
/// the raw data will be stored with base 1 for Fortran interoperability.
/// The operator(row,col) will then do the conversion to base 0.
///
/// In the first mode of construction, the connectivity table cannot be resized.
/// In the second mode of construction, resizing is possible

namespace detail {
// FortranIndex:
// Helper class that does +1 and -1 operations on stored values

class ConnectivityIndex
{
public:
  enum { BASE = 1 };
public:
  ConnectivityIndex(idx_t* idx): idx_(idx) {}
  void set(const idx_t& value) { *(idx_) = value+BASE; }
  idx_t get() const { return *(idx_)-BASE; }
  void operator=(const idx_t& value) { set(value); }
  ConnectivityIndex& operator=(const ConnectivityIndex& other) { set(other.get()); return *this; }
  ConnectivityIndex& operator+(const idx_t& value) { *(idx_)+=value; return *this; }
  ConnectivityIndex& operator-(const idx_t& value) { *(idx_)-=value; return *this; }
  ConnectivityIndex& operator--() { --(*(idx_)); return *this; }
  ConnectivityIndex& operator++() { ++(*(idx_)); return *this; }

  //implicit conversion
  operator idx_t() const { return get(); }

private:
  idx_t* idx_;
};
}

class ConnectivityRow
{
  #ifdef ATLAS_HAVE_FORTRAN
    typedef detail::ConnectivityIndex Index;
  #else
    typedef idx_t Index;
  #endif

public:

    GT_FUNCTION
    ConnectivityRow(idx_t *data, size_t size) : data_(data), size_(size) {}

    GT_FUNCTION
    idx_t operator()(size_t i) const { return data_[i] FROM_FORTRAN; }

    GT_FUNCTION
    Index operator()(size_t i)       { return INDEX_REF(data_+i); }

    GT_FUNCTION
    size_t size() const { return size_; }

  private:
    idx_t *data_;
    size_t size_;
};

class IrregularConnectivity : public eckit::Owned
{
public:
  typedef ConnectivityRow Row;

  static constexpr unsigned short _values_=0;
  static constexpr unsigned short _displs_=1;
  static constexpr unsigned short _counts_=2;
public:
//-- Constructors

  /// @brief Construct connectivity table that needs resizing a-posteriori
  /// Data is owned
  IrregularConnectivity( const std::string& name = "" );

  /// @brief Construct connectivity table wrapping existing raw data.
  /// No resizing can be performed as data is not owned.
  IrregularConnectivity( idx_t values[], size_t rows, size_t displs[], size_t counts[] );

  /// @brief Copy ctr (only to be used when calling a cuda kernel)
  // This ctr has to be defined in the header, since __CUDACC__ will identify whether
  // it is compiled it for a GPU kernel
  GT_FUNCTION
  IrregularConnectivity(const IrregularConnectivity &other) :
    owns_(false),
    missing_value_(other.missing_value_),
    rows_(other.rows_),
    maxcols_(other.maxcols_),
    mincols_(other.mincols_),
#ifdef __CUDACC__
    data_{0,0,0},
    values_view_(array::make_device_view<idx_t, 1>(*(other.data_[_values_]))),
    displs_view_(array::make_device_view<size_t, 1>(*(other.data_[_displs_]))),
    counts_view_(array::make_device_view<size_t, 1>(*(other.data_[_counts_]))),
#else
    data_{other.data_[0], other.data_[1], other.data_[2]},
    values_view_(array::make_host_view<idx_t, 1>(*(other.data_[_values_]))),
    displs_view_(array::make_host_view<size_t, 1>(*(other.data_[_displs_]))),
    counts_view_(array::make_host_view<size_t, 1>(*(other.data_[_counts_]))),
#endif
    ctxt_delete_(0),
    ctxt_set_(0),
    ctxt_update_(0)
  {}


  ~IrregularConnectivity();

//-- Accessors

  /// @brief Name associated to this Connetivity
  const std::string& name() const { return name_; }

  /// @brief Rename this Connectivity
  void rename(const std::string& name) { name_ = name; }

  /// @brief Number of rows in the connectivity table
  GT_FUNCTION
  size_t rows() const { return rows_; }

  /// @brief Number of columns for specified row in the connectivity table
  GT_FUNCTION
  size_t cols( size_t row_idx ) const { return counts_view_(row_idx); }

  /// @brief Maximum value for number of columns over all rows
  GT_FUNCTION
  size_t maxcols() const { return maxcols_; }

  /// @brief Minimum value for number of columns over all rows
  GT_FUNCTION
  size_t mincols() const { return mincols_; }

  /// @brief Access to connectivity table elements for given row and column
  /// The returned index has base 0 regardless if ATLAS_HAVE_FORTRAN is defined.
  GT_FUNCTION
  idx_t operator()( size_t row_idx, size_t col_idx ) const;

  /// @brief Access to raw data.
  /// Note that the connectivity base is 1 in case ATLAS_HAVE_FORTRAN is defined.
  const idx_t* data() const { return values_view_.data(); }
        idx_t* data()       { return values_view_.data(); }

  size_t size() const { return values_view_.size();}

  GT_FUNCTION
  idx_t missing_value() const { return missing_value_; }

  GT_FUNCTION
  Row row( size_t row_idx ) const;

///-- Modifiers

  /// @brief Modify row with given values. Values must be given with base 0
  void set( size_t row_idx, const idx_t column_values[] );

  /// @brief Modify (row,col) with given value. Value must be given with base 0
  void set( size_t row_idx, size_t col_idx, const idx_t value );

  /// @brief Resize connectivity
  /// @note Can only be used when data is owned.
  virtual void resize(size_t old_size, size_t size, bool initialize, const idx_t values[], bool fortran_array);

  /// @brief Resize connectivity, and add given rows
  /// @note Can only be used when data is owned.
  virtual void add( size_t rows, size_t cols, const idx_t values[], bool fortran_array=false );

  /// @brief Resize connectivity, and add given rows with missing values
  /// @note Can only be used when data is owned.
  virtual void add( size_t rows, size_t cols );

  /// @brief Resize connectivity, and add given rows with missing values
  /// @note Can only be used when data is owned.
  virtual void add( size_t rows, const size_t cols[] );

  /// @brief Resize connectivity, and copy from a BlockConnectivity
  virtual void add( const BlockConnectivity& block );

  /// @brief Resize connectivity, and insert given rows
  /// @note Can only be used when data is owned.
  virtual void insert( size_t position, size_t rows, size_t cols, const idx_t values[], bool fortran_array=false );

  /// @brief Resize connectivity, and insert given rows with missing values
  /// @note Can only be used when data is owned.
  virtual void insert( size_t position, size_t rows, size_t cols );

  /// @brief Resize connectivity, and insert given rows with missing values
  /// @note Can only be used when data is owned.
  virtual void insert( size_t position, size_t rows, const size_t cols[] );

  virtual void clear();

  virtual size_t footprint() const;

  size_t displs(const size_t row) const {return displs_view_(row); }

  virtual void cloneToDevice() const;
  virtual void cloneFromDevice() const;
  virtual void syncHostDevice() const;
  virtual bool valid() const;
  virtual bool isOnHost() const;
  virtual bool isOnDevice() const;

  void dump(std::ostream& os) const;

protected:
  bool owns() { return owns_; }
  const size_t *displs() const { return displs_view_.data(); }
  const size_t *counts() const { return counts_view_.data(); }

private:

  void on_delete();
  void on_update();

private:
  std::string name_;

  bool owns_;
  mutable std::array<array::Array*, 3> data_;

  mutable array::ArrayView<idx_t, 1> values_view_;
  mutable array::ArrayView<size_t,1> displs_view_;
  mutable array::ArrayView<size_t,1> counts_view_;

  idx_t  missing_value_;
  size_t rows_;
  size_t maxcols_;
  size_t mincols_;

public:
  typedef void* ctxt_t;
  typedef void (*callback_t)(ctxt_t);
private:
  friend class ConnectivityPrivateAccess;
  ctxt_t     ctxt_update_;
  ctxt_t     ctxt_set_;
  ctxt_t     ctxt_delete_;
  callback_t callback_update_;
  callback_t callback_set_;
  callback_t callback_delete_;
};

typedef IrregularConnectivity Connectivity;

// ----------------------------------------------------------------------------------------------

/// @brief MultiBlockConnectivity Table
/// @author Willem Deconinck
///
/// Container for connectivity tables that are layed out in memory as multiple BlockConnectivities stitched together.
/// This is e.g. the case for a element-node connectivity, with element-types grouped together:
///  connectivity = [
///     # triangles  (block 0)
///         1   2   3
///         4   5   6
///         7   8   9
///        10  11  12
///     # quadrilaterals (block 1)
///        13  14  15  16
///        17  18  19  20
///        21  22  23  24
///  ]
///
/// This class can also be interpreted as a atlas::IrregularConnectivity without distinction between blocks
///
/// There are 2 modes of construction:
///   - It wraps existing raw data
///   - It owns the connectivity data
///
/// In case ATLAS_HAVE_FORTRAN is defined (which is usually the case),
/// the raw data will be stored with base 1 for Fortran interoperability.
/// The operator(row,col) will then do the conversion to base 0.
///
/// In the first mode of construction, the connectivity table cannot be resized.
/// In the second mode of construction, resizing is possible
class MultiBlockConnectivity : public IrregularConnectivity
{
public:

//-- Constructors

  /// @brief Construct connectivity table that needs resizing a-posteriori
  /// Data is owned
  MultiBlockConnectivity( const std::string& name = "" );

/*
  /// @brief Construct connectivity table wrapping existing raw data.
  /// No resizing can be performed as data is not owned.
  MultiBlockConnectivity(
      idx_t values[],
      size_t rows,
      size_t displs[],
      size_t counts[],
      size_t blocks, size_t block_displs[],
      size_t block_cols[] );
*/
  ~MultiBlockConnectivity();

//-- Accessors

  /// @brief Number of blocks
  size_t blocks() const { return blocks_; }

  /// @brief Access to a block connectivity
  const BlockConnectivity& block( size_t block_idx ) const { return *(block_[block_idx]); }
        BlockConnectivity& block( size_t block_idx )       { return *(block_[block_idx]); }

  /// @brief Access to connectivity table elements for given row and column
  /// The row_idx counts up from 0, from block 0, as in IrregularConnectivity
  /// The returned index has base 0 regardless if ATLAS_HAVE_FORTRAN is defined.
  idx_t operator()( size_t row_idx, size_t col_idx ) const;

  /// @brief Access to connectivity table elements for given row and column
  /// The block_row_idx counts up from zero for every block_idx.
  /// The returned index has base 0 regardless if ATLAS_HAVE_FORTRAN is defined.
  idx_t operator()( size_t block_idx, size_t block_row_idx, size_t block_col_idx ) const;

///-- Modifiers

  /// @brief Resize connectivity, and add given rows as a new block
  /// @note Can only be used when data is owned.
  virtual void add( size_t rows, size_t cols, const idx_t values[], bool fortran_array=false );

  /// @brief Resize connectivity, and copy from a BlockConnectivity to a new block
  /// @note Can only be used when data is owned.
  virtual void add( const BlockConnectivity& );

  /// @brief Resize connectivity, and add given rows with missing values
  /// @note Can only be used when data is owned.
  virtual void add( size_t rows, size_t cols );

  /// @brief Resize connectivity, and add given rows with missing values
  /// @note Can only be used when data is owned.
  virtual void add( size_t rows, const size_t cols[] );

  /// @brief Resize connectivity, and insert given rows
  /// @note Can only be used when data is owned.
  virtual void insert( size_t position, size_t rows, size_t cols, const idx_t values[], bool fortran_array=false );

  /// @brief Resize connectivity, and insert given rows with missing values
  /// @note Can only be used when data is owned.
  virtual void insert( size_t position, size_t rows, size_t cols );

  /// @brief Resize connectivity, and insert given rows with missing values
  /// @note Can only be used when data is owned.
  virtual void insert( size_t position, size_t rows, const size_t cols[] );

  virtual void clear();

  virtual size_t footprint() const;

  virtual void cloneToDevice() const;
  virtual void cloneFromDevice() const;
  virtual void syncHostDevice() const;
  virtual bool valid() const;
  virtual bool isOnHost() const;
  virtual bool isOnDevice() const;

private:

  void rebuild_block_connectivity();

private:
  array::Array* block_displs_;
  array::Array* block_cols_;

  mutable array::ArrayView<size_t,1> block_displs_view_;
  mutable array::ArrayView<size_t,1> block_cols_view_;
  mutable array::Vector<BlockConnectivity*> block_;
  size_t blocks_;

};

// -----------------------------------------------------------------------------------------------------

/// @brief Block Connectivity table
/// @author Willem Deconinck
/// Container for connectivity tables that are layed out in memory as a block. Every row has the same
/// number of columns.
///
/// There are 2 modes of construction:
///   - It wraps existing raw data
///   - It owns the connectivity data
///
/// In case ATLAS_HAVE_FORTRAN is defined (which is usually the case),
/// the raw data will be stored with base 1 for Fortran interoperability.
/// The operator(row,col) will then do the conversion to base 0.
///
/// In the first mode of construction, the connectivity table cannot be resized.
/// In the second mode of construction, resizing is possible
class BlockConnectivity : public eckit::Owned {

private:
  friend class MultiBlockConnectivity;
  BlockConnectivity( size_t rows, size_t cols, idx_t values[], bool dummy);

public:

//-- Constructors

  /// @brief Construct connectivity table that needs resizing a-posteriori
  /// Data is owned
  BlockConnectivity();
  BlockConnectivity( size_t rows, size_t cols, const std::initializer_list<idx_t>& );

  /// @brief Construct connectivity table wrapping existing raw data.
  /// No resizing can be performed as data is not owned.
  BlockConnectivity( size_t rows, size_t cols, idx_t values[]);

  /// @brief Copy ctr (only to be used when calling a cuda kernel)
  // This ctr has to be defined in the header, since __CUDACC__ will identify whether
  // it is compiled it for a GPU kernel
  GT_FUNCTION
  BlockConnectivity(const BlockConnectivity& other)
    : owns_(false),
      rows_(other.rows_),
      cols_(other.cols_),
      values_(0),
#ifdef __CUDACC__
      values_view_(array::make_device_view<idx_t, 2>(*(other.values_))),
#else
      values_view_(array::make_device_view<idx_t, 2>(*(other.values_))),
#endif
      missing_value_( other.missing_value_)
  {}


  /// @brief Destructor
  ~BlockConnectivity();

  void rebuild( size_t rows, size_t cols, idx_t values[] );

//-- Accessors

  /// @brief Access to connectivity table elements for given row and column
  /// The returned index has base 0 regardless if ATLAS_HAVE_FORTRAN is defined.
  GT_FUNCTION
  idx_t operator()( size_t row_idx, size_t col_idx ) const;

  /// @brief Number of rows
  GT_FUNCTION
  size_t rows() const { return rows_; }

  /// @brief Number of columns
  GT_FUNCTION
  size_t cols() const { return cols_; }

  /// @brief Access to raw data.
  /// Note that the connectivity base is 1 in case ATLAS_HAVE_FORTRAN is defined.
  GT_FUNCTION
  const idx_t* data() const { return values_view_.data(); }
  GT_FUNCTION
        idx_t* data()       { return values_view_.data(); }

  GT_FUNCTION
  idx_t missing_value() const { return missing_value_; }

  size_t footprint() const;

//-- Modifiers

  /// @brief Modify row with given values. Values must be given with base 0
  GT_FUNCTION
  void set( size_t row_idx, const idx_t column_values[] );

  /// @brief Modify (row,col) with given value. Value must be given with base 0
  GT_FUNCTION
  void set( size_t row_idx, size_t col_idx, const idx_t value );

  /// @brief Resize connectivity, and add given rows
  /// @note Can only be used when data is owned.
  void add( size_t rows, size_t cols, const idx_t values[], bool fortran_array=false );

  void cloneToDevice() const;
  void cloneFromDevice() const;
  void syncHostDevice() const;
  bool valid() const;
  bool isOnHost() const;
  bool isOnDevice() const;

  bool owns() const { return owns_; }

private:
  bool owns_;
  array::Array* values_;
  mutable array::ArrayView<idx_t, 2> values_view_;

  size_t rows_;
  size_t cols_;
  idx_t missing_value_;

};

// -----------------------------------------------------------------------------------------------------

inline idx_t IrregularConnectivity::operator()( size_t row_idx, size_t col_idx ) const
{
  assert(counts_view_(row_idx) >( col_idx));
  return values_view_(displs_view_(row_idx) + col_idx) FROM_FORTRAN;
}

inline void IrregularConnectivity::set( size_t row_idx, const idx_t column_values[] ) {
  const size_t N = counts_view_(row_idx);
  for( size_t n=0; n<N; ++n ) {
    values_view_(displs_view_(row_idx) + n) = column_values[n] TO_FORTRAN;
  }
}

inline void IrregularConnectivity::set( size_t row_idx, size_t col_idx, const idx_t value ) {
    assert(col_idx < counts_view_(row_idx));
  values_view_(displs_view_(row_idx) + col_idx) = value TO_FORTRAN;
}

inline IrregularConnectivity::Row IrregularConnectivity::row( size_t row_idx ) const
{
  return IrregularConnectivity::Row(const_cast<idx_t*>(values_view_.data() ) +displs_view_(row_idx) , counts_view_(row_idx) );
}

// -----------------------------------------------------------------------------------------------------

inline idx_t MultiBlockConnectivity::operator()( size_t row_idx, size_t col_idx ) const
{
  return IrregularConnectivity::operator()(row_idx,col_idx);
}


inline idx_t MultiBlockConnectivity::operator()( size_t block_idx, size_t block_row_idx, size_t block_col_idx ) const
{
  return block(block_idx)(block_row_idx,block_col_idx);
}


// -----------------------------------------------------------------------------------------------------

inline idx_t BlockConnectivity::operator()( size_t row_idx, size_t col_idx ) const {
  return values_view_(row_idx, col_idx) FROM_FORTRAN;
}

inline void BlockConnectivity::set( size_t row_idx, const idx_t column_values[] ) {
  for( size_t n=0; n<cols_; ++n ) {
    values_view_(row_idx,n) = column_values[n] TO_FORTRAN;
  }
}

inline void BlockConnectivity::set( size_t row_idx, size_t col_idx, const idx_t value ) {
  values_view_(row_idx, col_idx) = value TO_FORTRAN;
}

// ------------------------------------------------------------------------------------------------------

extern "C"
{
Connectivity* atlas__Connectivity__create();
MultiBlockConnectivity* atlas__MultiBlockConnectivity__create();
const char* atlas__Connectivity__name (Connectivity* This);
void atlas__Connectivity__rename(Connectivity* This, const char* name);
void atlas__Connectivity__delete(Connectivity* This);
void atlas__Connectivity__displs(Connectivity* This, size_t* &displs, size_t &size);
void atlas__Connectivity__counts(Connectivity* This, size_t* &counts, size_t &size);
void atlas__Connectivity__values(Connectivity* This, int* &values, size_t &size);
size_t atlas__Connectivity__rows(const Connectivity* This);
void atlas__Connectivity__add_values(Connectivity* This, size_t rows, size_t cols, int values[]);
void atlas__Connectivity__add_missing(Connectivity* This, size_t rows, size_t cols);
int atlas__Connectivity__missing_value(const Connectivity* This);

size_t atlas__MultiBlockConnectivity__blocks(const MultiBlockConnectivity* This);
BlockConnectivity* atlas__MultiBlockConnectivity__block(MultiBlockConnectivity* This, size_t block_idx);

size_t atlas__BlockConnectivity__rows(const BlockConnectivity* This);
size_t atlas__BlockConnectivity__cols(const BlockConnectivity* This);
int atlas__BlockConnectivity__missing_value(const BlockConnectivity* This);
void atlas__BlockConnectivity__data(BlockConnectivity* This, int* &data, size_t &rows, size_t &cols);
void atlas__BlockConnectivity__delete(BlockConnectivity* This);
}

#undef FROM_FORTRAN
#undef TO_FORTRAN
#undef INDEX_REF

//------------------------------------------------------------------------------------------------------

} // namespace mesh

namespace array {

//TODO HACK
template<> inline DataType::kind_t DataType::kind<mesh::BlockConnectivity*>()   { return KIND_INT64;    }

}

} // namespace atlas

#endif
