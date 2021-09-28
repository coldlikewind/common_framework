#pragma once

#include "mod_sig.h"
#include "NgapMessage.h"
#include "EsmContext.h"

struct NgapMessage;


bool Epm_Init();


bool GNB_InitialContextSetupSuccessRspByMedia(NgapMessage& info);

bool GNB_PDUSessionSetupRspByMedia(NgapMessage& info);

bool GNB_PathSwitchRequestByMedia(NgapMessage& info);

bool GNB_HandoverReqAckMedia(NgapMessage &info);
