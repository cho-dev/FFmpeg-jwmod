/*
 * Copyright (c) 2012
 *      MIPS Technologies, Inc., California.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the MIPS Technologies, Inc., nor the names of is
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE MIPS TECHNOLOGIES, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE MIPS TECHNOLOGIES, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Author:  Stanislav Ocovaj (stanislav.ocovaj imgtec com)
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#ifndef AVUTIL_SOFTFLOAT_TABLES_H
#define AVUTIL_SOFTFLOAT_TABLES_H

static const int32_t av_sqrttbl_sf[512+1] = { /*  sqrt(x), 0.5<=x<1 */
   0x2d413ccd,0x2d4c8bb3,0x2d57d7c6,0x2d63210a,
   0x2d6e677f,0x2d79ab2a,0x2d84ec0b,0x2d902a23,
   0x2d9b6578,0x2da69e08,0x2db1d3d6,0x2dbd06e6,
   0x2dc83738,0x2dd364ce,0x2dde8fac,0x2de9b7d2,
   0x2df4dd43,0x2e000000,0x2e0b200c,0x2e163d68,
   0x2e215816,0x2e2c701a,0x2e378573,0x2e429824,
   0x2e4da830,0x2e58b598,0x2e63c05d,0x2e6ec883,
   0x2e79ce0a,0x2e84d0f5,0x2e8fd144,0x2e9acefb,
   0x2ea5ca1b,0x2eb0c2a7,0x2ebbb89e,0x2ec6ac04,
   0x2ed19cda,0x2edc8b23,0x2ee776df,0x2ef26012,
   0x2efd46bb,0x2f082add,0x2f130c7b,0x2f1deb95,
   0x2f28c82e,0x2f33a246,0x2f3e79e1,0x2f494eff,
   0x2f5421a3,0x2f5ef1ce,0x2f69bf81,0x2f748abe,
   0x2f7f5388,0x2f8a19e0,0x2f94ddc7,0x2f9f9f3e,
   0x2faa5e48,0x2fb51ae8,0x2fbfd51c,0x2fca8ce9,
   0x2fd5424e,0x2fdff54e,0x2feaa5eb,0x2ff55426,
   0x30000000,0x300aa97b,0x3015509a,0x301ff55c,
   0x302a97c5,0x303537d5,0x303fd58e,0x304a70f2,
   0x30550a01,0x305fa0be,0x306a352a,0x3074c747,
   0x307f5716,0x3089e499,0x30946fd2,0x309ef8c0,
   0x30a97f67,0x30b403c7,0x30be85e2,0x30c905bb,
   0x30d38351,0x30ddfea6,0x30e877bc,0x30f2ee96,
   0x30fd6332,0x3107d594,0x311245bc,0x311cb3ad,
   0x31271f67,0x313188ec,0x313bf03d,0x3146555c,
   0x3150b84a,0x315b1909,0x31657798,0x316fd3fc,
   0x317a2e34,0x31848642,0x318edc28,0x31992fe5,
   0x31a3817d,0x31add0f0,0x31b81e40,0x31c2696e,
   0x31ccb27b,0x31d6f969,0x31e13e38,0x31eb80eb,
   0x31f5c182,0x32000000,0x320a3c65,0x321476b1,
   0x321eaee8,0x3228e50a,0x32331917,0x323d4b13,
   0x32477afc,0x3251a8d6,0x325bd4a2,0x3265fe5f,
   0x32702611,0x327a4bb8,0x32846f55,0x328e90e9,
   0x3298b076,0x32a2cdfd,0x32ace97e,0x32b702fd,
   0x32c11a79,0x32cb2ff3,0x32d5436d,0x32df54e9,
   0x32e96466,0x32f371e8,0x32fd7d6d,0x330786f9,
   0x33118e8c,0x331b9426,0x332597cb,0x332f9979,
   0x33399933,0x334396fa,0x334d92cf,0x33578cb2,
   0x336184a6,0x336b7aab,0x33756ec3,0x337f60ed,
   0x3389512d,0x33933f83,0x339d2bef,0x33a71672,
   0x33b0ff10,0x33bae5c7,0x33c4ca99,0x33cead88,
   0x33d88e95,0x33e26dbf,0x33ec4b09,0x33f62673,
   0x34000000,0x3409d7af,0x3413ad82,0x341d817a,
   0x34275397,0x343123db,0x343af248,0x3444bedd,
   0x344e899d,0x34585288,0x3462199f,0x346bdee3,
   0x3475a254,0x347f63f5,0x348923c6,0x3492e1c9,
   0x349c9dfe,0x34a65865,0x34b01101,0x34b9c7d2,
   0x34c37cda,0x34cd3018,0x34d6e18f,0x34e0913f,
   0x34ea3f29,0x34f3eb4d,0x34fd95ae,0x35073e4c,
   0x3510e528,0x351a8a43,0x35242d9d,0x352dcf39,
   0x35376f16,0x35410d36,0x354aa99a,0x35544442,
   0x355ddd2f,0x35677463,0x357109df,0x357a9da2,
   0x35842fb0,0x358dc007,0x35974ea9,0x35a0db98,
   0x35aa66d3,0x35b3f05c,0x35bd7833,0x35c6fe5a,
   0x35d082d3,0x35da059c,0x35e386b7,0x35ed0626,
   0x35f683e8,0x36000000,0x36097a6e,0x3612f331,
   0x361c6a4d,0x3625dfc1,0x362f538f,0x3638c5b7,
   0x36423639,0x364ba518,0x36551252,0x365e7deb,
   0x3667e7e2,0x36715039,0x367ab6f0,0x36841c07,
   0x368d7f81,0x3696e15d,0x36a0419d,0x36a9a040,
   0x36b2fd49,0x36bc58b8,0x36c5b28e,0x36cf0acb,
   0x36d86170,0x36e1b680,0x36eb09f8,0x36f45bdc,
   0x36fdac2b,0x3706fae7,0x37104810,0x371993a7,
   0x3722ddad,0x372c2622,0x37356d08,0x373eb25f,
   0x3747f629,0x37513865,0x375a7914,0x3763b838,
   0x376cf5d0,0x377631e0,0x377f6c64,0x3788a561,
   0x3791dcd6,0x379b12c4,0x37a4472c,0x37ad7a0e,
   0x37b6ab6a,0x37bfdb44,0x37c90999,0x37d2366d,
   0x37db61be,0x37e48b8e,0x37edb3de,0x37f6daae,
   0x38000000,0x380923d3,0x3812462a,0x381b6703,
   0x38248660,0x382da442,0x3836c0aa,0x383fdb97,
   0x3848f50c,0x38520d09,0x385b238d,0x3864389b,
   0x386d4c33,0x38765e55,0x387f6f01,0x38887e3b,
   0x38918c00,0x389a9853,0x38a3a334,0x38acaca3,
   0x38b5b4a3,0x38bebb32,0x38c7c051,0x38d0c402,
   0x38d9c645,0x38e2c71b,0x38ebc685,0x38f4c482,
   0x38fdc114,0x3906bc3c,0x390fb5fa,0x3918ae4f,
   0x3921a53a,0x392a9abe,0x39338edb,0x393c8192,
   0x394572e2,0x394e62ce,0x39575155,0x39603e77,
   0x39692a36,0x39721494,0x397afd8f,0x3983e527,
   0x398ccb60,0x3995b039,0x399e93b2,0x39a775cc,
   0x39b05689,0x39b935e8,0x39c213e9,0x39caf08e,
   0x39d3cbd9,0x39dca5c7,0x39e57e5b,0x39ee5596,
   0x39f72b77,0x3a000000,0x3a08d331,0x3a11a50a,
   0x3a1a758d,0x3a2344ba,0x3a2c1291,0x3a34df13,
   0x3a3daa41,0x3a46741b,0x3a4f3ca3,0x3a5803d7,
   0x3a60c9ba,0x3a698e4b,0x3a72518b,0x3a7b137c,
   0x3a83d41d,0x3a8c936f,0x3a955173,0x3a9e0e29,
   0x3aa6c992,0x3aaf83ae,0x3ab83c7e,0x3ac0f403,
   0x3ac9aa3c,0x3ad25f2c,0x3adb12d1,0x3ae3c52d,
   0x3aec7642,0x3af5260e,0x3afdd492,0x3b0681d0,
   0x3b0f2dc6,0x3b17d878,0x3b2081e4,0x3b292a0c,
   0x3b31d0f0,0x3b3a7690,0x3b431aec,0x3b4bbe06,
   0x3b545fdf,0x3b5d0077,0x3b659fcd,0x3b6e3de4,
   0x3b76daba,0x3b7f7651,0x3b8810aa,0x3b90a9c4,
   0x3b9941a1,0x3ba1d842,0x3baa6da5,0x3bb301cd,
   0x3bbb94b9,0x3bc4266a,0x3bccb6e2,0x3bd5461f,
   0x3bddd423,0x3be660ee,0x3beeec81,0x3bf776dc,
   0x3c000000,0x3c0887ed,0x3c110ea4,0x3c199426,
   0x3c221872,0x3c2a9b8a,0x3c331d6e,0x3c3b9e1d,
   0x3c441d9a,0x3c4c9be5,0x3c5518fd,0x3c5d94e3,
   0x3c660f98,0x3c6e891d,0x3c770172,0x3c7f7898,
   0x3c87ee8e,0x3c906356,0x3c98d6ef,0x3ca1495b,
   0x3ca9ba9a,0x3cb22aac,0x3cba9992,0x3cc3074c,
   0x3ccb73dc,0x3cd3df41,0x3cdc497b,0x3ce4b28c,
   0x3ced1a73,0x3cf58132,0x3cfde6c8,0x3d064b37,
   0x3d0eae7f,0x3d17109f,0x3d1f719a,0x3d27d16e,
   0x3d30301d,0x3d388da8,0x3d40ea0d,0x3d49454f,
   0x3d519f6d,0x3d59f867,0x3d625040,0x3d6aa6f6,
   0x3d72fc8b,0x3d7b50fe,0x3d83a451,0x3d8bf683,
   0x3d944796,0x3d9c9788,0x3da4e65c,0x3dad3412,
   0x3db580a9,0x3dbdcc24,0x3dc61680,0x3dce5fc0,
   0x3dd6a7e4,0x3ddeeeed,0x3de734d9,0x3def79ab,
   0x3df7bd62,0x3e000000,0x3e084184,0x3e1081ee,
   0x3e18c140,0x3e20ff7a,0x3e293c9c,0x3e3178a7,
   0x3e39b39a,0x3e41ed77,0x3e4a263d,0x3e525def,
   0x3e5a948b,0x3e62ca12,0x3e6afe85,0x3e7331e4,
   0x3e7b642f,0x3e839567,0x3e8bc58c,0x3e93f49f,
   0x3e9c22a1,0x3ea44f91,0x3eac7b6f,0x3eb4a63e,
   0x3ebccffb,0x3ec4f8aa,0x3ecd2049,0x3ed546d9,
   0x3edd6c5a,0x3ee590cd,0x3eedb433,0x3ef5d68c,
   0x3efdf7d7,0x3f061816,0x3f0e3749,0x3f165570,
   0x3f1e728c,0x3f268e9d,0x3f2ea9a4,0x3f36c3a0,
   0x3f3edc93,0x3f46f47c,0x3f4f0b5d,0x3f572135,
   0x3f5f3606,0x3f6749cf,0x3f6f5c90,0x3f776e4a,
   0x3f7f7efe,0x3f878eab,0x3f8f9d53,0x3f97aaf6,
   0x3f9fb793,0x3fa7c32c,0x3fafcdc1,0x3fb7d752,
   0x3fbfdfe0,0x3fc7e76b,0x3fcfedf3,0x3fd7f378,
   0x3fdff7fc,0x3fe7fb7f,0x3feffe00,0x3ff7ff80,
   0x3fffffff,
};

