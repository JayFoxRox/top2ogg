// Copyright 2016 Jannik Vogel
// Licensed under GPLv2 (Version 2 only).
// Refer to the LICENSE file included.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <vorbis/vorbisenc.h>

#include "top.h"

int main(int argc, char* argv[]) {
  unsigned int packetno = 0;

  ogg_stream_state os;
  ogg_page og;
  ogg_packet op;

  if (argc != 3) {
    printf("Usage: %s <input> <output>\n", argv[0]);
    return 1;
  }
  FILE* fin = fopen(argv[1], "rb");

  // Initialize stream with random serial number
  srand(time(NULL));
  ogg_stream_init(&os,rand());

  FILE* fout = fopen(argv[2], "wb");


  TOPH toph;
  fread(&toph, 1, sizeof(toph), fin);
  printf("Read TOPH: %.4s\n", toph.magic);

  TOPH_extension toph_extension;
  fread(&toph_extension, 1, sizeof(toph_extension), fin);
  printf("Read TOPH extension: 0x%08" PRIX32 ", %" PRIu64 " bytes\n", toph_extension.checksum, toph_extension.size);
  {
    uint64_t remaining = toph_extension.size - 12;
    uint64_t skipped = 0;
    uint64_t processed = 0;

    VIDH vidh;
    fread(&vidh, 1, sizeof(vidh), fin);
    printf("Read VIDH: %.4s, %" PRIu32 " bytes\n", vidh.magic, vidh.size);
    printf("unk1: 0x%08" PRIX32 "\n", vidh.unk1);
    printf("Resolution: %" PRIu32 "x%" PRIu32 "\n", vidh.width, vidh.height);
    printf("FRAM count: %" PRIu32 "\n", vidh.fram_count);
    printf("unk2: 0x%08" PRIX32 " 0x%08" PRIX32 " 0x%08" PRIX32 "\n",
           vidh.unk2[0], vidh.unk2[1], vidh.unk2[2]);
    printf("Guessed frame rate [via unk2]: %.2f\n", (float)vidh.unk2[1] / (float)vidh.unk2[2]);
    printf("Skipping %lu bytes\n", vidh.size - sizeof(vidh));
    fseek(fin, vidh.size - sizeof(vidh), SEEK_CUR);
    remaining -= vidh.size;
    skipped += 8;

    AUDH audh;
    fread(&audh, 1, sizeof(audh), fin);
    printf("Read AUDH: %.4s, %" PRIu64 " bytes\n", audh.magic, audh.size);
    {
      VAUD vaud;
      processed += fread(&vaud, 1, sizeof(vaud), fin);
      printf("Read VAUD: %.4s, %" PRIu32 " Hz\n", vaud.magic, vaud.audio_frequency);

      uint8_t vorbis_setup_header[30];
      processed += fread(vorbis_setup_header, 1, sizeof(vorbis_setup_header), fin);
      printf("Vorbis Setup Header\n");
      {
        ogg_packet header;
        header.packet = vorbis_setup_header;
        header.bytes = sizeof(vorbis_setup_header);
        header.b_o_s = 0;
        header.e_o_s = 0;
        header.granulepos = 0;
        header.packetno = packetno++;
        ogg_stream_packetin(&os, &header);
      }

      // Construct a comment header [necessary but not part of *.top file]
      {
        ogg_packet header;
        vorbis_comment vc;
        vorbis_comment_init(&vc);
        vorbis_comment_add_tag(&vc,"ENCODER","convert.c");
        int ret = vorbis_commentheader_out(&vc, &header);
        ogg_stream_packetin(&os, &header);
      }

#if 1
      uint8_t unk[3]; // 0xCC, 0x02, 0x01
      processed += fread(unk, 1, sizeof(unk), fin);
      printf("Unknown 0x%02" PRIX8 ", 0x%02" PRIX8 ", 0x%02" PRIX8 "\n", unk[0], unk[1], unk[2]);
#endif
     
      printf("at %" PRIu64 "\n", ftell(fin));
 
      uint64_t vorbis_codec_header_size = audh.size - 12 - processed;
      uint8_t* vorbis_codec_header = malloc(vorbis_codec_header_size);
      processed += fread(vorbis_codec_header, 1, vorbis_codec_header_size, fin);
      printf("Vorbis Codec Header (%" PRIu64 " bytes)\n", vorbis_codec_header_size);
      {
        ogg_packet header;
        header.packet = vorbis_codec_header;
        header.bytes = vorbis_codec_header_size; // -1 is still fine, -2 breaks
        header.b_o_s = 0;
        header.e_o_s = 0;
        header.granulepos = 0;
        header.packetno = packetno++;
        ogg_stream_packetin(&os,&header);
      }
     
//      fseek(fin, audh.size - 12 - processed, SEEK_CUR);
      remaining -= audh.size;
      skipped += 12;
    }

    // Somehow the sizes include the header, but they are not actually counted
    // in the parent elements size?!
    fseek(fin, skipped, SEEK_CUR);

    printf("%" PRIu64 " remaining\n", remaining - skipped);
    printf("at %" PRIu64 "\n", ftell(fin));
  }

  // Dump our 3 headers into the output file
  while(1) {
    int result = ogg_stream_flush(&os,&og);
    if(result == 0) {
      break;
    }
    fwrite(og.header,1,og.header_len,fout);
    fwrite(og.body,1,og.body_len,fout);
  }

  //FIXME: Check for EOF
  while(1) {
    printf("at %" PRIu64 "\n", ftell(fin));

    FRAM fram;
    fread(&fram, 1, sizeof(fram), fin);
    printf("Read FRAM: %.4s, %" PRIu64 " bytes\n", fram.magic, fram.size);
    if (memcmp(fram.magic, "MARF", 4)) {
      break;
    }
    {
      uint64_t remaining = fram.size - 12;

      printf("Parsing frame\n");
      uint8_t zero[20]; // Always zero?!
      remaining -= fread(zero, 1, sizeof(zero), fin);
      
      VIDD vidd;
      remaining -= fread(&vidd, 1, sizeof(vidd), fin);
      printf("Read VIDD: %.4s, %" PRIu64 " bytes\n", vidd.magic, vidd.size);
      if (memcmp(vidd.magic, "DDIV", 4)) {
        break;
      }
      fseek(fin, vidd.size - 12, SEEK_CUR);
      remaining -= vidd.size - 12;

      AUDD audd;
      remaining -= fread(&audd, 1, sizeof(audd), fin);
      printf("Read AUDD: %.4s, %" PRIu64 " bytes\n", audd.magic, audd.size);
      if (memcmp(audd.magic, "DDUA", 4)) {
        break;
      }
      uint32_t data_size;
      remaining -= fread(&data_size, 1, sizeof(data_size), fin);
      fseek(fin, 4+2, SEEK_CUR);
      remaining -= 4+2;

      uint8_t* data = malloc(data_size);
      remaining -= fread(data, 1, data_size, fin);
      {
        printf("Data: %" PRIu32 " bytes\n", data_size);
        ogg_packet header;
        header.packet = &data[0];
        printf("foo 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",
               data[0], data[1], data[2], data[3], data[4], data[5]);
        header.bytes = data_size;
        header.b_o_s = 0;
        header.e_o_s = 0;
        header.granulepos = packetno * 1000;
        header.packetno = packetno++;

        ogg_stream_packetin(&os,&header);
      }

      printf("Remaining: %" PRIu64 " bytes\n", remaining);
      fseek(fin, remaining, SEEK_CUR);
    }

    printf("Outputting pages\n");

    /* write out pages (if any) */
    while(1) {
      int result = ogg_stream_pageout(&os,&og);
      if (result == 0) {
        break;
      }
      fwrite(og.header, 1, og.header_len, fout);
      fwrite(og.body, 1, og.body_len, fout);

      // Stolen from sample code, no idea how this works :)
      if (ogg_page_eos(&og)) {
        break;
      }
    }
  }

  // Cleanup
  ogg_stream_clear(&os);

  // Example code said "ogg_page and ogg_packet structs always point to storage
  // in libvorbis.  They're never freed or manipulated directly"

  fclose(fin);
  fclose(fout);

  printf("Done\n");
  return 0;
}
