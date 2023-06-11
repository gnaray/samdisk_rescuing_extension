// fdrawcmd.sys device

#include "FdrawcmdSys.h"

#ifdef HAVE_FDRAWCMD_H

#include "utils.h"

#include <algorithm>

#ifdef _DEBUG
// Macro overloading reference:
// http://stackoverflow.com/questions/3046889/optional-parameters-with-c-macros/28074198#28074198
// or shorter: https://stackoverflow.com/a/28074198

// These are the macros which are called with proper amount of parameters.
#define IOCTL_3(RESULT,IO_PARAMS,DEBUG_MSG) (RESULT = Ioctl(IO_PARAMS), \
(util::cout << (DEBUG_MSG) << ", success=" << (RESULT) << ", returned=" << (IO_PARAMS).returned << '\n'), (RESULT))
#define IOCTL_2(RESULT,IO_PARAMS) RESULT = Ioctl(IO_PARAMS) // No debug text in third parameter, ignoring debug.
// We do not need IOCTL_1 because at least the result and the ioctl_params structure must be passed.
// We do not need IOCTL_0 because at least the result and the ioctl_params structure must be passed.

#define RETURN_IOCTL_2(IO_PARAMS,DEBUG_MSG) const auto RESULT = Ioctl(IO_PARAMS); \
util::cout << (DEBUG_MSG) << ", success=" << RESULT << ", returned=" << (IO_PARAMS).returned << '\n'; \
return RESULT
#define RETURN_IOCTL_1(IO_PARAMS) return Ioctl(IO_PARAMS) // No debug text in second parameter, ignoring debug.
// We do not need RETURN_IOCTL_0 because at least the ioctl_params structure must be passed.

// Macro magic to use desired macro with optional 0, 1, 2, 3 parameters.
#define IOCTL_FUNC_CHOOSER(_f1, _f2, _f3, _f4, ...) _f4
#define IOCTL_FUNC_RECOMPOSER(argsWithParentheses) IOCTL_FUNC_CHOOSER argsWithParentheses
#define IOCTL_CHOOSE_FROM_ARG_COUNT(...) IOCTL_FUNC_RECOMPOSER((__VA_ARGS__, IOCTL_3, IOCTL_2, IOCTL_1, ))
#define IOCTL_NO_ARG_EXPANDER() ,,IOCTL_0
#define IOCTL_MACRO_CHOOSER(...) IOCTL_CHOOSE_FROM_ARG_COUNT(IOCTL_NO_ARG_EXPANDER __VA_ARGS__ ())

// Macro magic to use desired macro with optional 0, 1, 2 parameters.
#define RETURN_IOCTL_FUNC_CHOOSER(_f1, _f2, _f3, ...) _f3
#define RETURN_IOCTL_FUNC_RECOMPOSER(argsWithParentheses) RETURN_IOCTL_FUNC_CHOOSER argsWithParentheses
#define RETURN_IOCTL_CHOOSE_FROM_ARG_COUNT(...) RETURN_IOCTL_FUNC_RECOMPOSER((__VA_ARGS__, RETURN_IOCTL_2, RETURN_IOCTL_1, ))
#define RETURN_IOCTL_NO_ARG_EXPANDER() ,,RETURN_IOCTL_0
#define RETURN_IOCTL_MACRO_CHOOSER(...) RETURN_IOCTL_CHOOSE_FROM_ARG_COUNT(RETURN_IOCTL_NO_ARG_EXPANDER __VA_ARGS__ ())

// These are the macros that the user can call.
// IOCTL parameters: {writable} bool result, {writable} IOCTL_PARAMS ioctl_params, {optional, util::cout acceptable} debug_text
#define IOCTL(...) IOCTL_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)
// RETURN_IOCTL parameters: {writable} IOCTL_PARAMS ioctl_params, {optional, util::cout acceptable} debug_text
#define RETURN_IOCTL(...) RETURN_IOCTL_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)
#else
#define IOCTL(RESULT, IO_PARAMS, ...) RESULT = Ioctl(IO_PARAMS) // Third parameter is debug text, ignoring it.
#define RETURN_IOCTL(IO_PARAMS, ...) return Ioctl(IO_PARAMS) // Second parameter is debug text, ignoring it.
#endif

