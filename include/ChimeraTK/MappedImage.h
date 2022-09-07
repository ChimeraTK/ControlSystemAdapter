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
    /// call with initData=false if data already contains valid struct data.
    explicit MappedStruct(unsigned char* data, size_t dataLen, InitData doInitData = InitData::Yes) {
      static_assert(std::is_base_of<OpaqueStructHeader, StructHeader>::value,
          "MappedStruct expects StructHeader to implement OpaqueStructHeader");
      _data = data;
      _dataMaxLen = dataLen;
      if(doInitData == InitData::Yes) {
        memset(_data, 0, _dataMaxLen);
        _header = new(_data) StructHeader;
        _header->totalLength = sizeof(StructHeader); // minimal length, could be larger
      }
      else {
        _header = reinterpret_cast<StructHeader*>(_data);
      }
      assert(_header->totalLength <= _dataMaxLen);
    }
    /// convenience: same as above but for OneDRegisterAccessor
    explicit MappedStruct(
        ChimeraTK::OneDRegisterAccessor<unsigned char>& accToData, InitData doInitData = InitData::Yes)
    : MappedStruct(accToData.data(), accToData.getNElements(), doInitData) {}
    /// provided for convenience, if MappedStruct should allocate the data vector
    MappedStruct() {
      _allocate = true;
      realloc(sizeof(StructHeader));
      OpaqueStructHeader* h = _header;
      h->totalLength = sizeof(StructHeader);
    }
    unsigned char* data() { return _data; }
    size_t dataLen() const { return _dataMaxLen; }
    /// e.g. for setting meta data
    StructHeader& header() { return *_header; }

   protected:
    void realloc(size_t newLen) {
        _dataMaxLen = newLen;
        _allocatedBuf.resize(_dataMaxLen);
        _data = _allocatedBuf.data();
        _header = new (_data) StructHeader;
    }

    bool _allocate = false;
    std::vector<unsigned char> _allocatedBuf; // used only if _allocate=true
    StructHeader* _header;
    unsigned char* _data = nullptr; // pointer to data for header and struct content
    size_t _dataMaxLen = 0;         // this is the capacity of the container, actual datalen is less
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
      // TODO check how image gets distorted if wrong
      if constexpr((unsigned)OPTIONS & (unsigned)ImgOptions::RowMajor) {
        return _vec[(dy * _h->width + dx) * _h->channels + c];
      }
      else {
        return _vec[(dy + dx * _h->height) * _h->channels + c];
      }
    }

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

    /// needs to be called after construction. corrupts all data.
    void setShape(unsigned width, unsigned height, ImgFormat fmt) {
      unsigned channels;
      unsigned bpp;
      switch(fmt) {
        case ImgFormat::Unset:
          assert(false && "ImgFormat::Unset not allowed in setShape");
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

      size_t totalLen = sizeof(ImgHeader) + (size_t)width * height * bpp;
      if(!_allocate) {
          assert(totalLen <= _dataMaxLen);
      }
      else {
        realloc(totalLen);
      }
      _header->image_format = fmt;
      _header->totalLength = totalLen;
      _header->width = width;
      _header->height = height;
      _header->channels = channels;
      _header->bpp = bpp;
    }

    /// returns an ImgView object which can be used like a matrix. The ImgView becomes invalid at next setShape call.
    template<typename UserType, ImgOptions OPTIONS = ImgOptions::RowMajor>
    ImgView<UserType, OPTIONS> interpretedView() {
      assert(_header->channels > 0 && "call setShape() before interpretedView()!");
      assert(_header->bpp == _header->channels * sizeof(UserType) &&
          "choose correct bpp and channels value before conversion!");
      assert(((unsigned)_header->options & (unsigned)ImgOptions::RowMajor) ==
              ((unsigned)OPTIONS & (unsigned)ImgOptions::RowMajor) &&
          "inconsistent data ordering col/row major");
      ImgView<UserType, OPTIONS> ret;
      ret._h = _header;
      ret._vec = reinterpret_cast<UserType*>(_data + sizeof(ImgHeader));
      return ret;
    }
  };

} // namespace ChimeraTK
