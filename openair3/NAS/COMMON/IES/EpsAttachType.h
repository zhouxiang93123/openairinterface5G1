/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef EPS_ATTACH_TYPE_H_
#define EPS_ATTACH_TYPE_H_

#define EPS_ATTACH_TYPE_MINIMUM_LENGTH 1
#define EPS_ATTACH_TYPE_MAXIMUM_LENGTH 1

#define EPS_ATTACH_TYPE_EPS   0b0001
#define EPS_ATTACH_TYPE_IMSI    0b0010
#define EPS_ATTACH_TYPE_EMERGENCY 0b0110
#define EPS_ATTACH_TYPE_RESERVED  0b0111
typedef uint8_t EpsAttachType;

int encode_eps_attach_type(EpsAttachType *epsattachtype, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_eps_attach_type_xml(EpsAttachType *epsattachtype, uint8_t iei);

uint8_t encode_u8_eps_attach_type(EpsAttachType *epsattachtype);

int decode_eps_attach_type(EpsAttachType *epsattachtype, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_u8_eps_attach_type(EpsAttachType *epsattachtype, uint8_t iei, uint8_t value, uint32_t len);

#endif /* EPS ATTACH TYPE_H_ */

