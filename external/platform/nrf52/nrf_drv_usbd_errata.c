/**
 * Copyright (c) 2016 - 2017, Nordic Semiconductor ASA
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 * 
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

/* This was copied from the Nordic SDK usb interrupt handler.  Since we didn't want to
    re-use the handler as-is, but want to use this code verbatim, it was copied from the
    handler to here and will be used as a function. - Eric Holland
*/

#include <platform/nrf52840.h>
#include <platform/nrf52840_bitfields.h>

/**
 * @brief USBD interrupts
 */
typedef enum
{
    NRF_USBD_INT_USBRESET_MASK    = USBD_INTEN_USBRESET_Msk   , /**< Enable or disable interrupt for USBRESET event */
    NRF_USBD_INT_STARTED_MASK     = USBD_INTEN_STARTED_Msk    , /**< Enable or disable interrupt for STARTED event */
    NRF_USBD_INT_ENDEPIN0_MASK    = USBD_INTEN_ENDEPIN0_Msk   , /**< Enable or disable interrupt for ENDEPIN[0] event */
    NRF_USBD_INT_ENDEPIN1_MASK    = USBD_INTEN_ENDEPIN1_Msk   , /**< Enable or disable interrupt for ENDEPIN[1] event */
    NRF_USBD_INT_ENDEPIN2_MASK    = USBD_INTEN_ENDEPIN2_Msk   , /**< Enable or disable interrupt for ENDEPIN[2] event */
    NRF_USBD_INT_ENDEPIN3_MASK    = USBD_INTEN_ENDEPIN3_Msk   , /**< Enable or disable interrupt for ENDEPIN[3] event */
    NRF_USBD_INT_ENDEPIN4_MASK    = USBD_INTEN_ENDEPIN4_Msk   , /**< Enable or disable interrupt for ENDEPIN[4] event */
    NRF_USBD_INT_ENDEPIN5_MASK    = USBD_INTEN_ENDEPIN5_Msk   , /**< Enable or disable interrupt for ENDEPIN[5] event */
    NRF_USBD_INT_ENDEPIN6_MASK    = USBD_INTEN_ENDEPIN6_Msk   , /**< Enable or disable interrupt for ENDEPIN[6] event */
    NRF_USBD_INT_ENDEPIN7_MASK    = USBD_INTEN_ENDEPIN7_Msk   , /**< Enable or disable interrupt for ENDEPIN[7] event */
    NRF_USBD_INT_EP0DATADONE_MASK = USBD_INTEN_EP0DATADONE_Msk, /**< Enable or disable interrupt for EP0DATADONE event */
    NRF_USBD_INT_ENDISOIN0_MASK   = USBD_INTEN_ENDISOIN_Msk   , /**< Enable or disable interrupt for ENDISOIN[0] event */
    NRF_USBD_INT_ENDEPOUT0_MASK   = USBD_INTEN_ENDEPOUT0_Msk  , /**< Enable or disable interrupt for ENDEPOUT[0] event */
    NRF_USBD_INT_ENDEPOUT1_MASK   = USBD_INTEN_ENDEPOUT1_Msk  , /**< Enable or disable interrupt for ENDEPOUT[1] event */
    NRF_USBD_INT_ENDEPOUT2_MASK   = USBD_INTEN_ENDEPOUT2_Msk  , /**< Enable or disable interrupt for ENDEPOUT[2] event */
    NRF_USBD_INT_ENDEPOUT3_MASK   = USBD_INTEN_ENDEPOUT3_Msk  , /**< Enable or disable interrupt for ENDEPOUT[3] event */
    NRF_USBD_INT_ENDEPOUT4_MASK   = USBD_INTEN_ENDEPOUT4_Msk  , /**< Enable or disable interrupt for ENDEPOUT[4] event */
    NRF_USBD_INT_ENDEPOUT5_MASK   = USBD_INTEN_ENDEPOUT5_Msk  , /**< Enable or disable interrupt for ENDEPOUT[5] event */
    NRF_USBD_INT_ENDEPOUT6_MASK   = USBD_INTEN_ENDEPOUT6_Msk  , /**< Enable or disable interrupt for ENDEPOUT[6] event */
    NRF_USBD_INT_ENDEPOUT7_MASK   = USBD_INTEN_ENDEPOUT7_Msk  , /**< Enable or disable interrupt for ENDEPOUT[7] event */
    NRF_USBD_INT_ENDISOOUT0_MASK  = USBD_INTEN_ENDISOOUT_Msk  , /**< Enable or disable interrupt for ENDISOOUT[0] event */
    NRF_USBD_INT_SOF_MASK         = USBD_INTEN_SOF_Msk        , /**< Enable or disable interrupt for SOF event */
    NRF_USBD_INT_USBEVENT_MASK    = USBD_INTEN_USBEVENT_Msk   , /**< Enable or disable interrupt for USBEVENT event */
    NRF_USBD_INT_EP0SETUP_MASK    = USBD_INTEN_EP0SETUP_Msk   , /**< Enable or disable interrupt for EP0SETUP event */
    NRF_USBD_INT_DATAEP_MASK      = USBD_INTEN_EPDATA_Msk     , /**< Enable or disable interrupt for EPDATA event */
    NRF_USBD_INT_ACCESSFAULT_MASK = USBD_INTEN_ACCESSFAULT_Msk, /**< Enable or disable interrupt for ACCESSFAULT event */
}nrf_usbd_int_mask_t;

