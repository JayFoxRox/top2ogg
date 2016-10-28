// Copyright 2016 Jannik Vogel
// Licensed under GPLv2 (Version 2 only).
// Refer to the LICENSE file included.

#ifndef __TOP_H__
#define __TOP_H__

#include <stdint.h>

typedef struct {
  char magic[4]; // TOPH
  uint64_t size; // Size of this structure
  uint8_t data[20];
} __attribute__((packed)) TOPH;

typedef struct {
  uint32_t checksum; // ???
  uint64_t size; // Size of extension including checksum
} __attribute__((packed)) TOPH_extension;

typedef struct {
  char magic[4]; // HDIV
  uint32_t size; // Size of this structure [32 bit!!!]
  uint32_t unk1; // Always 1?
  uint16_t width;
  uint16_t height;
  uint32_t fram_count; // number of FRAM
  uint32_t unk2[3];
    // [0] = ???
    //    Maybe these are: 0xBB5 / 100 => 29.97 FPS?!
    // [1] = always 0xBB5?
    // [2] = always 0x64 / 100 ?
  //uint8_t data[];
} __attribute__((packed)) VIDH;

typedef struct {
  char magic[4]; // HDUA
  uint64_t size; // Size of this structure
  //uint8_t data[]; // Contains a VAUD struct?!
} __attribute__((packed)) AUDH;

typedef struct {
  char magic[4]; // DUAV
  uint32_t audio_frequency; // in Hz [44100]
  uint8_t unk1[4]; // 02 01 20 00
  uint8_t unk2[4]; // 00 24 02 00
  uint8_t unk3[4]; // 74 41 00 00
  uint8_t unk4[4]; // 00 00 00 00
  uint8_t unk5[4]; // 00 00 00 00
  uint8_t unk6[4]; // 00 00 00 00
  uint32_t frames; // Number of frames or something? way larger in large files
  uint8_t unk[2]; // E4 01
  //uint8_t data[]; // Contains vorbis packets
} __attribute__((packed)) VAUD;

typedef struct {
  char magic[4]; // MARF
  uint64_t size; // Size of this structure
  //uint8_t data[]; // Contains VIDD struct?! followed by AUDD struct?!
} __attribute__((packed)) FRAM;

typedef struct {
  char magic[4]; // DDAU
  uint64_t size; // Size of this structure
  //uint32_t size2; // Size of data - 1 ?!
  //uint8_t data[];
  // Followed by a 2 byte checksum or something?!
} __attribute__((packed)) AUDD;

typedef struct {
  char magic[4]; // DDIV
  uint64_t size; // Size of this structure
  //uint8_t data[];
} __attribute__((packed)) VIDD;

#endif
