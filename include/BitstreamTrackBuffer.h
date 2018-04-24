#ifndef BITSTREAMTRACKBUFFER_H
#define BITSTREAMTRACKBUFFER_H

#include "TrackBuffer.h"

class BitstreamTrackBuffer final : public TrackBuffer
{
public:
	BitstreamTrackBuffer (DataRate datarate, Encoding encoding);

	int size () const;
	void setEncoding (Encoding encoding) override;
	void addRawBit (bool bit) override;
	void addCrc (int size);

	BitBuffer &buffer ();
	DataRate datarate () const;
	Encoding encoding () const;

private:
	BitBuffer m_buffer;
};

#endif // BITSTREAMTRACKBUFFER_H
