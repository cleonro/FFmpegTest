#include <libavcodec/avcodec.h>
#include <libavcodec/aac.h>
#include <libavcodec/sbr.h>

#include <stdio.h>

int hasSBR1(AVCodecContext *codecContext)
{
    int res = -1;

    static int count = 0;

    if(!codecContext || !codecContext->priv_data)
    {
        return res;
    }

    AACContext *aacContext = codecContext->priv_data;
    if(aacContext->che[0][0])
    {
        res = aacContext->che[0][0]->sbr.bs_amp_res_header != 0 ? 1 : -1;

        printf("%d. che[0][0]->sbr.bs_amp_res_header = %d, che[0][0]->sbr.reset = %d, oc[0].m4ac.sbr = %d, oc[0].m4ac.object_type = %d \n",
               ++count,
               aacContext->che[0][0]->sbr.bs_amp_res_header,
               aacContext->che[0][0]->sbr.reset,
               aacContext->oc[0].m4ac.sbr,
               aacContext->oc[0].m4ac.object_type);
    }


    return res;
}

int hasSBR2(AVCodecContext *codecContext)
{
    int res = -1;

    if(!codecContext || !codecContext->priv_data)
    {
        return res;
    }

    AACContext *aacContext = codecContext->priv_data;

    res = aacContext->oc[0].m4ac.sbr;


    return res;
}

static int sbr_callback_set = 0;

static int sbr_callback_result = -1;

static void sbr_callback(int step)
{
    sbr_callback_result = step;
    printf("->%d", step);
}

int hasSBR3(AVCodecContext *codecContext)
{
    int res = -1;

    if(!codecContext || !codecContext->priv_data)
    {
        return res;
    }

    AACContext *aacContext = codecContext->priv_data;
    if(sbr_callback_set == 0)
    {
        sbr_callback_set = 1;
        aacContext->custom_sbr_callback = sbr_callback;
        return 0;
    }

    res = sbr_callback_result;
    sbr_callback_result = -1;

    //printf("\n");

    return res;
}
