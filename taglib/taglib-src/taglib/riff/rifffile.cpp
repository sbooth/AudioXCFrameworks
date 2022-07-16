/***************************************************************************
    copyright            : (C) 2002 - 2008 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License version   *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA         *
 *   02110-1301  USA                                                       *
 *                                                                         *
 *   Alternatively, this file is available under the Mozilla Public        *
 *   License Version 1.1.  You may obtain a copy of the License at         *
 *   http://www.mozilla.org/MPL/                                           *
 ***************************************************************************/

#include <algorithm>
#include <vector>

#include <tbytevector.h>
#include "tdebug.h"
#include <tstring.h>

#include "rifffile.h"
#include "riffutils.h"

using namespace TagLib;

namespace
{
  struct Chunk
  {
    ByteVector   name;
    long long    offset;
    unsigned int size;
    unsigned int padding;
  };

  unsigned int toUInt32(const ByteVector &v, size_t offset, ByteOrder endian)
  {
    if(endian == BigEndian)
      return v.toUInt32BE(offset);
    else
      return v.toUInt32LE(offset);
  }

  ByteVector fromUInt32(size_t value, ByteOrder endian)
  {
    if(endian == BigEndian)
      return ByteVector::fromUInt32BE(value);
    else
      return ByteVector::fromUInt32LE(value);
  }
}

class RIFF::File::FilePrivate
{
public:
  explicit FilePrivate(ByteOrder endianness) :
    endianness(endianness),
    size(0),
    sizeOffset(0) {}

  const ByteOrder endianness;

  unsigned int size;
  long long sizeOffset;

