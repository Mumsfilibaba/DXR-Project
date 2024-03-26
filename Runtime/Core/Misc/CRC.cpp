#include "CRC.h"

struct FCRC32Table
{
    constexpr FCRC32Table()
    {
        constexpr uint32 Polynomial = 0xEDB88320;
        for (uint32 Index = 0; Index < 256; Index++)
        {
            uint32 CRC = Index;
            for (uint32 Bit = 0; Bit < 8; Bit++)
            {
                CRC = (CRC >> 1) ^ ((CRC & 1) * Polynomial);
            }
            
            Table[0][Index] = CRC;
        }
        
        for (int32 Index = 0; Index < 256; Index++)
        {
            Table[1][Index] = (Table[0][Index] >> 8) ^ Table[0][Table[0][Index] & 0xFF];
            Table[2][Index] = (Table[1][Index] >> 8) ^ Table[0][Table[1][Index] & 0xFF];
            Table[3][Index] = (Table[2][Index] >> 8) ^ Table[0][Table[2][Index] & 0xFF];
        }
    }
    
    uint32 Table[4][256];
};

uint32 FCRC32::Generate(const void* Source, uint64 SourceSize)
{
    static constexpr FCRC32Table CRCTable;
    
    uint32 Result = 0xFFFFFFFF;

    const uint32* Data32 = reinterpret_cast<const uint32*>(Source);
    for (; SourceSize >= 4; SourceSize -= 4) 
    {
        Result ^= *Data32++;
        Result  = CRCTable.Table[3][Result & 0xFF] ^ CRCTable.Table[2][(Result >> 8) & 0xFF] ^ CRCTable.Table[1][(Result >> 16) & 0xFF] ^ CRCTable.Table[0][(Result >> 24) & 0xFF];
    }

    const uint8* Data = reinterpret_cast<const uint8*>(Data32);
    for (; SourceSize > 0; SourceSize--) 
    {
        Result = (Result >> 8) ^ CRCTable.Table[0][(Result & 0xFF) ^ *Data++];
    }

    return ~Result;
}
