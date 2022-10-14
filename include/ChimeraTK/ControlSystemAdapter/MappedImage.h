// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <type_traits>

#include <ChimeraTK/OneDRegisterAccessor.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <typeindex>
#include <typeinfo>
#include <vector>

namespace ChimeraTK {

  /**
   *  generic header for opaque struct handling
   *  It has fields needed for communication in the same process, e.g. between different ApplicationCore modules.
   *  Fields needed for communication over the net belong not here but into the relevant ControlsystemAdapter, e.g. the
   *  OPCUA adapter could implement OPC UA binary mapping and use type ids from a dictionary, in the future.
   */
  struct OpaqueStructHeader {
    std::type_index dataTypeId;
    uint32_t totalLength = 0; // 0: unknown/not set. length includes header.
    explicit OpaqueStructHeader(std::type_index dataTypeId_) : dataTypeId(dataTypeId_) {}
  };

  /**
   * Provides interface to a struct that is mapped onto a 1D array of ValType
   * The struct must be derived from OpaqueStructHeader.
   * Variable-length structs are supported, as long as they do not grow beyond the size of the given 1D array.
   */
  template<class StructHeader>
  class MappedStruct {
   public:
    enum class InitData { Yes, No };
    /// keeps a reference to given vector.
    /// call with InitData::Yes if data already contains valid struct data.
    explicit MappedStruct(std::vector<unsigned char>& buffer, InitData doInitData = InitData::Yes) {
      _containerImpl = ContainerImpl::Vector;
      _vectorToData = &buffer;
      helpInit(doInitData == InitData::Yes);
    }
    /// like above, but keeps a pointer for the data
    explicit MappedStruct(unsigned char* buffer, size_t bufferLen, InitData doInitData = InitData::Yes)
    : _containerImpl(ContainerImpl::CArray), _cArrToData(buffer), _cArrLenth(bufferLen) {
      helpInit(doInitData == InitData::Yes);
    }

    /// this stores a OneDRegisterAccessor instead of a vector. If the underlying vector is swapped out,
    /// the MappedStruct stays valid if the swapped-in vector was also setup as MappedStruct
    explicit MappedStruct(
        ChimeraTK::OneDRegisterAccessor<unsigned char>& accToData, InitData doInitData = InitData::Yes)
    : _containerImpl(ContainerImpl::Accessor), _accToData(accToData) {
      helpInit(doInitData == InitData::Yes);
    }

    /// returns pointer to data for header and struct content
    unsigned char* data() {
      switch(_containerImpl) {
        case ContainerImpl::Accessor:
          return _accToData.data();
        case ContainerImpl::Vector:
          return _vectorToData->data();
        case ContainerImpl::CArray:
          return _cArrToData;
      }
    }
    /// capacity of used container
    size_t capacity() const {
      switch(_containerImpl) {
        case ContainerImpl::Accessor:
          // reason for cast: getNElements not declared const
          return const_cast<MappedStruct*>(this)->_accToData.getNElements();
        case ContainerImpl::Vector:
          return _vectorToData->capacity();
        case ContainerImpl::CArray:
          return _cArrLenth;
      }
    }
    /// currently used size
    size_t size() const { return static_cast<OpaqueStructHeader*>(header())->totalLength; }
    /// returns header, e.g. for setting meta data
    StructHeader* header() { return reinterpret_cast<StructHeader*>(data()); }

   protected:
    void helpInit(bool doInitData) {
      static_assert(std::is_base_of<OpaqueStructHeader, StructHeader>::value,
          "MappedStruct expects StructHeader to implement OpaqueStructHeader");
      if(doInitData) {
        if(capacity() < sizeof(StructHeader)) {
          throw logic_error("buffer provided to MappedStruct is too small for correct initialization");
        }

        auto* p = data();
        memset(p, 0, capacity());
        new(p) StructHeader;
        header()->totalLength = sizeof(StructHeader); // minimal length, could be larger
      }
      else {
        if(header()->totalLength > capacity()) {
          throw logic_error("buffer provided to MappedStruct is too small for assumed content");
        }
      }
    }

    // implementation choice for referred data container
    enum class ContainerImpl { Accessor, Vector, CArray };
    ContainerImpl _containerImpl;
    // We keep the accessor instead of the naked pointer to
    // simplify usage, like this the object can exist even after memory used by accessor was swapped.
    ChimeraTK::OneDRegisterAccessor<unsigned char> _accToData;
    std::vector<unsigned char>* _vectorToData;
    unsigned char* _cArrToData;
    size_t _cArrLenth;
  };

  /*******************************  application to Image encoding *********************************/

  enum class ImgFormat { Unset = 0, Gray8, Gray16, RGB24, RGBA32 };
  // values are defined for a bit field
  enum class ImgOptions { RowMajor = 1, ColMajor = 0 };

  /// Image header
  struct ImgHeader : public OpaqueStructHeader {
    ImgHeader() : OpaqueStructHeader(typeid(ImgHeader)) {}

