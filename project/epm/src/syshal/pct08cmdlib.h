/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   pct08cmdlib.h
 * Author: Sam Chang
 *
 * Created on 2017年1月25日, 下午 4:55
 */

#ifndef PCT08CMDLIB_H
#define PCT08CMDLIB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nuc970.h"
#include "sys.h"

#include "fileagent.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL PCT08TakePhoto(uint8_t** photoPr, int* photoLen, StorageType type, char* dir, char* fileName);
BOOL PCT08ReadVerInfo(void);

#ifdef __cplusplus
}
#endif

#endif /* PCT08CMDLIB_H */

