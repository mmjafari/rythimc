#include   <stdio.h>
#include  <stdint.h>
#include  <string.h>
#include  <stdlib.h>
#include<inttypes.h>

typedef struct {
  int vnote; // note
  int vstrt; // first beat
  int vend;  // last beat
  int vspce; // spacing between
  int voffs; // offset of spacing
  int vreps; // repeats
  int vrpsp; // space between repeats
  int vblen; // beat length
  int vprog; // midi program
} voice;

typedef struct{
  int   bpm;
  char  title[256];
  int   tnum;
  int   vcount;
  voice *vlist;
} track;

typedef struct {
    uint8_t status;
    uint8_t data1;
    uint8_t data2;
    uint32_t delta_time;
} MidiEvent;

typedef struct {
    uint8_t type;
    uint8_t* data;
    uint32_t length;
    uint32_t delta_time;
} MetaEvent;

typedef struct {
    MidiEvent* midi_events;
    MetaEvent* meta_events;
    uint32_t midi_event_count;
    uint32_t meta_event_count;
} MidiTrack;

typedef struct {
    uint16_t format;
    uint16_t track_count;
    uint16_t division;
    MidiTrack* tracks;
} MidiFile;

void create_midi_file(MidiFile* midi_file, const char* filename);
void add_midi_event(MidiTrack* track, uint8_t status, uint8_t data1, uint8_t data2, uint32_t delta_time);
void add_meta_event(MidiTrack* track, uint8_t type, const uint8_t* data, uint32_t length, uint32_t delta_time);
void free_midi_file(MidiFile* midi_file);

void write_var_length(uint32_t value, FILE* file) {
    uint8_t buffer[4];
    uint8_t* p = &buffer[3];
    *p = value & 0x7F;
    while (value >>= 7) {
        *--p = (value & 0x7F) | 0x80;
    }
    fwrite(p, 1, &buffer[4] - p, file);
}

void write_uint16_be(uint16_t value, FILE* file) {
    uint8_t bytes[2];
    bytes[0] = (value >> 8) & 0xFF;
    bytes[1] = value & 0xFF;
    fwrite(bytes, 1, 2, file);
}

void write_uint32_be(uint32_t value, FILE* file) {
    uint8_t bytes[4];
    bytes[0] = (value >> 24) & 0xFF;
    bytes[1] = (value >> 16) & 0xFF;
    bytes[2] = (value >> 8) & 0xFF;
    bytes[3] = value & 0xFF;
    fwrite(bytes, 1, 4, file);
}

void create_midi_file(MidiFile* midi_file, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Failed to open file");
        return;
    }

    fwrite("MThd", 1, 4, file);
    write_uint32_be(6, file); // Header length
    write_uint16_be(midi_file->format, file);
    write_uint16_be(midi_file->track_count, file);
    write_uint16_be(midi_file->division, file);

    for (int i = 0; i < midi_file->track_count; i++) {
        MidiTrack* track = &midi_file->tracks[i];
        fwrite("MTrk", 1, 4, file);

        uint32_t track_length_position = ftell(file);
        write_uint32_be(0, file); // Placeholder for track length

        uint32_t track_length = 0;
        for (int j = 0; j < track->midi_event_count; j++) {
            write_var_length(track->midi_events[j].delta_time, file);
            fwrite(&track->midi_events[j].status, 1, 1, file);
            fwrite(&track->midi_events[j].data1, 1, 1, file);
            fwrite(&track->midi_events[j].data2, 1, 1, file);
        }

        for (int j = 0; j < track->meta_event_count; j++) {
            write_var_length(track->meta_events[j].delta_time, file);
            fwrite(&track->meta_events[j].type, 1, 1, file);
            write_var_length(track->meta_events[j].length, file);
            fwrite(track->meta_events[j].data, 1, track->meta_events[j].length, file);
        }

        uint32_t track_end_position = ftell(file);
        fseek(file, track_length_position, SEEK_SET);
        write_uint32_be(track_end_position - track_length_position - 4, file);
        fseek(file, track_end_position, SEEK_SET);
    }

    fclose(file);
}

void free_midi_file(MidiFile* midi_file) {
    for (int i = 0; i < midi_file->track_count; i++) {
        MidiTrack* track = &midi_file->tracks[i];
        for (int j = 0; j < track->meta_event_count; j++) {
            free(track->meta_events[j].data);
        }
        free(track->meta_events);
        free(track->midi_events);
    }
    free(midi_file->tracks);
}

void add_midi_event(MidiTrack* track, uint8_t status, uint8_t data1, uint8_t data2, uint32_t delta_time) {
    track->midi_event_count++;
    track->midi_events = realloc(track->midi_events, sizeof(MidiEvent) * track->midi_event_count);
    MidiEvent* event = &track->midi_events[track->midi_event_count - 1];
    event->status = status;
    event->data1 = data1;
    event->data2 = data2;
    event->delta_time = delta_time;
}

void add_meta_event(MidiTrack* track, uint8_t type, const uint8_t* data, uint32_t length, uint32_t delta_time) {
    track->meta_event_count++;
    track->meta_events = realloc(track->meta_events, sizeof(MetaEvent) * track->meta_event_count);
    MetaEvent* event = &track->meta_events[track->meta_event_count - 1];
    event->type = type;
    event->length = length;
    event->delta_time = delta_time;
    event->data = malloc(length);
    memcpy(event->data, data, length);
}

void note_maker(__uint128_t vcon, voice *v){
  v->vnote = (vcon & (__uint128_t)0xFF000000000000000000000000000000) >> 120;
  v->vstrt = (vcon & (__uint128_t)0x00FFFF00000000000000000000000000) >> 104;
  v->vend  = (vcon & (__uint128_t)0x000000FFFF0000000000000000000000) >> 88;
  v->vspce = (vcon & (__uint128_t)0x0000000000FFFF000000000000000000) >> 72;
  v->voffs = (vcon & (__uint128_t)0x00000000000000FFFF00000000000000) >> 56;
  v->vreps = (vcon & (__uint128_t)0x000000000000000000FFFF0000000000) >> 40;
  v->vrpsp = (vcon & (__uint128_t)0x0000000000000000000000FFFF000000) >> 24;
  v->vblen = (vcon & (__uint128_t)0x00000000000000000000000000F00000) >> 20;
  v->vprog = (vcon & (__uint128_t)0x000000000000000000000000000FF000) >> 12;
  printf("note:%d\nstart:%d\nend:%d\nspace:%d\noffset:%d\nplace holder 2:%d\n",\
         v->vnote,\
         v->vstrt,\
         v->vend,\
         v->vspce,\
         v->voffs,\
         v->vprog);
}