    uint32_t width = 0;
    uint32_t height = 0;
    // start coordinates, in output
    int32_t x_start = 0;
    int32_t y_start = 0;
    // can be used in output to provide scaled coordinates
    float scale_x = 1;
    float scale_y = 1;
    // gray=1, rgb=3, rgba=4
    uint32_t channels = 0;
    // bytes per pixel
    uint32_t bpp = 0;
    // effective bits per pixel
    uint32_t ebitpp = 0;
    ImgFormat image_format{ImgFormat::Unset};
    ImgOptions options{ImgOptions::RowMajor};
    // frame number/counter
    uint32_t frame = 0;
  };

  /**
   * provides convenient matrix-like access for MappedImage
   */
  template<typename ValType, ImgOptions OPTIONS>
  class ImgView {
    friend class MappedImage;

   public:
    /// dx, dy are relative to x_start, y_start, i.e. x = x_start+dx  on output side
    /// this method is for random access. for sequential access, iterators provide better performance
    ValType& operator()(unsigned dx, unsigned dy, unsigned c = 0) {
      assert(dy < _h->height);
      assert(dx < _h->width);
      assert(c < _h->channels);
      // this is the only place where row-major / column-major storage is decided
      // note, definition of row major/column major is confusing for images.
      // - for a matrix M(i,j) we say it is stored row-major if rows are stored without interleaving: M11, M12,...
      // - for the same Matrix, if we write M(x,y) for pixel value at coordite (x,y) of an image, this means
      //   that pixel _columns_ are stored without interleaving
      // So definition used here is opposite to matrix definition.
      if constexpr((unsigned)OPTIONS & (unsigned)ImgOptions::RowMajor) {
        return _vec[(dy * _h->width + dx) * _h->channels + c];
      }
      else {
        return _vec[(dy + dx * _h->height) * _h->channels + c];
      }
    }

    // simply define iterator access via pointers
    using iterator = ValType*;
    using value_type = ValType;
    // for iteration over whole image
    ValType* begin() { return beginRow(0); }
    ValType* end() { return beginRow(_h->height); }
    // these assume ROW-MAJOR ordering
    ValType* beginRow(unsigned row) { return _vec + row * _h->width * _h->channels; }
    ValType* endRow(unsigned row) { beginRow(row + 1); }

   protected:
    ImgHeader* _h;
    ValType* _vec;
  };

  /**
   * interface to an image that is mapped onto a 1D array of ValType
   */
  class MappedImage : public MappedStruct<ImgHeader> {
   public:
    using MappedStruct<ImgHeader>::MappedStruct;

    size_t formatsDefinition(ImgFormat fmt, unsigned width, unsigned height, unsigned& channels, unsigned& bpp) {
      switch(fmt) {
        case ImgFormat::Unset:
          assert(false && "ImgFormat::Unset not allowed");
          break;
        case ImgFormat::Gray8:
          channels = 1;
          bpp = 1;
          break;
        case ImgFormat::Gray16:
          channels = 1;
          bpp = 2;
          break;
        case ImgFormat::RGB24:
          channels = 3;
          bpp = 3;
          break;
        case ImgFormat::RGBA32:
          channels = 4;
          bpp = 4;
          break;
      }
      return sizeof(ImgHeader) + (size_t)width * height * bpp;
    }

    size_t lengthForShape(unsigned width, unsigned height, ImgFormat fmt) {
      unsigned channels;
      unsigned bpp;
      return formatsDefinition(fmt, width, height, channels, bpp);
    }

    /// needs to be called after construction. corrupts all data.
    /// this throws logic_error if our buffer size is too small. Try lenghtForShape() to check that in advance
    void setShape(unsigned width, unsigned height, ImgFormat fmt) {
      unsigned channels;
      unsigned bpp;
      size_t totalLen = formatsDefinition(fmt, width, height, channels, bpp);
      if(totalLen > capacity()) {
        throw logic_error("MappedImage: provided buffer to small for requested image shape");
      }
      auto* h = header();
      h->image_format = fmt;
      h->totalLength = totalLen;
      h->width = width;
      h->height = height;
      h->channels = channels;
      h->bpp = bpp;
    }
    /// returns pointer to image payload data
    unsigned char* imgBody() { return data() + sizeof(ImgHeader); }

    /// returns an ImgView object which can be used like a matrix. The ImgView becomes invalid at next setShape call.
    template<typename UserType, ImgOptions OPTIONS = ImgOptions::RowMajor>
    ImgView<UserType, OPTIONS> interpretedView() {
      auto* h = header();
      assert(h->channels > 0 && "call setShape() before interpretedView()!");
      assert(h->bpp == h->channels * sizeof(UserType) && "choose correct bpp and channels value before conversion!");
      assert(((unsigned)h->options & (unsigned)ImgOptions::RowMajor) ==
              ((unsigned)OPTIONS & (unsigned)ImgOptions::RowMajor) &&
          "inconsistent data ordering col/row major");
      ImgView<UserType, OPTIONS> ret;
      ret._h = h;
      ret._vec = reinterpret_cast<UserType*>(imgBody());
      return ret;
    }
  };

} // namespace ChimeraTK