  std::vector<Chunk> chunks;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

RIFF::File::~File()
{
  delete d;
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

RIFF::File::File(FileName file, ByteOrder endianness) :
  TagLib::File(file),
  d(new FilePrivate(endianness))
{
  if(isOpen())
    read();
}

RIFF::File::File(IOStream *stream, ByteOrder endianness) :
  TagLib::File(stream),
  d(new FilePrivate(endianness))
{
  if(isOpen())
    read();
}

unsigned int RIFF::File::riffSize() const
{
  return d->size;
}

size_t RIFF::File::chunkCount() const
{
  return d->chunks.size();
}

unsigned int RIFF::File::chunkDataSize(unsigned int i) const
{
  if(i >= d->chunks.size()) {
    debug("RIFF::File::chunkDataSize() - Index out of range. Returning 0.");
    return 0;
  }

  return d->chunks[i].size;
}

long long RIFF::File::chunkOffset(unsigned int i) const
{
  if(i >= d->chunks.size()) {
    debug("RIFF::File::chunkOffset() - Index out of range. Returning 0.");
    return 0;
  }

  return d->chunks[i].offset;
}

unsigned int RIFF::File::chunkPadding(unsigned int i) const
{
  if(i >= d->chunks.size()) {
    debug("RIFF::File::chunkPadding() - Index out of range. Returning 0.");
    return 0;
  }

  return d->chunks[i].padding;
}

ByteVector RIFF::File::chunkName(unsigned int i) const
{
  if(i >= d->chunks.size()) {
    debug("RIFF::File::chunkName() - Index out of range. Returning an empty vector.");
    return ByteVector();
  }

  return d->chunks[i].name;
}

ByteVector RIFF::File::chunkData(unsigned int i)
{
  if(i >= d->chunks.size()) {
    debug("RIFF::File::chunkData() - Index out of range. Returning an empty vector.");
    return ByteVector();
  }

  seek(d->chunks[i].offset);
  return readBlock(d->chunks[i].size);
}

void RIFF::File::setChunkData(unsigned int i, const ByteVector &data)
{
  if(i >= d->chunks.size()) {
    debug("RIFF::File::setChunkData() - Index out of range.");
    return;
  }

  // Now update the specific chunk

  std::vector<Chunk>::iterator it = d->chunks.begin();
  std::advance(it, i);

  const long long originalSize = static_cast<long long>(it->size) + it->padding;

  writeChunk(it->name, data, it->offset - 8, it->size + it->padding + 8);

  it->size    = static_cast<unsigned int>(data.size());
  it->padding = data.size() % 2;

  const long long diff = static_cast<long long>(it->size) + it->padding - originalSize;

  // Now update the internal offsets

  for(++it; it != d->chunks.end(); ++it)
    it->offset += diff;

  // Update the global size.

  updateGlobalSize();
}

void RIFF::File::setChunkData(const ByteVector &name, const ByteVector &data)
{
  setChunkData(name, data, false);
}

void RIFF::File::setChunkData(const ByteVector &name, const ByteVector &data, bool alwaysCreate)
{
  if(d->chunks.empty()) {
    debug("RIFF::File::setChunkData - No valid chunks found.");
    return;
  }

  if(alwaysCreate && name != "LIST") {
    debug("RIFF::File::setChunkData - alwaysCreate should be used for only \"LIST\" chunks.");
    return;
  }

  if(!alwaysCreate) {
    for(unsigned int i = 0; i < d->chunks.size(); i++) {
      if(d->chunks[i].name == name) {
        setChunkData(i, data);
        return;
      }
    }
  }

  // Couldn't find an existing chunk, so let's create a new one.

  // Adjust the padding of the last chunk to place the new chunk at even position.

  Chunk &last = d->chunks.back();

  long long offset = last.offset + last.size + last.padding;
  if(offset & 1) {
    if(last.padding == 1) {
      last.padding = 0; // This should not happen unless the file is corrupted.
      offset--;
      removeBlock(offset, 1);
    }
    else {
      insert(ByteVector("\0", 1), offset, 0);
      last.padding = 1;
      offset++;
    }
  }

  // Now add the chunk to the file.

  writeChunk(name, data, offset, 0);

  // And update our internal structure

  Chunk chunk;
  chunk.name    = name;
  chunk.size    = static_cast<unsigned int>(data.size());
  chunk.offset  = offset + 8;
  chunk.padding = data.size() % 2;

  d->chunks.push_back(chunk);

  // Update the global size.

  updateGlobalSize();
}

void RIFF::File::removeChunk(unsigned int i)
{
  if(i >= d->chunks.size()) {
    debug("RIFF::File::removeChunk() - Index out of range.");
    return;
  }

  std::vector<Chunk>::iterator it = d->chunks.begin();
  std::advance(it, i);

  const size_t removeSize = it->size + it->padding + 8;
  removeBlock(it->offset - 8, removeSize);
  it = d->chunks.erase(it);

  for(; it != d->chunks.end(); ++it)
    it->offset -= removeSize;

  // Update the global size.

  updateGlobalSize();
}

void RIFF::File::removeChunk(const ByteVector &name)
{
  for(int i = static_cast<int>(d->chunks.size()) - 1; i >= 0; --i) {
    if(d->chunks[i].name == name)
      removeChunk(i);
  }
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void RIFF::File::read()
{
  long long offset = tell();

  offset += 4;
  d->sizeOffset = offset;

  seek(offset);
  d->size = toUInt32(readBlock(4), 0, d->endianness);

  offset += 8;

  // + 8: chunk header at least, fix for additional junk bytes
  while(offset + 8 <= length()) {

    seek(offset);
    const ByteVector   chunkName = readBlock(4);
    const unsigned int chunkSize = toUInt32(readBlock(4), 0, d->endianness);

    if(!isValidChunkName(chunkName)) {
      debug("RIFF::File::read() -- Chunk '" + chunkName + "' has invalid ID");
      setValid(false);
      break;
    }

    if(offset + 8 + chunkSize > length()) {
      debug("RIFF::File::read() -- Chunk '" + chunkName + "' has invalid size (larger than the file size)");
      setValid(false);
      break;
    }

    Chunk chunk;
    chunk.name    = chunkName;
    chunk.size    = chunkSize;
    chunk.offset  = offset + 8;
    chunk.padding = 0;

    offset = chunk.offset + chunk.size;

    // Check padding

    if(offset & 1) {
      seek(offset);
      const ByteVector iByte = readBlock(1);
      if(iByte.size() == 1 && iByte[0] == '\0') {
        chunk.padding = 1;
        offset++;
      }
    }

    d->chunks.push_back(chunk);
  }
}

void RIFF::File::writeChunk(const ByteVector &name, const ByteVector &data,
                            long long offset, size_t replace)
{
  ByteVector combined;

  combined.append(name);
  combined.append(fromUInt32(data.size(), d->endianness));
  combined.append(data);

  if(data.size() & 1)
    combined.resize(combined.size() + 1, '\0');

  insert(combined, offset, replace);
}

void RIFF::File::updateGlobalSize()
{
  const Chunk first = d->chunks.front();
  const Chunk last  = d->chunks.back();
  d->size = static_cast<unsigned int>(last.offset + last.size + last.padding - first.offset + 12);

  const ByteVector data = fromUInt32(d->size, d->endianness);
  insert(data, d->sizeOffset, 4);
}
