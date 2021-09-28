#pragma once

#include "mod_sig.h"
#include "NgapMessage.h"

#define SIGGW_GNB_ID_LEN 24
#define SIGGW_NR_CELL_ID_LEN 36


struct NgapMessage;


bool APM_Init();

bool AMF_InitialContextSetupReqByMedia(NgapMessage& info);

bool AMF_PDUSessionResourceSetupReqByMedia(NgapMessage& info);

bool AMF_HandoverReqByMedia(NgapMessage& info);

