
/*
 * Copyright (c) 2006-2009 The StarBED Project  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/************************************************************************
 *
 * QOMET Emulator Implementation
 *
 * File name: global.h
 * Function:  Header file with global defines
 *
 * Author: Razvan Beuran
 *
 *   $Revision: 140 $
 *   $LastChangedDate: 2009-03-26 10:41:59 +0900 (Thu, 26 Mar 2009) $
 *   $LastChangedBy: razvan $
 *
 ***********************************************************************/


#ifndef __GLOBAL_H
#define __GLOBAL_H


/////////////////////////////////////////////
// Basic constants
/////////////////////////////////////////////

// BOOLEAN
#define TRUE                            1
#define FALSE                           0

#define SUCCESS                         0
#define ERROR                           -1

#define MAX_STRING                      256


/////////////////////////////////////////////
// Definitions of the main structures
// (used to avoid circular references)
/////////////////////////////////////////////

struct node_class_s;
struct object_class_s;
struct environment_class_s;
struct motion_class_s;
struct connection_class_s;
struct scenario_class_s;
struct scenario_class_s;

typedef struct node_class_s node_class;
typedef struct object_class_s object_class;
typedef struct environment_class_s environment_class;
typedef struct motion_class_s motion_class;
typedef struct connection_class_s connection_class;
typedef struct scenario_class_s scenario_class;
typedef struct xml_scenario_class_s xml_scenario_class;
typedef struct xml_jpgis_class_s xml_jpgis_class;

#endif
