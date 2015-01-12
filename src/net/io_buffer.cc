#include "net/io_buffer.h"

#include "base/logging.h"

namespace net {

IOBuffer::IOBuffer() : data_(NULL) { }

IOBuffer::IOBuffer(int buffer_size) {
  CHECK_GT(buffer_size, 0);
  data_ = new char[buffer_size];
}

char* IOBuffer::data() const {
  return data_;
}

IOBuffer::~IOBuffer() {
  if (data_)
    delete []data_;
}

GrowableIOBuffer::GrowableIOBuffer()
    : IOBuffer(),
      capacity_(0),
      offset_(0) {
}

void GrowableIOBuffer::SetCapacity(int capacity) {
  DCHECK_GE(capacity, 0);
  // realloc will crash if it fails.
  real_data_.reset(static_cast<char*>(realloc(real_data_.release(), capacity)));
  capacity_ = capacity;
  if (offset_ > capacity)
    set_offset(capacity);
  else
    set_offset(offset_);  // The pointer may have changed.
}

void GrowableIOBuffer::set_offset(int offset) {
  DCHECK_GE(offset, 0);
  DCHECK_LE(offset, capacity_);
  offset_ = offset;
  data_ = real_data_.get() + offset;
}

int GrowableIOBuffer::RemainingCapacity() {
  return capacity_ - offset_;
}

char* GrowableIOBuffer::StartOfBuffer() {
  return real_data_.get();
}

GrowableIOBuffer::~GrowableIOBuffer() {
  data_ = NULL;
}

}  // namespace net
