
#include "PlmnUtil.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PLMN_ID_DIGIT1(x) (((x) / 10 / 10) % 10)
#define PLMN_ID_DIGIT2(x) (((x) / 10) % 10)
#define PLMN_ID_DIGIT3(x) ((x) % 10)

uint16_t plmn_id_mcc(plmn_id_t *plmn_id)
{
    return plmn_id->mcc1 * 10 * 10 + plmn_id->mcc2 * 10 + plmn_id->mcc3;
}

uint16_t plmn_id_mnc(plmn_id_t *plmn_id)
{
    return plmn_id->mnc1 == 0xf ? plmn_id->mnc2 * 10 + plmn_id->mnc3 :
        plmn_id->mnc1 * 10 * 10 + plmn_id->mnc2 * 10 + plmn_id->mnc3;
}

uint16_t plmn_id_mnc_len(plmn_id_t *plmn_id)
{
    return plmn_id->mnc1 == 0xf ? 2 : 3;
}

void plmn_id_build(plmn_id_t *plmn_id, 
        uint16_t mcc, uint16_t mnc, uint16_t mnc_len)
{
	  plmn_id->mcc1 = PLMN_ID_DIGIT1(mcc);
      plmn_id->mcc2 = PLMN_ID_DIGIT2(mcc);
      plmn_id->mcc3 = PLMN_ID_DIGIT3(mcc);


    if (mnc_len == 2)
        plmn_id->mnc1 = 0xf;
    else
        plmn_id->mnc1 = PLMN_ID_DIGIT1(mnc);

	plmn_id->mnc2 = PLMN_ID_DIGIT2(mnc);
	plmn_id->mnc3 = PLMN_ID_DIGIT3(mnc);

}


#ifdef __cplusplus
}
#endif

