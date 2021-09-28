#pragma once


#include "mod_sig.h"

#define PLMN_ID_LEN 3
#define PLMN_MNC_ID_LEN 2



#ifdef __cplusplus
extern "C" {
#endif

typedef struct _plmn_id_t {
#if BYTE_ORDER == BIG_ENDIAN
	uint8_t mcc2:4;
    uint8_t mcc1:4;
	uint8_t mnc1:4;
    uint8_t mcc3:4;
	uint8_t mnc3:4;
    uint8_t mnc2:4;
#else
    uint8_t mcc1:4;
	uint8_t mcc2:4;
    uint8_t mcc3:4;
	uint8_t mnc1:4;
    uint8_t mnc2:4;
	uint8_t mnc3:4;
#endif
} __attribute__ ((packed)) plmn_id_t;


	
uint16_t plmn_id_mcc(plmn_id_t *plmn_id);
uint16_t plmn_id_mnc(plmn_id_t *plmn_id);
uint16_t plmn_id_mnc_len(plmn_id_t *plmn_id);

void plmn_id_build(plmn_id_t *plmn_id, uint16_t mcc, uint16_t mnc, uint16_t mnc_len);


#ifdef __cplusplus
}
#endif

