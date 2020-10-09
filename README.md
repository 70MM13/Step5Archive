# Step5Archive
STEP 5 Project Archive file format demystified, with example C/C++ code written with [U++ framework](https://www.ultimatepp.org/index.html).

### Introduction

The archive is a file which is produced by the Siemens STEP 5 (v7.x?) software package. This is software for programming Simatic S5 controllers (PLC). They can also be handled with Siemens' [S5DEARCH.EXE](https://support.industry.siemens.com/cs/document/1160072/archiving-dearchivingtool-for-step-5-v7-x-projekt-files?dti=0&lc=en-CY).

The file extension is .ACS - normally the filename is "\<S5PROJ\>PX.ACS", where \<S5PROJ\> is the cryptic STEP 5 project name with only 6 characters in length.

A standard archive has the full pathname stored and STEP 5 extracts to these full pathnames when dearchiving. (With the s5dearch tool you can specify another destination). The files are either stored or compressed with the "Sixpack" compression algorithm. This seems to be a bit obscure compression scheme, known as "SIXPACK.C" by Philip G. Gage. [Original files](resources/sixpack) in this repo are extracted from [DDJCOMPR.ZIP](https://www.sac.sk/download/pack/ddjcompr.zip).

### Structure of archive

| description | comment                                     |
| ----------- | ------------------------------------------- |
| Header      | With number of files in archive             |
| Directory   | File records with offset+size in data block |
| Data block  | Blob of data of all files                   |

### Structure of header

| description       | size     | comment   |
| ----------------- | -------- | --------- |
| identification    | 5 bytes  | ("STEP5") |
| *null*            | 2 bytes  |           |
| Number of files   | 2 bytes  |           |
| Archive file size | 4 bytes  |           |
| reserved          | 41 bytes |           |
|                   | 54 bytes |           |

### Structure of file record

| description                | size      | comment                  |
| -------------------------- | --------- | ------------------------ |
| File name                  | 260 bytes |                          |
| Date (DOS)                 | 2 bytes   |                          |
| Time (DOS)                 | 2 bytes   |                          |
| Attribute (DOS)            | 2 bytes   |                          |
| Compression                | 1 byte    | (0=stored; 1=compressed) |
| Size uncompressed          | 4 bytes   |                          |
| Size compressed            | 4 bytes   |                          |
| reserved                   | 8 bytes   |                          |
| Start offset in data block | 4 bytes   |                          |
|                            | 287 bytes |                          |