/**
 * @brief Lowest position of bit for IN endpoint
 *
 * The first bit position corresponding to IN endpoint.
 * @sa ep2bit bit2ep
 */
#define USBD_EPIN_BITPOS_0   0

/**
 * @brief Lowest position of bit for OUT endpoint
 *
 * The first bit position corresponding to OUT endpoint
 * @sa ep2bit bit2ep
 */
#define USBD_EPOUT_BITPOS_0  16


void fix_errata_104(uint8_t m_dma_pending, uint32_t enabled,
                    uint32_t *active, uint32_t *m_simulated_dataepstatus)
{
    /* Event correcting */
    if ((0 == m_dma_pending) && (0 != (*active & (USBD_INTEN_SOF_Msk))))
    {
        uint8_t usbi, uoi, uii;
        /* Testing */
        *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7A9;
        uii = (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
        if (0 != uii)
        {
            uii &= (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
        }

        *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AA;
        uoi = (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
        if (0 != uoi)
        {
            uoi &= (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
        }
        *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AB;
        usbi = (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
        if (0 != usbi)
        {
            usbi &= (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
        }
        /* Processing */
        *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AC;
        uii &= (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
        if (0 != uii)
        {
            //uint8_t rb;
            *m_simulated_dataepstatus |= ((uint32_t)uii) << USBD_EPIN_BITPOS_0;
            *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7A9;
            *((volatile uint32_t *)(NRF_USBD_BASE + 0x804)) = uii;
            //rb = (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
        //NRF_DRV_USBD_LOG_PROTO1_FIX_PRINTF("   uii: 0x%.2x (0x%.2x)", uii, rb);
        }

        *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AD;
        uoi &= (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
        if (0 != uoi)
        {
            //uint8_t rb;
            *m_simulated_dataepstatus |= ((uint32_t)uoi) << USBD_EPOUT_BITPOS_0;
            *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AA;
            *((volatile uint32_t *)(NRF_USBD_BASE + 0x804)) = uoi;
            //rb = (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
        //NRF_DRV_USBD_LOG_PROTO1_FIX_PRINTF("   uoi: 0x%.2u (0x%.2x)", uoi, rb);
        }

        *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AE;
        usbi &= (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
        if (0 != usbi)
        {
            //uint8_t rb;
            if (usbi & 0x01)
            {
                *active |= USBD_INTEN_EP0SETUP_Msk;
            }
            if (usbi & 0x10)
            {
                *active |= USBD_INTEN_USBRESET_Msk;
            }
            *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AB;
            *((volatile uint32_t *)(NRF_USBD_BASE + 0x804)) = usbi;
            //rb = (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
        //NRF_DRV_USBD_LOG_PROTO1_FIX_PRINTF("   usbi: 0x%.2u (0x%.2x)", usbi, rb);
        }

        if (0 != (*m_simulated_dataepstatus &
            ~((1U << USBD_EPOUT_BITPOS_0) | (1U << USBD_EPIN_BITPOS_0))))
        {
            *active |= enabled & NRF_USBD_INT_DATAEP_MASK;
        }
        if (0 != (*m_simulated_dataepstatus &
            ((1U << USBD_EPOUT_BITPOS_0) | (1U << USBD_EPIN_BITPOS_0))))
        {
            if (0 != (enabled & NRF_USBD_INT_EP0DATADONE_MASK))
            {
                *m_simulated_dataepstatus &=
                    ~((1U << USBD_EPOUT_BITPOS_0) | (1U << USBD_EPIN_BITPOS_0));
                *active |= NRF_USBD_INT_EP0DATADONE_MASK;
            }
        }
    }
}

