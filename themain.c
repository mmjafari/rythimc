#include <stdio.h>
#include<stdint.h>
#include "bcint.h"
#include<string.h>

int main( int argc, char *argv[]){
  FILE *input = fopen(argv[1], "rb");
  FILE *output = fopen(argv[2], "wb+");
  if (input == NULL) {return 0;}
  MidiFile midi_file;
  midi_file.format = 1;
  midi_file.track_count = 1; // default
  midi_file.division = 480; // default
  int vcount = 0;
  int bpm = 220; //dummy
  //voice **vlist = (voice **)malloc(64 * sizeof(voice));
  voice *vlist = (voice *)malloc(256 * sizeof(voice));
  __uint128_t lnote = 0;
  unsigned char li[2]; 
  unsigned char le;
  int i;
  char tname[256];
  while (fread(&le, 1, 1, input)){
    switch(le){
    case 0x54:
      i = 0;
      while(fread(&le, 1, 1, input) && le != 0xFF){
        tname[i] = le; i++;
      }
      tname[i] = '\0';
      break;
    case 0x42:
      fread(&le, 1, 1, input);
      bpm = le;
      fread(&le, 1, 1, input);
      midi_file.division = le;
      break;
    case 0x58:
      fread(&le, 1, 1, input);
      midi_file.track_count = le;
      break;
    case 0x4e:
      for (int i = 0; i < 8; i++){
        fread(&li, 1, 2, input);
        lnote = (lnote << 16) + (li[0] << 8) + li[1];
      }
      printf("%d\n", vcount);
      if (&vlist[vcount]){note_maker(lnote, &vlist[vcount]);}
      printf("%d\n", vlist[vcount].vend);
      vcount++;
      break;
    case 0xFF:
      printf("\n");
      break;    default:
      printf("%02x\n", le);
    }
  }
  printf("%s", tname);
}
