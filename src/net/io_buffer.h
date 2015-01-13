// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Copyright (c) 2015 Chaobin Zhang. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file and LICENSE file.

#ifndef  IO_BUFFER_H_
#define  IO_BUFFER_H_

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"

namespace net {

class IOBuffer : public base::RefCountedThreadSafe<IOBuffer> {
 public:
  IOBuffer();
  explicit IOBuffer(int buffer_size);
  char* data() const;

 protected:
  friend class base::RefCountedThreadSafe<IOBuffer>;

  virtual ~IOBuffer();

  char* data_;
};

// This version provides a resizable buffer and a changeable offset.
//
// GrowableIOBuffer is useful when you read data progressively without
// knowing the total size in advance. GrowableIOBuffer can be used as
// follows:
//
// buf = new GrowableIOBuffer;
// buf->SetCapacity(1024);  // Initial capacity.
//
// while (!some_stream->IsEOF()) {
//   // Double the capacity if the remaining capacity is empty.
//   if (buf->RemainingCapacity() == 0)
//     buf->SetCapacity(buf->capacity() * 2);
//   int bytes_read = some_stream->Read(buf, buf->RemainingCapacity());
//   buf->set_offset(buf->offset() + bytes_read);
// }
//
class GrowableIOBuffer : public IOBuffer {
 public:
  GrowableIOBuffer();

  // realloc memory to the specified capacity.
  void SetCapacity(int capacity);
  int capacity() { return capacity_; }

  // |offset| moves the |data_| pointer, allowing "seeking" in the data.
  void set_offset(int offset);
  int offset() { return offset_; }

  int RemainingCapacity();
  char* StartOfBuffer();

 private:
  ~GrowableIOBuffer() override;

  scoped_ptr<char, base::FreeDeleter> real_data_;
  int capacity_;
  int offset_;
};

}  // namespace net

#endif  // IO_BUFFER_H_
