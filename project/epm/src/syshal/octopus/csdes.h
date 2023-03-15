/*****************************************************************************/
/* File Name   : csdes.h		                                                 */
/* Author      : Copyright OCL                                        			 */
/* Company     : Octopus Cards Ltd.                                          */
/* Purpose     : Header for DES encryption routines													 */
/* Last Update : 17 Jul 2015                                                 */
/*****************************************************************************/
#ifndef __CSDES_H__
#define __CSDES_H__

#include "rwltype.h"

#define DES3CODE	30
#define DES_DECRYPT 0
#define DES_ENCRYPT 1
#define DES3_DECRYPT (DES3CODE+DES_DECRYPT)
#define DES3_ENCRYPT (DES3CODE+DES_ENCRYPT)

/*----------------------------------------------------------------------*
 * Encrypt/Decrypt using DES for a block of 8 bytes
 *----------------------------------------------------------------------*/
void CscryptEncrypt(BYTE *key,			/* 8-byte key */
                    BYTE *in,			/* block to be encrypted */
                    BYTE *out,			/* encrypted data */
                    INT encrypt);		/* 1=encrypt, 0=decrypt */

/*----------------------------------------------------------------------*
 * Encrypt/Decrypt using DES for a block of data
 * - make sure data block has space for multiple of 8-byte
 *----------------------------------------------------------------------*/
void CscryptEncryptBlock(BYTE *key,		/* the key */
						 BYTE *blk,		/* the data block */
						 INT length,	/* data length */
						 INT encrypt);	/* encrypt or decrypt */
#endif // __CSDES_H__

