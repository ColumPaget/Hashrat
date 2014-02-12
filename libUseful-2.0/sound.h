#ifndef LIBUSEFUL_SOUND_H
#define LIBUSEFUL_SOUND_H

#include "file.h"

typedef struct
{
unsigned int Format;
unsigned int Channels;
unsigned int SampleRate;
unsigned int SampleSize;
unsigned int DataSize;
}TAudioInfo;

#define VOLUME_LEAVEALONE -1
#define PLAYSOUND_NONBLOCK 1


#define OUTPUT 0
#define INPUT 1


/* For systems that lack 'soundcard.h' but still have some kind of sound */
/* We define enough audio formats for us to use internally */
#ifndef AFMT_MU_LAW 
# define AFMT_MU_LAW    0x00000001
#endif

#ifndef AFMT_A_LAW 
# define AFMT_A_LAW   0x00000002
#endif

# define AFMT_IMA_ADPCM   0x00000004

#ifndef AFMT_U8
# define AFMT_U8      0x00000008
#endif

#ifndef AFMT_S8
# define AFMT_S8      0x00000040
#endif

#ifndef AFMT_S16_LE
# define AFMT_S16_LE    0x00000010  /* Little endian signed 16*/
#endif

#ifndef AFMT_S16_BE
# define AFMT_S16_BE    0x00000010  /* Little endian signed 16*/
#endif


#ifndef AFMT_U16_BE
# define AFMT_U16_LE    0x00000080  /* Little endian U16 */
#endif

#ifndef AFMT_U16_BE
# define AFMT_U16_BE    0x00000100  /* Big endian U16 */
#endif


#ifdef __cplusplus
extern "C" {
#endif


TAudioInfo *AudioInfoCreate(unsigned int Format, unsigned int Channels, unsigned int SampleRate, unsigned int SampleSize, unsigned int DataSize);
int SoundPlayFile(char *Device, char *Path, int Vol, int Flags);
int SoundAlterVolume(char *Device, char *Channel, int delta);
int SoundOpenOutput(char *Dev, TAudioInfo *Info);
int SoundOpenInput(char *Dev, TAudioInfo *Info);
TAudioInfo *SoundReadWAV(STREAM *S);
TAudioInfo *SoundReadAU(STREAM *S);
void SoundWriteWAVHeader(STREAM *S, TAudioInfo *AI);


#ifdef __cplusplus
}
#endif


#endif
