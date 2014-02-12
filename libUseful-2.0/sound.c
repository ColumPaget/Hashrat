#include <stdint.h>
#include <sys/ioctl.h>
#include "sound.h"
#include "file.h"
#include "string.h"
#include "defines.h"
#include "includes.h"


typedef struct
{
uint32_t DataStart;
uint32_t DataSize;
uint32_t Format;
uint32_t SampleRate;
uint32_t Channels;
} AUHeader;



typedef struct
{
char RIFF[4];
uint32_t FileSize;
char Format[4];
char Block[4];
uint32_t ChunkSize;
uint16_t AudioFormat;
uint16_t Channels;
uint32_t SampleRate;
uint32_t ByteRate;
uint16_t BlockAlign;
uint16_t BitsPerSample;
char DataHeader[4];
uint32_t DataSize;
} WAVHeader;



char *VolTypeStrings[]={"master","pcm","cd","mic","line","video","phonein","phoneout","all",NULL};
typedef enum {VOL_MASTER,VOL_PCM,VOL_CD,VOL_MIC,VOL_LINE1,VOL_VIDEO,VOL_PHONEIN,VOL_PHONEOUT,VOL_ALL} TVOLUME_CHANNELS;



#define SAMPLE_SIGNED 1



int SoundReformat(char **Out, char *In, int Len, TAudioInfo *AI)
{
int len, i;

//Expand mono to both channels
if (AI->Channels==1)
{
	len=Len*2;
	*Out=SetStrLen(*Out, Len*2);
	for (i=0; i < Len; i++)
	{
		Out[i*2]=In[i];
		Out[i*2+1]=In[i];
	}
}


return(len);
}


TAudioInfo *AudioInfoCreate(unsigned int Format, unsigned int Channels, unsigned int SampleRate, unsigned int SampleSize, unsigned int DataSize)
{
TAudioInfo *AI;

AI=(TAudioInfo *) calloc(1,sizeof(TAudioInfo));

AI->Format=Format;
AI->Channels=Channels;
AI->SampleRate=SampleRate;
AI->SampleSize=SampleSize;
AI->DataSize=DataSize;

return(AI);
}


/*  ------------------------ SOUND FILE FORMATS --------------- */
TAudioInfo *SoundReadWAV(STREAM *S)
{
WAVHeader Wav;
TAudioInfo *AudioInfo;


AudioInfo=(TAudioInfo *) calloc(1,sizeof(TAudioInfo));

STREAMReadBytes(S,(char *) &Wav,sizeof(WAVHeader));

AudioInfo->Channels=Wav.Channels;
AudioInfo->SampleRate=Wav.SampleRate;

AudioInfo->DataSize=Wav.DataSize;

if (Wav.BitsPerSample==16)
{
	AudioInfo->SampleSize=2;
	AudioInfo->Format=AFMT_S16_LE;
}
else 
{
	AudioInfo->SampleSize=1;
	AudioInfo->Format=AFMT_U8;
}

return(AudioInfo);
}


void SoundWriteWAVHeader(STREAM *S, TAudioInfo *AI)
{
  WAVHeader WavHead;

  strcpy(WavHead.RIFF,"RIFF");
  strcpy(WavHead.Format,"WAVE");
  strcpy(WavHead.Block,"fmt ");
  WavHead.FileSize=AI->DataSize+44;
	WavHead.AudioFormat=1; 
  WavHead.Channels=AI->Channels;
  WavHead.SampleRate=AI->SampleRate;
  WavHead.BlockAlign=4;
  WavHead.ByteRate=WavHead.SampleRate * WavHead.BlockAlign;
  WavHead.BitsPerSample=AI->SampleSize * 8;
  WavHead.ChunkSize=WavHead.BitsPerSample;
  strcpy(WavHead.DataHeader,"data");
  WavHead.DataSize=AI->DataSize;


  STREAMWriteBytes(S,(char *) &WavHead,sizeof(WAVHeader));
}



