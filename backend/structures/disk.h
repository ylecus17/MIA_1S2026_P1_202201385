#pragma once
#include <cstdint>

struct Partition{
    char PartStatus;
    char PartType;
    char PartFit;
    int32_t PartStart;
    int32_t PartSize;
    char PartName[16];
    int32_t PartCorrelative;
    char PartID[4];
};

struct MBR{
    int32_t MbrSize;
    char MbrCreationDate[20];
    int32_t MbrDiskSignature;
    char DskFit;
    Partition Partitions[4];
};

struct EBR{
    char PartStatus;
    char PartFit;
    int32_t PartStart;
    int32_t PartSize;
    int32_t PartNext;
    char PartName[16];
};