/*static*/ std::unique_ptr<FdrawcmdSys> FdrawcmdSys::Open(int device_index)
{
    auto path = util::format(R"(\\.\fdraw)", device_index);

    Win32Handle hdev{ CreateFile(
        path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        nullptr) };

    if (hdev.get() != INVALID_HANDLE_VALUE)
        return std::make_unique<FdrawcmdSys>(hdev.release());

    return std::unique_ptr<FdrawcmdSys>();
}

FdrawcmdSys::FdrawcmdSys(HANDLE hdev)
{
    m_hdev.reset(hdev);
}

bool FdrawcmdSys::Ioctl(DWORD code, void* inbuf, int insize, void* outbuf, int outsize, DWORD* returned)
{
    DWORD returned_local;
    const auto pReturned = returned == nullptr ? &returned_local : returned;
    *pReturned = 0;
    return !!DeviceIoControl(m_hdev.get(), code, inbuf, insize, outbuf, outsize, pReturned, nullptr);
}

constexpr uint8_t FdrawcmdSys::DtlFromSize(int size)
{
    // Data length used only for 128-byte sectors.
    return (size == 0) ? 0x80 : 0xff;
}

util::Version& FdrawcmdSys::GetVersion()
{
    if (m_driver_version.value == 0)
        if (!GetVersion(m_driver_version))
            throw util::exception("GetVersion error in fdrawcmd.sys");
    return m_driver_version;
}

FD_FDC_INFO* FdrawcmdSys::GetFdcInfo()
{
    if (!m_fdc_info_queried)
    {
        if (!GetFdcInfo(m_fdc_info))
            return nullptr;
        m_fdc_info_queried = true;
    }
    return &m_fdc_info;
}

int FdrawcmdSys::GetMaxTransferSize()
{
    if (m_max_transfer_size == 0)
    {
        GetVersion(); // Required for MaxTransferSize.
        GetFdcInfo(); // Required for MaxTransferSize.
        const auto have_max_transfer_size = m_driver_version.value >= 0x0100010c && m_fdc_info_queried;
                      // Version 12 returns MaxTransferSize. In older version let it be IoBufferSize (32768).
        m_max_transfer_size = have_max_transfer_size ? m_fdc_info.MaxTransferSize : 32768;
    }
    return m_max_transfer_size;
}

////////////////////////////////////////////////////////////////////////////////

bool FdrawcmdSys::GetVersion(util::Version& version)
{
    DWORD dwVersion = 0;
    const auto result = Ioctl(IOCTL_FDRAWCMD_GET_VERSION,
        nullptr, 0,
        &dwVersion, sizeof(dwVersion));
    version.value = dwVersion;
    return result;
}

