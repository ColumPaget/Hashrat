/*
* SPDX-License-Identifier: CC-PDDC
*/

/*
Copyright (c) 2011 Wu Hongjun <wuhj@ntu.edu.sg>

Whilst there seems to be no official license for JH, it seems to be public domain.
From: https://www3.ntu.edu.sg/home/wuhj/research/jh/index.html "JH is not covered by any patent and JH is freely-available."
Professor Wu was also kind enough to confirm that JH can be used in GPL programs in the below email

Delivered-To: colum.paget@gmail.com
Received: by 10.140.49.6 with SMTP id p6csp11949qga;
        Sat, 29 Mar 2014 11:16:09 -0700 (PDT)
X-Received: by 10.69.21.106 with SMTP id hj10mr15325620pbd.87.1396116968622;
        Sat, 29 Mar 2014 11:16:08 -0700 (PDT)
Return-Path: <wuhj@ntu.edu.sg>
Received: from SMTP3.ntu.edu.sg (smtp3.ntu.edu.sg. [155.69.5.81])
        by mx.google.com with ESMTP id xi5si1057670pab.37.2014.03.29.11.16.07
        for <colum.paget@gmail.com>;
        Sat, 29 Mar 2014 11:16:08 -0700 (PDT)
Received-SPF: pass (google.com: domain of wuhj@ntu.edu.sg designates 155.69.5.81 as permitted sender) client-ip=155.69.5.81;
Authentication-Results: mx.google.com;
       spf=pass (google.com: domain of wuhj@ntu.edu.sg designates 155.69.5.81 as permitted sender) smtp.mail=wuhj@ntu.edu.sg
X-AuditID: 9b450551-b7c1aae000004f70-3b-53370de52165
Received: from EXSMTP6.staff.main.ntu.edu.sg (exsmtp6.ntu.edu.sg [155.69.5.101]) by SMTP3.ntu.edu.sg (Symantec Messaging Gateway) with SMTP id 74.8B.20336.5ED07335; Sun, 30 Mar 2014 02:16:06 +0800 (MYT)
Received: from EXCHHUB14.staff.main.ntu.edu.sg (155.69.25.17) by EXSMTP6.staff.main.ntu.edu.sg (155.69.5.101) with Microsoft SMTP Server (TLS) id 14.3.123.3; Sun, 30 Mar 2014 02:15:11 +0800
Received: from EXCHMBOX31.staff.main.ntu.edu.sg ([169.254.1.91]) by EXCHHUB14.staff.main.ntu.edu.sg ([155.69.25.17]) with mapi id 14.03.0123.003; Sun, 30 Mar 2014 02:14:53 +0800
From: WU Hongjun <wuhj@ntu.edu.sg>
To: Colum Paget <colum.paget@googlemail.com>
Subject: RE: What's the legal status of using the JH reference implementation code in programs?
Thread-Topic: What's the legal status of using the JH reference implementation code in programs?
Thread-Index: AQHPSb6Ogb7OadQwR0GgfWXQpHtu/pr4YMbz
Date: Sat, 29 Mar 2014 18:14:53 +0000
Message-ID: <BDC26E8E4C1DAB4A86AA2B004FCF693D01B8A5@EXCHMBOX31.staff.main.ntu.edu.sg>
References: <CABsuZXUfs=U5_-3FSgMv2dC1dS_P3UFVhpM77D8JKhYEWXj+vA@mail.gmail.com>
In-Reply-To: <CABsuZXUfs=U5_-3FSgMv2dC1dS_P3UFVhpM77D8JKhYEWXj+vA@mail.gmail.com>
Accept-Language: en-US
Content-Language: en-US
X-MS-Has-Attach: 
X-MS-TNEF-Correlator: 
x-originating-ip: [119.74.104.56]
Content-Type: text/plain; charset="us-ascii"
Content-Transfer-Encoding: quoted-printable
MIME-Version: 1.0
Return-Path: wuhj@ntu.edu.sg
X-Brightmail-Tracker: H4sIAAAAAAAAA+NgFtrKKsWRmVeSWpSXmKPExsUy25U1VfcZr3mwwYJz7BYH9u1mcmD0eDph MnsAYxSXTUpqTmZZapG+XQJXxomjt9kKdvNUnL/3m62BsZWri5GTQ0LAROL0259sELaYxIV7 64FsLg4hgTOMEtcu9TJBOAcZJTqWb4ZytjJKbPrbxATSwiagJLGv7S8jiC0ioCPxeNNBli5G Dg5hgUSJviOSEOEkibeLzrJC2EYStxtPgNksAqoSD1v2gY3hFQiVeLD2BzuILSQQIHFv6h0W EJtTIFDiz8RLYDWMQNd9P7UGzGYWEJe49WQ+E8TVAhJL9pxnhrBFJV4+/scKYStK7L7dwg5R ryOxYPcnNghbW2LZwtfMEHsFJU7OfMICsVdG4kLvV5YJjOKzkKyYhaR9FpL2WUjaFzCyrGIU CPYNCTDWyysp1UtNKdUrTt/ECImdwB2MV//oHWIU4GBU4uFV5DYPFmJNLCuuzD3EKMnBpCTK KwQS4kvKT6nMSCzOiC8qzUktPsQowcGsJMK7455ZsBBvSmJlVWpRPkxKmoNFSZw3W8QwWEgg PbEkNTs1tSC1CCYrw8GhJMHLDUwRQoJFqempFWmZOSUIaSYOTpDhPEDDeUFqeIsLEnOLM9Mh 8qcYFaXEee/xACUEQBIZpXlwvaDUVv////9XjOJArwjzPgGp4gGmRbjuV0CDmYAGuxWBXF1c koiQkmpgLDD+qKlR4aZb1un2hUdb8XinXuYEsekxIQoXz2pOOceouGfBjPvFs55yPPnUETT5 2eNlGquveYn9NCu6/YX9WvjPwufm7Dqxts25SzayOkyXPcvG//Gpdvu1pxWTmHiP39coDRWZ 4vcsu+Xg50dv5QP3tWwVS32y6+7UGzyGhkfyv4TNTvgfpcRSnJFoqMVcVJwIAEATx1dIAwAA

Dear Colum,

Thank you for your interest in JH.

I can confirm that:  the JH reference implementation can be used in the pro=
grams that are released under the GNU Public License.

(Developing the algorithm takes much more effort than writing the code.  So=
 the code of JH is freely available.)

Best Regards,
hongjun


*/