static const int32_t av_sqr_exp_multbl_sf[2] = {
   0x20000000,0x2d413ccd,
};

static const int32_t av_costbl_1_sf[16] = {
   0x40000000,0x3ec52fa0,0x3b20d79e,0x3536cc52,
   0x2d413ccd,0x238e7673,0x187de2a7,0x0c7c5c1e,
   0x00000000,0xf383a3e3,0xe7821d5a,0xdc71898e,
   0xd2bec334,0xcac933af,0xc4df2863,0xc13ad061,
};

static const int32_t av_costbl_2_sf[32] = {
   0x40000000,0x3fffb10b,0x3ffec42d,0x3ffd3969,
   0x3ffb10c1,0x3ff84a3c,0x3ff4e5e0,0x3ff0e3b6,
   0x3fec43c7,0x3fe7061f,0x3fe12acb,0x3fdab1d9,
   0x3fd39b5a,0x3fcbe75e,0x3fc395f9,0x3fbaa740,
   0x3fb11b48,0x3fa6f228,0x3f9c2bfb,0x3f90c8da,
   0x3f84c8e2,0x3f782c30,0x3f6af2e3,0x3f5d1d1d,
   0x3f4eaafe,0x3f3f9cab,0x3f2ff24a,0x3f1fabff,
   0x3f0ec9f5,0x3efd4c54,0x3eeb3347,0x3ed87efc,
};