TAudioInfo *SoundReadAU(STREAM *S)
{
AUHeader Header;
TAudioInfo *AudioInfo;

AudioInfo=(TAudioInfo *) calloc(1,sizeof(TAudioInfo));
STREAMReadBytes(S,(char *) &Header,sizeof(AUHeader));

//AU uses big endian
Header.DataSize=htonl(Header.DataSize);
Header.DataStart=htonl(Header.DataStart);
Header.Channels=htonl(Header.Channels);
Header.SampleRate=htonl(Header.SampleRate);
Header.Format=htonl(Header.Format);


AudioInfo->Channels=Header.Channels;
AudioInfo->SampleRate=Header.SampleRate;
AudioInfo->DataSize=Header.DataSize;


switch(Header.Format)
{
	case 1:
		AudioInfo->Format=AFMT_MU_LAW;
		break;

	case 2:
		AudioInfo->Format=AFMT_S8;
		break;

	case 3:
		AudioInfo->Format=AFMT_S16_BE;
		break;
}

STREAMSeek(S,(double) Header.DataStart,SEEK_SET);
return(AudioInfo);
}




#ifdef HAVE_OSS

#include <sys/soundcard.h>
/*  ------------------------ OSS Functions  -------------------- */
int OpenOSSDevice(char *Dev, int Type, TAudioInfo *Info)
{
	int fd, i;
	unsigned char *data;
	int val;

	if (Type==OUTPUT) val=O_WRONLY;
	else val=O_RDONLY;


	if (StrLen(Dev)==0) fd=open("/dev/dsp",val);
	else fd=open(Dev,val);

	if (fd==-1) return(-1);
	if (fd==-1) return(-1);


/* Tell the sound card that the sound about to be played is stereo. 0=mono 1=stereo */

	val=Info->Channels-1;
	if (val < 0) val=0;
	if ( ioctl(fd, SNDCTL_DSP_STEREO,&val) == -1 )
	{
		perror("ioctl stereo");
		return -1;
	}

	/* Inform the sound card of the audio format */
	if ( ioctl(fd, SNDCTL_DSP_SETFMT,&Info->Format) == -1 )
	{
		perror("ioctl format");
		return -1;
	}
	
	/* Set the DSP playback rate, sampling rate of the raw PCM audio */
	if (ioctl(fd, SNDCTL_DSP_SPEED,&Info->SampleRate) == -1 )
	{
		perror("ioctl sample rate");
		return -1;
	}
   
	/*
	if (Flags & O_RDWR)
	{
	if (ioctl(fd, SNDCTL_DSP_SETDUPLEX,0) == -1 )
	{
		perror("ioctl set duplex");
		return -1;
	}
   	}
	*/

	return(fd);
}

int OpenOSSInput(char *Dev, TAudioInfo *Info)
{
return(OpenOSSDevice(Dev,INPUT,Info));
}


int OpenOSSOutput(char *Dev, TAudioInfo *Info)
{
return(OpenOSSDevice(Dev,OUTPUT,Info));
}



int OSSAlterVolumeType(char *device, int Type, int delta)
{
int left_val, right_val, val, mixfd=0;

mixfd=open(device, O_RDWR);
if (mixfd > -1)
{
switch (Type)
{
	case VOL_MASTER:
	ioctl(mixfd,MIXER_READ(SOUND_MIXER_VOLUME),&val);
	break;

	case VOL_PCM:
	ioctl(mixfd,MIXER_READ(SOUND_MIXER_PCM),&val);
	break;

	case VOL_CD:
	ioctl(mixfd,MIXER_READ(SOUND_MIXER_CD),&val);
	break;

	case VOL_MIC:
	ioctl(mixfd,MIXER_READ(SOUND_MIXER_MIC),&val);
	break;

	case VOL_VIDEO:
	ioctl(mixfd,MIXER_READ(SOUND_MIXER_VIDEO),&val);
	break;

	case VOL_PHONEIN:
	ioctl(mixfd,MIXER_READ(SOUND_MIXER_PHONEIN),&val);
	break;

	case VOL_PHONEOUT:
	ioctl(mixfd,MIXER_READ(SOUND_MIXER_PHONEOUT),&val);
	break;

	case VOL_LINE1:
	ioctl(mixfd,MIXER_READ(SOUND_MIXER_LINE1),&val);
	break;
}

left_val=val & 0xFF;
right_val=(val & 0xFF00) >> 8;

left_val+=delta;
if (left_val < 0) left_val=0;
if (left_val > 255) left_val=255;

right_val+=delta;
if (right_val < 0) right_val=0;
if (right_val > 255) right_val=255;

right_val=right_val << 8;
val=right_val + left_val;

switch (Type)
{
	case VOL_MASTER:
	ioctl(mixfd,MIXER_WRITE(SOUND_MIXER_VOLUME),&val);
	break;

	case VOL_PCM:
	ioctl(mixfd,MIXER_WRITE(SOUND_MIXER_PCM),&val);
	break;

	case VOL_CD:
	ioctl(mixfd,MIXER_WRITE(SOUND_MIXER_CD),&val);
	break;

	case VOL_MIC:
	ioctl(mixfd,MIXER_WRITE(SOUND_MIXER_MIC),&val);
	break;

	case VOL_VIDEO:
	ioctl(mixfd,MIXER_WRITE(SOUND_MIXER_VIDEO),&val);
	break;

	case VOL_PHONEIN:
	ioctl(mixfd,MIXER_WRITE(SOUND_MIXER_PHONEIN),&val);
	break;

	case VOL_PHONEOUT:
	ioctl(mixfd,MIXER_WRITE(SOUND_MIXER_PHONEOUT),&val);
	break;

	case VOL_LINE1:
	ioctl(mixfd,MIXER_WRITE(SOUND_MIXER_LINE1),&val);
	break;
}

close(mixfd);
}

return(left_val);
}


