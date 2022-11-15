// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE MappedImageTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>
// boost unit_test needs to be included before serverBasedTestTools.h
#include "MappedImage.h"

#include <boost/test/unit_test.hpp>

using namespace boost::unit_test_framework;
using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(MappedImageTestSuite)

BOOST_AUTO_TEST_CASE(testStructMapping) {
  // this test shows an example how to map user-defined opaque structs onto a byte array
  struct AStruct : public OpaqueStructHeader {
    int a = 0;
    float x = 0, y = 1;
    AStruct() : OpaqueStructHeader(typeid(AStruct)) {}
  };
  unsigned len = 100;
  std::vector<unsigned char> buf(len);
  MappedStruct<AStruct> ms(buf);
  auto* h = ms.header();
  h->x = 4.;
  BOOST_CHECK(h->totalLength == sizeof(AStruct));

  MappedStruct<AStruct> ms1(buf, MappedStruct<AStruct>::InitData::No);
  BOOST_CHECK(ms1.header()->x == 4.);
  BOOST_CHECK(ms1.header()->y == 1.); // a value set in constructor of AStruct
}

BOOST_AUTO_TEST_CASE(testMappedImage) {
  // this test shows MappedImage usage
  std::vector<uint8_t> buffer(100); // includes header: 64 bytes
  MappedImage A0(buffer);
  unsigned w = 4, h = 2;
  A0.setShape(w, h, ImgFormat::Gray16);
  auto Av = A0.interpretedView<uint16_t>();
  Av(0, 0) = 8;
  Av(1, 0) = 7;
  Av(2, 0) = 6;
  Av(3, 0) = 5;
  Av(0, 1) = 4;
  Av(1, 1) = 3;
  Av(2, 1) = 2;
  Av(3, 1) = 1;
  float val = Av(2, 0);
  BOOST_CHECK(val == 6);

  // also test iterator-style access
  for(unsigned y = 0; y < h; y++) {
    unsigned x = 0;
    for(auto* it = Av.beginRow(y); it != Av.endRow(y); ++it) {
      BOOST_CHECK(*it == Av(x, y));
      x++;
    }
  }
  // iterate over whole image
  unsigned counter = 0;
  for(auto& pixVal : Av) {
    pixVal = ++counter;
  }
  counter = 0;
  for(auto& pixVal : Av) {
    counter++;
    BOOST_CHECK(pixVal == counter);
  }

  // test actual header contents of our buffer
  auto* head = reinterpret_cast<ImgHeader*>(buffer.data());
  BOOST_CHECK(head->width == w);
  BOOST_CHECK(head->height == h);
  BOOST_CHECK(head->image_format == ImgFormat::Gray16);
  BOOST_CHECK(head->channels == 1);
  BOOST_CHECK(head->bytesPerPixel == 2);

  // test actual image body contents of buffer
  auto* imgBody = reinterpret_cast<uint16_t*>(buffer.data() + sizeof(ImgHeader));
  unsigned i = 0;
  for(auto& pixVal : Av) {
    BOOST_CHECK(imgBody[i] == pixVal);
    i++;
  }
}

BOOST_AUTO_TEST_SUITE_END()