/* This program gives the reference implementation of JH.
   It implements the standard description of JH (not bitslice)
   The description given in this program is suitable for hardware implementation

   --------------------------------
   Comparing to the original reference implementation,
   two functions are added to make the porgram more readable.
   One function is E8_initialgroup() at the beginning of E8;
   another function is E8_finaldegroup() at the end of E8.

   --------------------------------

   Last Modified: January 16, 2011
*/


#ifndef JH_HASH_H
#define JH_HASH_H


typedef unsigned long long DataLength;
typedef enum { SUCCESS = 0, FAIL = 1, BAD_HASHLEN = 2 } HashReturn;

typedef struct
{
    int hashbitlen;                     /*the message digest size*/
    unsigned long long databitlen;      /*the message size in bits*/
    unsigned long long datasize_in_buffer;  /*the size of the message remained in buffer; assumed to be multiple of 8bits except for the last partial block at the end of the message*/
    unsigned char H[128];               /*the hash value H; 128 bytes;*/
    unsigned char A[256];               /*the temporary round value; 256 4-bit elements*/
    unsigned char roundconstant[64];    /*round constant for one round; 64 4-bit elements*/
    unsigned char buffer[64];           /*the message block to be hashed; 64 bytes*/
} hashState;

/*The API functions*/
HashReturn JHInit(hashState *state, int hashbitlen);
HashReturn JHUpdate(hashState *state, const unsigned char *data, DataLength databitlen);
unsigned int JHFinal(hashState *state, unsigned char *hashval);


#endif