int OSSAlterVolume(char *device, char *type_str, int delta)
{
int type;
type=MatchTokenFromList(type_str,VolTypeStrings,0);
if ((type==VOL_ALL) || (type==-1))
{
OSSAlterVolumeType(device, VOL_MASTER, delta);
OSSAlterVolumeType(device, VOL_PCM, delta);
}
else OSSAlterVolumeType(device, type, delta);
}


int OSSPlaySoundFile(char *DevPath, char *FilePath, int Vol)
{
STREAM *S;
char *Buffer=NULL, *Tempstr=NULL;
int result, fd=-1, mixfd=-1;
int Flags, val, oldvol;
char FourCharacter[5];
TAudioInfo *AudioInfo=NULL;

S=STREAMOpenFile(FilePath,O_RDONLY);
if (! S) return(FALSE);
STREAMReadBytes(S,FourCharacter,4);
FourCharacter[4]='\0';
STREAMSeek(S,0,SEEK_SET);
if (strcmp(FourCharacter,".snd")==0) AudioInfo=SoundReadAU(S);
else if (strcmp(FourCharacter,"RIFF")==0) AudioInfo=SoundReadWAV(S);

if (AudioInfo==NULL) 
{
	STREAMClose(S);
	return(FALSE);
}

//Must do all the above before we do open oss!
fd=OpenOSSOutput(DevPath, AudioInfo);
if (fd==-1)
{
	STREAMClose(S);
	return(FALSE);
}

if (Vol != VOLUME_LEAVEALONE)
{
	mixfd=open("/dev/dsp", O_RDWR);
	if (mixfd > -1)
	{
		ioctl(mixfd,SOUND_MIXER_READ_PCM,&oldvol);
		if (Vol > 255) result=255;
		else result=Vol;
		val=(result) | (result <<8);
		ioctl(mixfd,SOUND_MIXER_WRITE_PCM,&val);
	}
}

Buffer=SetStrLen(Buffer,1024);
result=STREAMReadBytes(S,Buffer,1024);

/*
if (AudioInfo->Channels==1)
{
	Tempstr=SetStrLen(Tempstr,result);
	memcpy(Tempstr,Buffer,result);
	result=SoundReformat(&Buffer, Tempstr, result, AudioInfo);
}
*/

val=0;
while (result > 0)  
{
write(fd,Buffer,result);
val+=result;
if ((AudioInfo->DataSize > 0) && (val >= AudioInfo->DataSize)) break;
result=STREAMReadBytes(S,Buffer,1024);
}

close(fd);
if (mixfd > -1) 
{
ioctl(mixfd,SOUND_MIXER_WRITE_PCM,&oldvol);
close(mixfd);
}

DestroyString(Buffer);
DestroyString(Tempstr);

STREAMClose(S);
return(TRUE);
}

#endif


/*  ------------------------ ESound Functions  -------------------- */

#ifdef HAVE_LIBESD
#include <esd.h>

int ESDGetConnection()
{
static int ConFD=-1;

if (ConFD==-1) ConFD=esd_open_sound(NULL);
return(ConFD);
}