bool FdrawcmdSys::GetResult(FD_CMD_RESULT& result)
{
    return Ioctl(IOCTL_FD_GET_RESULT,
        nullptr, 0,
        &result, sizeof(result));
bool FdrawcmdSys::SetPerpendicularMode(int ow_ds_gap_wgate)
{
    FD_PERPENDICULAR_PARAMS pp{};
    pp.ow_ds_gap_wgate = lossless_static_cast<uint8_t>(ow_ds_gap_wgate);
    IOCTL_PARAMS ioctl_params{};
    ioctl_params.code = IOCTL_FDCMD_PERPENDICULAR_MODE;
    ioctl_params.inbuf = &pp;
    ioctl_params.insize = sizeof(pp);
    RETURN_IOCTL(ioctl_params, util::format("FdrawcmdSys::SetPerpendicularMode: ow_ds_gap_wgate=", ow_ds_gap_wgate));
}

bool FdrawcmdSys::SetEncRate(Encoding encoding, DataRate datarate)
{
    if (encoding != Encoding::MFM && encoding != Encoding::FM)
        throw util::exception("unsupported encoding (", encoding, ") for fdrawcmd.sys");

    // Set perpendicular mode and write-enable for 1M data rate
    FD_PERPENDICULAR_PARAMS pp{};
    pp.ow_ds_gap_wgate = (datarate == DataRate::_1M) ? 0xbc : 0x00;
    Ioctl(IOCTL_FDCMD_PERPENDICULAR_MODE, &pp, sizeof(pp));

    uint8_t rate;
    switch (datarate)
    {
    case DataRate::_250K:   rate = FD_RATE_250K; break;
    case DataRate::_300K:   rate = FD_RATE_300K; break;
    case DataRate::_500K:   rate = FD_RATE_500K; break;
    case DataRate::_1M:     rate = FD_RATE_1M; break;
    default:
        throw util::exception("unsupported datarate (", datarate, ")");
    }

    m_encoding_flags = encoding == Encoding::MFM ? FD_OPTION_MFM : FD_OPTION_FM;

    return Ioctl(IOCTL_FD_SET_DATA_RATE, &rate, sizeof(rate));
}

bool FdrawcmdSys::SetHeadSettleTime(int ms)
{
    auto hst = static_cast<uint8_t>(std::max(0, std::min(255, ms)));
    return Ioctl(IOCTL_FD_SET_HEAD_SETTLE_TIME, &hst, sizeof(hst));
}

bool FdrawcmdSys::SetMotorTimeout(int seconds)
{
    auto timeout = static_cast<uint8_t>(std::max(0, std::min(3, seconds)));
    return Ioctl(IOCTL_FD_SET_MOTOR_TIMEOUT, &timeout, sizeof(timeout));
}

bool FdrawcmdSys::SetMotorOff()
{
    return Ioctl(IOCTL_FD_MOTOR_OFF);
}

bool FdrawcmdSys::SetDiskCheck(bool enable)
{
    uint8_t check{ static_cast<uint8_t>(enable ? 1 : 0) };
    return Ioctl(IOCTL_FD_SET_DISK_CHECK, &check, sizeof(check));
}

bool FdrawcmdSys::GetFdcInfo(FD_FDC_INFO& info)
{
    return Ioctl(IOCTL_FD_GET_FDC_INFO,
        nullptr, 0,
        &info, sizeof(info));
}

bool FdrawcmdSys::CmdPartId(uint8_t& part_id)
{
    return Ioctl(IOCTL_FDCMD_PART_ID,
        nullptr, 0,
        &part_id, sizeof(part_id));
}

bool FdrawcmdSys::Configure(uint8_t eis_efifo_poll_fifothr, uint8_t pretrk)
{
    FD_CONFIGURE_PARAMS cp{};
    cp.eis_efifo_poll_fifothr = eis_efifo_poll_fifothr;
    cp.pretrk = pretrk;

    return Ioctl(IOCTL_FDCMD_CONFIGURE, &cp, sizeof(cp));
}

bool FdrawcmdSys::Specify(int step_rate, int head_unload_time, int head_load_time)
{
    auto srt = static_cast<uint8_t>(step_rate & 0x0f);
    auto hut = static_cast<uint8_t>(head_unload_time & 0x0f);
    auto hlt = static_cast<uint8_t>(head_load_time & 0x7f);

    FD_SPECIFY_PARAMS sp{};
    sp.srt_hut = static_cast<uint8_t>(srt << 4) | hut;
    sp.hlt_nd = static_cast<uint8_t>(hlt << 1) | 0;

    return Ioctl(IOCTL_FDCMD_SPECIFY, &sp, sizeof(sp));
}

bool FdrawcmdSys::Recalibrate()
{
    // ToDo: should we check TRACK0 and retry if not signalled?
    return Ioctl(IOCTL_FDCMD_RECALIBRATE);
}

bool FdrawcmdSys::Seek(int cyl, int head /*= -1*/)
{
    if (cyl == 0)
        return Recalibrate();

    FD_SEEK_PARAMS sp{};
    sp.cyl = static_cast<uint8_t>(cyl);
    int sp_size = sizeof(sp);
    if (head >= 0)
    {
        if (head < 0 || head > 1)
            throw util::exception("unsupported head (", head, ")");
        sp.head = static_cast<uint8_t>(head);
    }
    else
        sp_size -= sizeof(sp.head);

    return Ioctl(IOCTL_FDCMD_SEEK, &sp, sp_size);
}

bool FdrawcmdSys::RelativeSeek(int head, int offset)
{
    FD_RELATIVE_SEEK_PARAMS rsp{};
    rsp.flags = (offset > 0) ? FD_OPTION_DIR : 0;
    rsp.head = static_cast<uint8_t>(head);
    rsp.offset = static_cast<uint8_t>(std::abs(offset));

    return Ioctl(IOCTL_FDCMD_RELATIVE_SEEK, &rsp, sizeof(rsp));
}

bool FdrawcmdSys::CmdVerify(int cyl, int head, int sector, int size, int eot)
{
    return CmdVerify(head, cyl, head, sector, size, eot);
}

bool FdrawcmdSys::CmdVerify(int phead, int cyl, int head, int sector, int size, int eot)
{
    FD_READ_WRITE_PARAMS rwp{};
    rwp.flags = m_encoding_flags;
    rwp.phead = static_cast<uint8_t>(phead);
    rwp.cyl = static_cast<uint8_t>(cyl);
    rwp.head = static_cast<uint8_t>(head);
    rwp.sector = static_cast<uint8_t>(sector);
    rwp.size = static_cast<uint8_t>(size);
    rwp.eot = static_cast<uint8_t>(eot);
    rwp.gap = RW_GAP;
    rwp.datalen = DtlFromSize(size);

    return Ioctl(IOCTL_FDCMD_VERIFY, &rwp, sizeof(rwp));
}

bool FdrawcmdSys::CmdReadTrack(int phead, int cyl, int head, int sector, int size, int eot, MEMORY& mem)
{
    FD_READ_WRITE_PARAMS rwp{};
    rwp.flags = m_encoding_flags;
    rwp.phead = static_cast<uint8_t>(phead);
    rwp.cyl = static_cast<uint8_t>(cyl);
    rwp.head = static_cast<uint8_t>(head);
    rwp.sector = static_cast<uint8_t>(sector);
    rwp.size = static_cast<uint8_t>(size);
    rwp.eot = static_cast<uint8_t>(eot);
    rwp.gap = RW_GAP;
    rwp.datalen = DtlFromSize(size);

    return Ioctl(IOCTL_FDCMD_READ_TRACK,
        &rwp, sizeof(rwp),
        mem, eot * Sector::SizeCodeToLength(rwp.size));
}

bool FdrawcmdSys::CmdRead(int phead, int cyl, int head, int sector, int size, int count, MEMORY& mem, size_t data_offset, bool deleted)
{
    FD_READ_WRITE_PARAMS rwp{};
    rwp.flags = m_encoding_flags;
    rwp.phead = static_cast<uint8_t>(phead);
    rwp.cyl = static_cast<uint8_t>(cyl);
    rwp.head = static_cast<uint8_t>(head);
    rwp.sector = static_cast<uint8_t>(sector);
    rwp.size = static_cast<uint8_t>(size);
    rwp.eot = static_cast<uint8_t>(sector + count);
    rwp.gap = RW_GAP;
    rwp.datalen = DtlFromSize(size);

    return Ioctl(deleted ? IOCTL_FDCMD_READ_DELETED_DATA : IOCTL_FDCMD_READ_DATA,
        &rwp, sizeof(rwp),
        mem + data_offset, count * Sector::SizeCodeToLength(size));
}

bool FdrawcmdSys::CmdWrite(int phead, int cyl, int head, int sector, int size, int count, MEMORY& mem, bool deleted)
{
    FD_READ_WRITE_PARAMS rwp{};
    rwp.flags = m_encoding_flags;
    rwp.phead = static_cast<uint8_t>(phead);
    rwp.cyl = static_cast<uint8_t>(cyl);
    rwp.head = static_cast<uint8_t>(head);
    rwp.sector = static_cast<uint8_t>(sector);
    rwp.size = static_cast<uint8_t>(size);
    rwp.eot = static_cast<uint8_t>(sector + count);
    rwp.gap = RW_GAP;
    rwp.datalen = DtlFromSize(size);

    return Ioctl(deleted ? IOCTL_FDCMD_WRITE_DELETED_DATA : IOCTL_FDCMD_WRITE_DATA,
        &rwp, sizeof(rwp),
        mem, count * Sector::SizeCodeToLength(size));
}

bool FdrawcmdSys::CmdFormat(FD_FORMAT_PARAMS* params, int size)
{
    return Ioctl(IOCTL_FDCMD_FORMAT_TRACK, params, size);
}

bool FdrawcmdSys::CmdFormatAndWrite(FD_FORMAT_PARAMS* params, int size)
{
    return Ioctl(IOCTL_FDCMD_FORMAT_AND_WRITE, params, size);
}

bool FdrawcmdSys::CmdScan(int head, FD_SCAN_RESULT* scan, int size)
{
    FD_SCAN_PARAMS sp{};
    sp.flags = m_encoding_flags;
    sp.head = static_cast<uint8_t>(head);

    return Ioctl(IOCTL_FD_SCAN_TRACK,
        &sp, sizeof(sp),
        scan, size);
}

bool FdrawcmdSys::CmdTimedScan(int head, FD_TIMED_SCAN_RESULT* timed_scan, int size)
{
    FD_SCAN_PARAMS sp{};
    sp.flags = m_encoding_flags;
    sp.head = static_cast<uint8_t>(head);

    return Ioctl(IOCTL_FD_TIMED_SCAN_TRACK,
        &sp, sizeof(sp),
        timed_scan, size);
}

bool FdrawcmdSys::CmdTimedMultiScan(int head, int track_retries,
                                    FD_TIMED_MULTI_SCAN_RESULT* timed_multi_scan, int size,
                                    int byte_tolerance_of_time /* = -1 */)
{
    if (head < 0 || head > 1)
        throw util::exception("unsupported head (", head, ")");
    if (track_retries == 0)
        throw util::exception("unsupported track_retries (", track_retries, ")");
    FD_MULTI_SCAN_PARAMS msp{};
    msp.flags = m_encoding_flags;
    msp.head = lossless_static_cast<uint8_t>(head);
    msp.track_retries = lossless_static_cast<int8_t>(track_retries);
    msp.byte_tolerance_of_time = lossless_static_cast<int8_t>(byte_tolerance_of_time);

    return Ioctl(IOCTL_FD_TIMED_MULTI_SCAN_TRACK,
        &msp, sizeof(msp),
        timed_multi_scan, size);
}

bool FdrawcmdSys::CmdReadId(int head, FD_CMD_RESULT& result)
{
    FD_READ_ID_PARAMS rip{};
    rip.flags = m_encoding_flags;
    rip.head = static_cast<uint8_t>(head);

    return Ioctl(IOCTL_FDCMD_READ_ID,
        &rip, sizeof(rip),
        &result, sizeof(result));
}

bool FdrawcmdSys::FdRawReadTrack(int head, int size, MEMORY& mem)
{
    FD_RAW_READ_PARAMS rrp{};
    rrp.flags = FD_OPTION_MFM;
    rrp.head = static_cast<uint8_t>(head);
    rrp.size = static_cast<uint8_t>(size);

    return Ioctl(IOCTL_FD_RAW_READ_TRACK,
        &rrp, sizeof(rrp),
        mem.pb, mem.size);
}

bool FdrawcmdSys::FdSetSectorOffset(int index)
{
    FD_SECTOR_OFFSET_PARAMS sop{};
    sop.sectors = static_cast<uint8_t>(std::max(0, std::min(255, index)));

    return Ioctl(IOCTL_FD_SET_SECTOR_OFFSET, &sop, sizeof(sop));
}

bool FdrawcmdSys::FdSetShortWrite(int length, int finetune)
{
    FD_SHORT_WRITE_PARAMS swp{};
    swp.length = static_cast<DWORD>(length);
    swp.finetune = static_cast<DWORD>(finetune);

    return Ioctl(IOCTL_FD_SET_SHORT_WRITE, &swp, sizeof(swp));
}

bool FdrawcmdSys::FdGetRemainCount(int& remain)
{
    return Ioctl(IOCTL_FD_GET_REMAIN_COUNT,
        nullptr, 0,
        &remain, sizeof(remain));
}

bool FdrawcmdSys::FdCheckDisk()
{
    return Ioctl(IOCTL_FD_CHECK_DISK);
}

bool FdrawcmdSys::FdGetTrackTime(int& microseconds)
{
    return Ioctl(IOCTL_FD_GET_TRACK_TIME,
        nullptr, 0,
        &microseconds, sizeof(microseconds));
}

bool FdrawcmdSys::FdGetMultiTrackTime(FD_MULTI_TRACK_TIME_RESULT& track_time, uint8_t revolutions /* = 10*/)
{
    return Ioctl(IOCTL_FD_GET_MULTI_TRACK_TIME,
        &revolutions, sizeof(revolutions),
        &track_time, sizeof(FD_MULTI_TRACK_TIME_RESULT));
}

bool FdrawcmdSys::FdReset()
{
    return Ioctl(IOCTL_FD_RESET);
}

#endif // HAVE_FDRAWCMD_H
