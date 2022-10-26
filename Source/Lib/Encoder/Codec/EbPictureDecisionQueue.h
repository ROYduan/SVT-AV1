/*
* Copyright(c) 2019 Intel Corporation
*
* This source code is subject to the terms of the BSD 2 Clause License and
* the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
* was not distributed with this source code in the LICENSE file, you can
* obtain it at https://www.aomedia.org/license/software-license. If the Alliance for Open
* Media Patent License 1.0 was not distributed with this source code in the
* PATENTS file, you can obtain it at https://www.aomedia.org/license/patent-license.
*/

#ifndef EbPictureDecisionQueue_h
#define EbPictureDecisionQueue_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbPredictionStructure.h"
#include "EbObject.h"
/************************************************
 * PA Reference Queue Entry
 ************************************************/
typedef struct PaReferenceQueueEntry {
    EbDctor          dctor;
    EbObjectWrapper *input_object_ptr;
    EbObjectWrapper *eb_y8b_wrapper_ptr;
    uint64_t         picture_number;
#if OPT_REPLACE_DEP_CNT
#if OPT_PD_REF_QUEUE
    bool is_valid;
#else
    uint8_t refresh_frame_mask;
#endif
    uint64_t decode_order;
#else
    uint32_t         dependent_count;
#endif
#if !OPT_REPLACE_DEP_CNT_CL
    ReferenceList   *list0_ptr;
    ReferenceList   *list1_ptr;
    uint32_t         dep_list0_count;
    uint32_t         dep_list1_count;
    DependentList    list0;
    DependentList    list1;
#endif
    uint8_t          is_alt_ref;
} PaReferenceQueueEntry;

extern EbErrorType pa_reference_queue_entry_ctor(PaReferenceQueueEntry *entry_ptr);

#endif // EbPictureDecisionQueue_h
