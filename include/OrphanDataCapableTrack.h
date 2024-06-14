#ifndef ORPHANDATACAPABLETRACK_H
#define ORPHANDATACAPABLETRACK_H
//---------------------------------------------------------------------------

class OrphanDataCapableTrack; // Required because this and PhysicalTrackMFM includes each other.

#include "PhysicalTrackMFM.h"
#include "Track.h"

class OrphanDataCapableTrack
{
public:
    bool empty() const;
    DataRate getDataRate() const;
    Encoding getEncoding() const;
    int getTimeOfOffset(const int offset) const;
    int getOffsetOfTime(const int time) const;
    int getTrackLen() const;
    void setTrackLen(const int trackLen);
    void addTrackLen(const int trackLen);
    int getTrackTime() const;
    void setTrackTime(const int trackTime);
    void addTrackTime(const int trackTime);
    void add(OrphanDataCapableTrack&& orphanDataCapableTrack);
    int size() const;
    int Score() const;

    static void MergeOrphansIntoParents(Track& orphansTrack, Track& parentsTrack, bool removeOrphanAfterMerge, const std::function<bool (const Sector &)>& considerParentSectorPredicate = nullptr);

    void MergeOrphansIntoParents(bool removeOrphanAfterMerge);
    void MergeInto(Track& targetTrack, const std::function<bool (const Sector &)>& considerTargetSectorPredicate = nullptr);
    void syncThisToOtherAsMulti(const int offsetDiffBest, OrphanDataCapableTrack& targetODCTrack);

protected:
    void join();
    void disjoin();

public:
    void TuneOffsetsToEachOtherByMin(OrphanDataCapableTrack& otherOrphanDataCapableTrack);
    void syncUnlimitedToOffset(const int syncOffset);
    void syncLimitedToOffset(const int syncOffset);
    void demultiAndSyncUnlimitedToOffset(const int syncOffset, const int trackLenSingle);

    int determineBestTrackLen(const int timedTrackTime) const;
    void FixOffsetsByTimedToAvoidRepeatedSectorWhenMerging(Track& timedTrack, const RepeatedSectors& repeatedSectorIds);
    void ShowOffsets() const;

    Track track{};
    Track orphanDataTrack{};
    bool cylheadMismatch = false;
    int trackIndexOffset = 0;
};

//---------------------------------------------------------------------------
#endif // ORPHANDATACAPABLETRACK_H
