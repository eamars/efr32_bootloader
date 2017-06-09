/*
 * File: rail-phy-configurations.h
 * Description: Supported RAIL configurations.
 *
 * Author(s): Viktor Huszar, viktor.huszar@silabs.com
 *
 * Copyright 2015 by Silicon Laboratories. All rights reserved.
 */

//------------------------------------------------------------------------------
// US FCC 434

#define RAIL_CONFIG                \
{                                  \
  0x01010FF4UL, 0x00000000UL,      \
  0x01010FF8UL, 0x0003C000UL,      \
  0x01010FFCUL, 0x0003C008UL,      \
  0x00010004UL, 0x00157001UL,      \
  0x00010008UL, 0x0000007FUL,      \
  0x00010018UL, 0x00000000UL,      \
  0x0001001CUL, 0x00000000UL,      \
  0x00010028UL, 0x00000000UL,      \
  0x0001002CUL, 0x00000000UL,      \
  0x00010030UL, 0x00000000UL,      \
  0x00010034UL, 0x00000000UL,      \
  0x0001003CUL, 0x00000000UL,      \
  0x00010040UL, 0x000007A0UL,      \
  0x00010048UL, 0x00000000UL,      \
  0x00010054UL, 0x00000000UL,      \
  0x00010058UL, 0x00000000UL,      \
  0x000100A0UL, 0x00004000UL,      \
  0x000100A4UL, 0x00004CFFUL,      \
  0x000100A8UL, 0x00004100UL,      \
  0x000100ACUL, 0x00004DFFUL,      \
  0x00012000UL, 0x00000704UL,      \
  0x00012010UL, 0x00000000UL,      \
  0x00012018UL, 0x00008408UL,      \
  0x00013008UL, 0x0000AC3FUL,      \
  0x0001302CUL, 0x021E8000UL,      \
  0x00013030UL, 0x00108000UL,      \
  0x00013034UL, 0x00000013UL,      \
  0x0001303CUL, 0x0000A000UL,      \
  0x00013040UL, 0x00000000UL,      \
  0x000140A0UL, 0x0F00277AUL,      \
  0x000140F4UL, 0x00001020UL,      \
  0x00014134UL, 0x00000880UL,      \
  0x00014138UL, 0x000087F6UL,      \
  0x00014140UL, 0x008800E0UL,      \
  0x00014144UL, 0x1153E6C1UL,      \
  0x00016014UL, 0x00000010UL,      \
  0x00016018UL, 0x04000000UL,      \
  0x0001601CUL, 0x0002400FUL,      \
  0x00016020UL, 0x000020C8UL,      \
  0x00016024UL, 0x000AD000UL,      \
  0x00016028UL, 0x03000000UL,      \
  0x0001602CUL, 0x00000000UL,      \
  0x00016030UL, 0x00FF17E8UL,      \
  0x00016034UL, 0x00000820UL,      \
  0x00016038UL, 0x02020038UL,      \
  0x0001603CUL, 0x00100012UL,      \
  0x00016040UL, 0x00002BB4UL,      \
  0x00016044UL, 0x00000000UL,      \
  0x00016048UL, 0x04600617UL,      \
  0x0001604CUL, 0x00000000UL,      \
  0x00016050UL, 0x002303F0UL,      \
  0x00016054UL, 0x00000000UL,      \
  0x00016058UL, 0x00000000UL,      \
  0x0001605CUL, 0x22140A04UL,      \
  0x00016060UL, 0x504B4133UL,      \
  0x00016064UL, 0x00000000UL,      \
  0x00017014UL, 0x000270FEUL,      \
  0x00017018UL, 0x00000300UL,      \
  0x0001701CUL, 0x81B10060UL,      \
  0x00017028UL, 0x00000000UL,      \
  0x00017048UL, 0x0000383EUL,      \
  0x0001704CUL, 0x000025BCUL,      \
  0x00017070UL, 0x00020103UL,      \
  0x00017074UL, 0x00000112UL,      \
  0x00017078UL, 0x006D8480UL,      \
  0xFFFFFFFFUL }

#define RAIL_CHANNEL_CONFIG        \
  {0, 0, 500000, 434000000}