static const int32_t av_sintbl_2_sf[32] = {
   0x00000000,0x006487c4,0x00c90e90,0x012d936c,
   0x0192155f,0x01f69373,0x025b0caf,0x02bf801a,
   0x0323ecbe,0x038851a2,0x03ecadcf,0x0451004d,
   0x04b54825,0x0519845e,0x057db403,0x05e1d61b,
   0x0645e9af,0x06a9edc9,0x070de172,0x0771c3b3,
   0x07d59396,0x08395024,0x089cf867,0x09008b6a,
   0x09640837,0x09c76dd8,0x0a2abb59,0x0a8defc3,
   0x0af10a22,0x0b540982,0x0bb6ecef,0x0c19b374,
};

static const int32_t av_costbl_3_sf[32] = {
   0x40000000,0x3fffffec,0x3fffffb1,0x3fffff4e,
   0x3ffffec4,0x3ffffe13,0x3ffffd39,0x3ffffc39,
   0x3ffffb11,0x3ffff9c1,0x3ffff84a,0x3ffff6ac,
   0x3ffff4e6,0x3ffff2f8,0x3ffff0e3,0x3fffeea7,
   0x3fffec43,0x3fffe9b7,0x3fffe705,0x3fffe42a,
   0x3fffe128,0x3fffddff,0x3fffdaae,0x3fffd736,
   0x3fffd396,0x3fffcfcf,0x3fffcbe0,0x3fffc7ca,
   0x3fffc38c,0x3fffbf27,0x3fffba9b,0x3fffb5e7,
};

