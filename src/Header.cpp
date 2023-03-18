#include "Header.h"
#include "Sector.h"

std::string to_string(const DataRate& datarate)
{
    switch (datarate)
    {
    case DataRate::_250K:   return "250Kbps";       break;
    case DataRate::_300K:   return "300Kbps";       break;
    case DataRate::_500K:   return "500Kbps";       break;
    case DataRate::_1M:     return "1Mbps";         break;
    case DataRate::Unknown: break;
    }
    return "Unknown";
}

std::string to_string(const Encoding& encoding)
{
    switch (encoding)
    {
    case Encoding::MFM:     return "MFM";           break;
    case Encoding::FM:      return "FM";            break;
    case Encoding::RX02:    return "RX02";          break;
    case Encoding::Amiga:   return "Amiga";         break;
    case Encoding::GCR:     return "GCR";           break;
    case Encoding::Ace:     return "Ace";           break;
    case Encoding::MX:      return "MX";            break;
    case Encoding::Agat:    return "Agat";          break;
    case Encoding::Apple:   return "Apple";         break;
    case Encoding::Victor:  return "Victor";        break;
    case Encoding::Vista:   return "Vista";         break;
    case Encoding::Unknown: break;
    }
    return "Unknown";
}

std::string short_name(const Encoding& encoding)
{
    switch (encoding)
    {
    case Encoding::MFM:     return "mfm";           break;
    case Encoding::FM:      return "fm";            break;
    case Encoding::RX02:    return "rx";            break;
    case Encoding::Amiga:   return "ami";           break;
    case Encoding::GCR:     return "gcr";           break;
    case Encoding::Ace:     return "ace";           break;
    case Encoding::MX:      return "mx";            break;
    case Encoding::Agat:    return "agat";          break;
    case Encoding::Apple:   return "a2";            break;
    case Encoding::Victor:  return "vic";           break;
    case Encoding::Vista:   return "vis";           break;
    case Encoding::Unknown: break;
    }
    return "unk";
}


DataRate datarate_from_string(std::string str)
{
    str = util::lowercase(str);
    auto len = str.size();

    if (str == std::string("250kbps").substr(0, len)) return DataRate::_250K;
    if (str == std::string("300kbps").substr(0, len)) return DataRate::_300K;
    if (str == std::string("500kbps").substr(0, len)) return DataRate::_500K;
    if (str == std::string("1mbps").substr(0, len)) return DataRate::_1M;
    return DataRate::Unknown;
}

Encoding encoding_from_string(std::string str)
{
    str = util::lowercase(str);

    if (str == "mfm") return Encoding::MFM;
    if (str == "fm") return Encoding::FM;
    if (str == "gcr") return Encoding::GCR;
    if (str == "amiga") return Encoding::Amiga;
    if (str == "ace") return Encoding::Ace;
    if (str == "mx") return Encoding::MX;
    if (str == "agat") return Encoding::Agat;
    if (str == "apple") return Encoding::Apple;
    if (str == "victor") return Encoding::Victor;
    if (str == "vista") return Encoding::Vista;
    return Encoding::Unknown;
}

//////////////////////////////////////////////////////////////////////////////

CylHead::operator int() const
{
    return (cyl * MAX_DISK_HEADS) + head;
}

CylHead operator * (const CylHead& cylhead, int cyl_step)
{
    return CylHead(cylhead.cyl * cyl_step, cylhead.head);
}

//////////////////////////////////////////////////////////////////////////////

Header::Header(int cyl_, int head_, int sector_, int size_)
    : cyl(cyl_), head(head_), sector(sector_), size(size_)
{
}

Header::Header(const CylHead& cylhead, int sector_, int size_)
    : cyl(cylhead.cyl), head(cylhead.head), sector(sector_), size(size_)
{
}

bool Header::operator== (const Header& rhs) const
{
    return compare_crn(rhs);    // ToDo: use compare_chrn?
}

bool Header::operator!= (const Header& rhs) const
{
    return !compare_crn(rhs);
}

Header::operator CylHead() const
{
    return CylHead(cyl, head);
}

bool Header::compare_chrn(const Header& rhs) const
{
    return cyl == rhs.cyl &&
        head == rhs.head &&
        sector == rhs.sector &&
        size == rhs.size;
}

bool Header::compare_crn(const Header& rhs) const
{
    // Compare without head match, like WD17xx
    return cyl == rhs.cyl &&
        sector == rhs.sector &&
        size == rhs.size;
}

int Header::sector_size() const
{
    return Sector::SizeCodeToLength(size);
}

//////////////////////////////////////////////////////////////////////////////

bool Headers::contains(const Header& header) const
{
    return std::find(begin(), end(), header) != end();
}

std::string Headers::to_string() const
{
    std::stringstream s;
    std::copy(begin(), end(), std::ostream_iterator<Header>(s, " "));
    return s.str();
}

std::string Headers::sector_ids_to_string() const
{
    std::ostringstream ss;
    std::for_each(begin(), end(), [&](const Header& header) {
        if (&header != &*begin())
            ss << " ";
        ss << header.sector;
    });
    return ss.str();
}

bool Headers::has_id_sequence(const int first_id, const int length) const
{
    if (static_cast<int>(size()) < length) // No chance for long enough sequence.
        return false;
    std::vector<bool> sequence(first_id + length);
    // Check the sequence of first_id, first_id+1, ..., up_to_id
    for_each(begin(), end(), [&](const Header& header) {
        if (header.sector >= first_id + length || header.sector < first_id)
            return;
        sequence[header.sector] = true; // Multiple ids also mean id is found.
    });
    return std::all_of(sequence.begin() + first_id, sequence.end(), [](bool marked) {return marked; });
}

Headers Headers::map(const std::map<int, int>& sector_id_map) const