void ESDSendFileData(int ConFD, char *SoundFilePath, int Vol)
{
char *Tempstr=NULL;
int id;

Tempstr=CopyStr(Tempstr,"ColLib:");
Tempstr=CatStr(Tempstr,SoundFilePath);

id=esd_sample_getid(ConFD,Tempstr);
if (id < 0)
{
   id=esd_file_cache(ConFD,"ColLib",SoundFilePath);
}
if (Vol != VOLUME_LEAVEALONE) esd_set_default_sample_pan(ConFD,id,Vol,Vol);
esd_sample_play(ConFD,id);

DestroyString(Tempstr);
}

#endif



int ESDPlaySoundFile(char *SoundFilePath, int Vol)
{
int result, fd;

if (StrLen(SoundFilePath) < 1) return(FALSE);
#ifdef HAVE_LIBESD

fd=ESDGetConnection();
if (fd > -1)
{
ESDSendFileData(fd,SoundFilePath,Vol);
return(TRUE);
}

#endif

return(FALSE);
}

/*
//Dont understand ESD volumes yet.

int ESDAlterVolume(char *Device, int Vol)
{
int result, fd;

if (StrLen(Device) < 1) return;
#ifdef HAVE_LIBESD

fd=ESDGetConnection();
if (fd > -1)
{

return(TRUE);
}

#endif

return(FALSE);
}

*/


//---------------- Wrapper functions -------------//


int SoundOpenOutput(char *Dev, TAudioInfo *Info)
{
int fd=-1;

#ifdef HAVE_LIBESD
esd_format_t esd_format;

if ((StrLen(Dev)==0) || (strncmp(Dev,"esd:",4)==0))
{
esd_format=ESD_STREAM | ESD_PLAY;

if (Info->Channels==2) esd_format |= ESD_STEREO;
else esd_format |= ESD_MONO;

if (Info->SampleSize==2) esd_format |= ESD_BITS16;
else esd_format |= ESD_BITS8;

if (StrLen(Dev) > 4) setenv("ESPEAKER",Dev+4,TRUE);
else setenv("ESPEAKER","localhost",TRUE);
fd=esd_play_stream(esd_format, Info->SampleRate, NULL, "testing");
}

#endif

#ifdef HAVE_OSS
if (fd < 0) fd=OpenOSSOutput(Dev, Info);
#endif

return(fd);
}


int SoundOpenInput(char *Dev, TAudioInfo *Info)
{
int fd=-1;

#ifdef HAVE_LIBESD
esd_format_t esd_format;

if ((StrLen(Dev)==0) || (strncmp(Dev,"esd:",4)==0))
{
esd_format=ESD_STREAM | ESD_RECORD;

if (Info->Channels==2) esd_format |= ESD_STEREO;
else esd_format |= ESD_MONO;

if (Info->SampleSize==2) esd_format |= ESD_BITS16;
else esd_format |= ESD_BITS8;

if (StrLen(Dev) > 4) setenv("ESPEAKER",Dev+4,TRUE);
else setenv("ESPEAKER","localhost",TRUE);
fd=esd_record_stream(esd_format, Info->SampleRate, NULL, "testing");
}

#endif

#ifdef HAVE_OSS
if (fd < 0) fd=OpenOSSInput(Dev, Info);
#endif

return(fd);
}



int SoundPlayFile(char *Output, char *Path, int Vol, int Flags)
{
int result=0, pid=0;


if (Flags & PLAYSOUND_NONBLOCK) 
{
	pid=fork();
	if (pid==0) CloseOpenFiles();
}
if (pid==0)
{
result=ESDPlaySoundFile(Path, Vol);

#ifdef HAVE_OSS
if (! result) result=OSSPlaySoundFile(Output, Path, Vol);
#endif

if (Flags & PLAYSOUND_NONBLOCK) _exit(0);
}

return(result);
}

int SoundAlterVolume(char *ReqDev, char *VolType, int delta)
{
char *device=NULL;
int val;

if (StrLen(ReqDev)) device=CopyStr(device,ReqDev);
else device=CopyStr(device,"/dev/mixer");

#ifdef HAVE_OSS
val=OSSAlterVolume(device, VolType, delta);
#endif

DestroyString(device);
return(val);

}