static const int32_t av_sintbl_3_sf[32] = {
   0x00000000,0x0003243f,0x0006487f,0x00096cbe,
   0x000c90fe,0x000fb53d,0x0012d97c,0x0015fdbb,
   0x001921fb,0x001c463a,0x001f6a79,0x00228eb8,
   0x0025b2f7,0x0028d736,0x002bfb74,0x002f1fb3,
   0x003243f1,0x00356830,0x00388c6e,0x003bb0ac,
   0x003ed4ea,0x0041f928,0x00451d66,0x004841a3,
   0x004b65e1,0x004e8a1e,0x0051ae5b,0x0054d297,
   0x0057f6d4,0x005b1b10,0x005e3f4c,0x00616388,
};

static const int32_t av_costbl_4_sf[33] = {
   0x40000000,0x40000000,0x40000000,0x40000000,
   0x40000000,0x40000000,0x3fffffff,0x3fffffff,
   0x3fffffff,0x3ffffffe,0x3ffffffe,0x3ffffffe,
   0x3ffffffd,0x3ffffffd,0x3ffffffc,0x3ffffffc,
   0x3ffffffb,0x3ffffffa,0x3ffffffa,0x3ffffff9,
   0x3ffffff8,0x3ffffff7,0x3ffffff7,0x3ffffff6,
   0x3ffffff5,0x3ffffff4,0x3ffffff3,0x3ffffff2,
   0x3ffffff1,0x3ffffff0,0x3fffffef,0x3fffffed,
   0x3fffffec,
};

static const int32_t av_sintbl_4_sf[33] = {
   0x00000000,0x00001922,0x00003244,0x00004b66,
   0x00006488,0x00007daa,0x000096cc,0x0000afee,
   0x0000c910,0x0000e232,0x0000fb54,0x00011476,
   0x00012d98,0x000146ba,0x00015fdc,0x000178fe,
   0x00019220,0x0001ab42,0x0001c464,0x0001dd86,
   0x0001f6a8,0x00020fca,0x000228ec,0x0002420e,
   0x00025b30,0x00027452,0x00028d74,0x0002a696,
   0x0002bfb7,0x0002d8d9,0x0002f1fb,0x00030b1d,
   0x0003243f,
};
#endif /* AVUTIL_SOFTFLOAT_TABLES_H */