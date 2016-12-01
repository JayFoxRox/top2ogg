// Copyright 2016 Jannik Vogel
// Licensed under GPLv2 (Version 2 only).
// Refer to the LICENSE file included.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

#define NO_MAIN
#include "support/avi-example.c" // Private file due to licensing issue.. soz :(

#include "top.h"

int main(int argc, char* argv[]) {
  unsigned int packetno = 0;






  OutputStream video_st = { 0 }, audio_st = { 0 };

  AVCodec *audio_codec, *video_codec;
  int ret;
  int have_video = 0, have_audio = 0;
  AVDictionary *opt = NULL;
  int i;




  if (argc != 3) {
    printf("Usage: %s <input> <output>\n", argv[0]);
    return 1;
  }

  const char* in_path = argv[1];
  const char* out_path = argv[2];

  // Initialize libavcodec, and register all codecs and formats.
  av_register_all();


  // Load input
  
  FILE* fin = fopen(in_path, "rb");

#if 0
  // Initialize stream with random serial number
  srand(time(NULL));
  ogg_stream_init(&os,rand());
#endif

  //FILE* fout = fopen(argv[2], "wb");

/* allocate the output media context */
    AVFormatContext *oc = NULL;
    avformat_alloc_output_context2(&oc, NULL, "avi", out_path);
    if (!oc) {
        printf("Could not deduce output format from file extension.\n");
        return 1;
    }
    if (!oc) {
      return 1;
    }

    AVOutputFormat* fmt = oc->oformat;

    fmt->video_codec = AV_CODEC_ID_NONE; //MPEG4;
    fmt->audio_codec = AV_CODEC_ID_VORBIS;

    /* Add the audio and video streams using the default format codecs
     * and initialize the codecs. */
    if (fmt->video_codec != AV_CODEC_ID_NONE) {
      add_stream(&video_st, oc, &video_codec, fmt->video_codec);
      have_video = 1;
    }
    if (fmt->audio_codec != AV_CODEC_ID_NONE) {
      add_stream(&audio_st, oc, &audio_codec, fmt->audio_codec);
      have_audio = 1;
    }

    av_dump_format(oc, 0, out_path, 1);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
      ret = avio_open(&oc->pb, out_path, AVIO_FLAG_WRITE);
      if (ret < 0) {
        fprintf(stderr, "Could not open '%s': %s\n", out_path, av_err2str(ret));
        return 1;
      }
    }




  uint8_t vorbis_setup_header[30];
  // Random comment header.. [dumped from some FFmpeg generated file]
  uint8_t vorbis_comment_header[] = {
    0x03, 0x76, 0x6F, 0x72, 0x62, 0x69, 0x73, 0x2C,
    0x00, 0x00, 0x00, 0x58, 0x69, 0x70, 0x68, 0x2E,
    0x4F, 0x72, 0x67, 0x20, 0x6C, 0x69, 0x62, 0x56,
    0x6F, 0x72, 0x62, 0x69, 0x73, 0x20, 0x49, 0x20,
    0x32, 0x30, 0x31, 0x35, 0x30, 0x31, 0x30, 0x35,
    0x20, 0x28, 0xE2, 0x9B, 0x84, 0xE2, 0x9B, 0x84,
    0xE2, 0x9B, 0x84, 0xE2, 0x9B, 0x84, 0x29, 0x01,
    0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x65,
    0x6E, 0x63, 0x6F, 0x64, 0x65, 0x72, 0x3D, 0x4C,
    0x61, 0x76, 0x63, 0x35, 0x37, 0x2E, 0x34, 0x38,
    0x2E, 0x31, 0x30, 0x31, 0x01
  };
  uint64_t vorbis_codec_header_size = 0;
  uint8_t* vorbis_codec_header = NULL;


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

    if (have_video) {
      AVCodecParameters *par = video_st.st->codecpar;

      par->width = vidh.width;
      par->height = vidh.height;
      par->codec_type = AVMEDIA_TYPE_VIDEO;
      par->codec_id   = fmt->video_codec;
      par->codec_tag = 0x30355844;
      par->format = AV_PIX_FMT_YUV420P;
      par->extradata_size = 0;
    }

    AUDH audh;
    fread(&audh, 1, sizeof(audh), fin);
    printf("Read AUDH: %.4s, %" PRIu64 " bytes\n", audh.magic, audh.size);
    {
      VAUD vaud;
      processed += fread(&vaud, 1, sizeof(vaud), fin);
      printf("Read VAUD: %.4s, %" PRIu32 " Hz\n", vaud.magic, vaud.audio_frequency);

      processed += fread(vorbis_setup_header, 1, sizeof(vorbis_setup_header), fin);
      printf("Vorbis Setup Header\n");
#if 0
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
#endif

      // Construct a comment header [necessary but not part of *.top file]
#if 0
      {
        ogg_packet header;
        vorbis_comment vc;
        vorbis_comment_init(&vc);
        vorbis_comment_add_tag(&vc,"ENCODER","convert.c");
        int ret = vorbis_commentheader_out(&vc, &header);
        ogg_stream_packetin(&os, &header);
      }
#endif


#if 1
      uint8_t unk[3]; // 0xCC, 0x02, 0x01
      processed += fread(unk, 1, sizeof(unk), fin);
      printf("Unknown 0x%02" PRIX8 ", 0x%02" PRIX8 ", 0x%02" PRIX8 "\n", unk[0], unk[1], unk[2]);
#endif
     
      printf("at %" PRIu64 "\n", ftell(fin));
 
      vorbis_codec_header_size = audh.size - 12 - processed;
      vorbis_codec_header = malloc(vorbis_codec_header_size);
      processed += fread(vorbis_codec_header, 1, vorbis_codec_header_size, fin);
      printf("Vorbis Codec Header (%" PRIu64 " bytes)\n", vorbis_codec_header_size);
#if 0
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
#endif
     
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

  // Dump our 3 audio headers into the output file

#if 0
  while(1) {
    int result = ogg_stream_flush(&os,&og);
    if(result == 0) {
      break;
    }
    fwrite(og.header,1,og.header_len,fout);
    fwrite(og.body,1,og.body_len,fout);
  }
#else

  if (have_audio) {

    AVCodecParameters *par = audio_st.st->codecpar;

    printf("extra size: %d\n", par->extradata_size);

    par->channels = 2; // Not necessary?!
    par->sample_rate = 44100; // FIXME: Useless here?
    par->bit_rate = 100; //FIXME: What to put here?!
    par->format = AV_SAMPLE_FMT_S16; //FIXME: Necessary?!
    par->codec_type = AVMEDIA_TYPE_AUDIO;
    par->codec_id   = fmt->audio_codec;
    //par->bit_rate = bytestream_get_le32(&p); // nominal bitrate
    unsigned int len = sizeof(vorbis_setup_header) + sizeof(vorbis_comment_header) + vorbis_codec_header_size;
    par->extradata = av_malloc(64 + len + len/255);

    unsigned int offset = 0;

    par->extradata[offset++] = 2; // 2 lace sizes following
    offset += av_xiphlacing(&par->extradata[offset], sizeof(vorbis_setup_header));
    offset += av_xiphlacing(&par->extradata[offset], sizeof(vorbis_comment_header));
    printf("Wrote %d bytes!\n", offset);

    memcpy(&par->extradata[offset], vorbis_setup_header, sizeof(vorbis_setup_header));
    offset += sizeof(vorbis_setup_header);
    memcpy(&par->extradata[offset], vorbis_comment_header, sizeof(vorbis_comment_header));
    offset += sizeof(vorbis_comment_header);
    memcpy(&par->extradata[offset], vorbis_codec_header, vorbis_codec_header_size);
    offset += vorbis_codec_header_size;

    par->extradata_size = offset;
  }
#endif

  ret = avformat_write_header(oc, &opt);
  if (ret < 0) {
      fprintf(stderr, "Error occurred when opening output file: %s\n",
              av_err2str(ret));
      return 1;
  }

  //FIXME: Check for EOF
  unsigned int fram_index = 0;
  while(1) {
    printf("at %" PRIu64 "\n", ftell(fin));

    FRAM fram;
    fread(&fram, 1, sizeof(fram), fin);
    printf("Read FRAM[%u]: %.4s, %" PRIu64 " bytes\n", fram_index, fram.magic, fram.size);
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
      } else {
        uint32_t data_size = vidd.size - 12;
        uint8_t* data = av_malloc(data_size + AV_INPUT_BUFFER_PADDING_SIZE);
        remaining -= fread(data, 1, data_size, fin);

        if (have_video) {
          AVPacket pkt = { 0 }; // data and size must be 0;
          #define SKIP 0
          av_packet_from_data(&pkt, &data[SKIP], data_size - SKIP);
          write_frame(oc, NULL, video_st.st, &pkt);
        }
      }

      AUDD audd;
      remaining -= fread(&audd, 1, sizeof(audd), fin);
      printf("Read AUDD: %.4s, %" PRIu64 " bytes\n", audd.magic, audd.size);
      if (memcmp(audd.magic, "DDUA", 4)) {
        break;
      } else {
        uint32_t data_size;
        remaining -= fread(&data_size, 1, sizeof(data_size), fin);
        data_size -= 8;

        uint16_t unk_dat[3];
        remaining -= fread(&unk_dat, 1, sizeof(unk_dat), fin);

        uint16_t unk_eight = unk_dat[0];
        if (unk_eight != 0x0008) {
          printf("Oops?!\n");
          return 2;
        }
        uint16_t unk_four = unk_dat[1];
        uint16_t unk_len = unk_dat[2] >> 4;
        printf("Len thing: %d [%d] == %" PRIu32 " < %" PRIu64 " ??\n", unk_len, unk_dat[2] & 0xF, data_size, audd.size);

unsigned int skip = 0;
if (unk_four & 0xFF)  {
  skip = 2;
}

uint32_t unk_tot = (unk_dat[1] << 16) | unk_dat[2];

        printf("X%04X %" PRIu32 "\n", unk_four, unk_tot >> 19);


fseek(fin, skip, SEEK_CUR);
remaining -= skip;


        uint8_t* data = av_malloc(data_size + AV_INPUT_BUFFER_PADDING_SIZE);
        remaining -= fread(data, 1, data_size, fin);

        printf("Data: %" PRIu32 " bytes\n", data_size);
        printf("foo 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X ...",
               data[0], data[1], data[2], data[3], data[4], data[5]);
        uint8_t* b = &data[data_size];
        printf(" 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",
               b[-6], b[-5], b[-4], b[-3], b[-2], b[-1]);

#if 0
        {
          ogg_packet header;
          header.packet = &data[0];
          header.bytes = data_size;
          header.b_o_s = 0;
          header.e_o_s = 0;
          header.granulepos = 0;//packetno * 1000;
          header.packetno = packetno++;

          ogg_stream_packetin(&os,&header);
        }
#else
        if (have_audio) {
          AVPacket pkt = { 0 }; // data and size must be 0;
          av_packet_from_data(&pkt, &data[0], data_size);
          pkt.duration = 1;
          pkt.pts = fram_index;
          pkt.dts = 0;
          write_frame(oc, NULL, audio_st.st, &pkt);
        }
#endif
      }

      printf("Remaining: %" PRIu64 " bytes\n", remaining);
      fseek(fin, remaining, SEEK_CUR);
      fram_index++;
    }

#if 0
    printf("Outputting pages\n");
static int x = 0;
if (x > 100) {
  x = 0;
} else {
  x++;
  continue;
}
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
#endif

  }

#if 0
  // Cleanup
  ogg_stream_clear(&os);

  // Example code said "ogg_page and ogg_packet structs always point to storage
  // in libvorbis.  They're never freed or manipulated directly"
#endif

  fclose(fin);
#if 0
  fclose(fout);
#endif

  printf("Done\n");
  return 0;
}
