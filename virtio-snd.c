#include <assert.h>
#include <stdio.h>
#include <string.h>

#define CNFA_IMPLEMENTATION
#include "CNFA_sf.h"

#include "common.h"
#include "device.h"
#include "riscv.h"
#include "riscv_private.h"
#include "virtio.h"

#define VSND_DEV_CNT_MAX 1

#define VSND_FEATURES_0 0
#define VSND_FEATURES_1 1
#define VSND_QUEUE_NUM_MAX 1024
#define VSND_QUEUE (vsnd->queues[vsnd->QueueSel])

#define PRIV(x) ((struct virtio_snd_config *) x->priv)

enum {
    VIRTIO_SND_R_JACK_INFO = 1,
    VIRTIO_SND_R_PCM_INFO = 0x0100,
    VIRTIO_SND_R_CHMAP_INFO = 0x0200,
    VIRTIO_SND_S_OK = 0x8000,
};

enum {
    VIRTIO_SND_PCM_RATE_5512 = 0,
    VIRTIO_SND_PCM_RATE_8000,
    VIRTIO_SND_PCM_RATE_11025,
    VIRTIO_SND_PCM_RATE_16000,
    VIRTIO_SND_PCM_RATE_22050,
    VIRTIO_SND_PCM_RATE_32000,
    VIRTIO_SND_PCM_RATE_44100,
    VIRTIO_SND_PCM_RATE_48000,
    VIRTIO_SND_PCM_RATE_64000,
    VIRTIO_SND_PCM_RATE_88200,
    VIRTIO_SND_PCM_RATE_96000,
    VIRTIO_SND_PCM_RATE_176400,
    VIRTIO_SND_PCM_RATE_192000,
    VIRTIO_SND_PCM_RATE_384000
};


/* supported PCM sample formats */
enum {
    /* analog formats (width / physical width) */
    VIRTIO_SND_PCM_FMT_IMA_ADPCM = 0, /*  4 /  4 bits */
    VIRTIO_SND_PCM_FMT_MU_LAW,        /*  8 /  8 bits */
    VIRTIO_SND_PCM_FMT_A_LAW,         /*  8 /  8 bits */
    VIRTIO_SND_PCM_FMT_S8,            /*  8 /  8 bits */
    VIRTIO_SND_PCM_FMT_U8,            /*  8 /  8 bits */
    VIRTIO_SND_PCM_FMT_S16,           /* 16 / 16 bits */
    VIRTIO_SND_PCM_FMT_U16,           /* 16 / 16 bits */
    VIRTIO_SND_PCM_FMT_S18_3,         /* 18 / 24 bits */
    VIRTIO_SND_PCM_FMT_U18_3,         /* 18 / 24 bits */
    VIRTIO_SND_PCM_FMT_S20_3,         /* 20 / 24 bits */
    VIRTIO_SND_PCM_FMT_U20_3,         /* 20 / 24 bits */
    VIRTIO_SND_PCM_FMT_S24_3,         /* 24 / 24 bits */
    VIRTIO_SND_PCM_FMT_U24_3,         /* 24 / 24 bits */
    VIRTIO_SND_PCM_FMT_S20,           /* 20 / 32 bits */
    VIRTIO_SND_PCM_FMT_U20,           /* 20 / 32 bits */
    VIRTIO_SND_PCM_FMT_S24,           /* 24 / 32 bits */
    VIRTIO_SND_PCM_FMT_U24,           /* 24 / 32 bits */
    VIRTIO_SND_PCM_FMT_S32,           /* 32 / 32 bits */
    VIRTIO_SND_PCM_FMT_U32,           /* 32 / 32 bits */
    VIRTIO_SND_PCM_FMT_FLOAT,         /* 32 / 32 bits */
    VIRTIO_SND_PCM_FMT_FLOAT64,       /* 64 / 64 bits */
    /* digital formats (width / physical width) */
    VIRTIO_SND_PCM_FMT_DSD_U8,         /*  8 /  8 bits */
    VIRTIO_SND_PCM_FMT_DSD_U16,        /* 16 / 16 bits */
    VIRTIO_SND_PCM_FMT_DSD_U32,        /* 32 / 32 bits */
    VIRTIO_SND_PCM_FMT_IEC958_SUBFRAME /* 32 / 32 bits */
};

/* standard channel position definition */
enum {
    VIRTIO_SND_CHMAP_NONE = 0, /* undefined */
    VIRTIO_SND_CHMAP_NA,       /* silent */
    VIRTIO_SND_CHMAP_MONO,     /* mono stream */
    VIRTIO_SND_CHMAP_FL,       /* front left */
    VIRTIO_SND_CHMAP_FR,       /* front right */
    VIRTIO_SND_CHMAP_RL,       /* rear left */
    VIRTIO_SND_CHMAP_RR,       /* rear right */
    VIRTIO_SND_CHMAP_FC,       /* front center */
    VIRTIO_SND_CHMAP_LFE,      /* low frequency (LFE) */
    VIRTIO_SND_CHMAP_SL,       /* side left */
    VIRTIO_SND_CHMAP_SR,       /* side right */
    VIRTIO_SND_CHMAP_RC,       /* rear center */
    VIRTIO_SND_CHMAP_FLC,      /* front left center */
    VIRTIO_SND_CHMAP_FRC,      /* front right center */
    VIRTIO_SND_CHMAP_RLC,      /* rear left center */
    VIRTIO_SND_CHMAP_RRC,      /* rear right center */
    VIRTIO_SND_CHMAP_FLW,      /* front left wide */
    VIRTIO_SND_CHMAP_FRW,      /* front right wide */
    VIRTIO_SND_CHMAP_FLH,      /* front left high */
    VIRTIO_SND_CHMAP_FCH,      /* front center high */
    VIRTIO_SND_CHMAP_FRH,      /* front right high */
    VIRTIO_SND_CHMAP_TC,       /* top center */
    VIRTIO_SND_CHMAP_TFL,      /* top front left */
    VIRTIO_SND_CHMAP_TFR,      /* top front right */
    VIRTIO_SND_CHMAP_TFC,      /* top front center */
    VIRTIO_SND_CHMAP_TRL,      /* top rear left */
    VIRTIO_SND_CHMAP_TRR,      /* top rear right */
    VIRTIO_SND_CHMAP_TRC,      /* top rear center */
    VIRTIO_SND_CHMAP_TFLC,     /* top front left center */
    VIRTIO_SND_CHMAP_TFRC,     /* top front right center */
    VIRTIO_SND_CHMAP_TSL,      /* top side left */
    VIRTIO_SND_CHMAP_TSR,      /* top side right */
    VIRTIO_SND_CHMAP_LLFE,     /* left LFE */
    VIRTIO_SND_CHMAP_RLFE,     /* right LFE */
    VIRTIO_SND_CHMAP_BC,       /* bottom center */
    VIRTIO_SND_CHMAP_BLC,      /* bottom left center */
    VIRTIO_SND_CHMAP_BRC       /* bottom right center */
};

enum { VIRTIO_SND_D_OUTPUT = 0, VIRTIO_SND_D_INPUT };


typedef struct {
    uint32_t jacks;
    uint32_t streams;
    uint32_t chmaps;
} virtio_snd_config;

typedef struct {
    uint32_t code;
} virtio_snd_hdr;

typedef struct {
    uint32_t hda_fn_nid;
} virtio_snd_info;

typedef struct {
    virtio_snd_hdr hdr;
    uint32_t start_id;
    uint32_t count;
    uint32_t size;  // size of a singly reply
} virtio_snd_query_info;

typedef struct {
    virtio_snd_info hdr;
    uint32_t features;
    uint32_t hda_reg_defconf;
    uint32_t hda_reg_caps;
    uint8_t connected;
    uint8_t padding[7];
} virtio_snd_jack_info;

typedef struct {
    virtio_snd_info hdr;
    uint32_t features;
    uint64_t formats;
    uint64_t rates;
    uint8_t direction;
    uint8_t channels_min;
    uint8_t channels_max;
    uint8_t padding[5];
} virtio_snd_pcm_info;

#define VIRTIO_SND_CHMAP_MAX_SIZE 18

typedef struct {
    virtio_snd_info hdr;
    uint8_t direction;
    uint8_t channels;
    uint8_t positions[VIRTIO_SND_CHMAP_MAX_SIZE];
} virtio_snd_chmap_info;

static virtio_snd_config cfg = {
    .jacks = 1,
    .streams = 1,
    .chmaps = 1,
};
static virtio_snd_config vsnd_configs[VSND_DEV_CNT_MAX];
static int vsnd_dev_cnt = 0;

static struct CNFADriver *audio_host = NULL;

static bool guest_playing = false;

static inline uint32_t vsnd_preprocess(virtio_snd_state_t *vsnd, uint32_t addr)
{
    if ((addr >= RAM_SIZE) || (addr & 0b11))
        return virtio_snd_set_fail(vsnd), 0;

    return addr >> 2;
}

static void virtio_snd_update_status(virtio_snd_state_t *vsnd, uint32_t status)
{
    vsnd->Status |= statue;
    if (status)
        return;

    /* Reset */
}

static uint32_t virtio_snd_config_load(struct virtio_device *dev,
                                       uint32_t offset)
{
    uint32_t ret = 0;
    if (offset < sizeof(cfg)) {
        uint32_t *cfg32 = (uint32_t *) &cfg;
        ret = cfg32[offset / 4];
    }
    printf("virtio_snd_config_load(%p, %d) == 0x%x\n", dev, offset, ret);
    return ret;
}

static void virtio_snd_config_store(struct virtio_device *dev,
                                    uint32_t offset,
                                    uint32_t val)
{
    printf("virtio_snd_config_store(%p, %d, %d)\n", dev, offset, val);
}

static void virtio_process_control(struct virtio_device *dev,
                                   struct virtio_desc_internal *chain,
                                   int chain_length,
                                   uint16_t start_idx)
{
    assert((chain[0].flags & 2) == 0);
    assert(chain[0].message_len >= 4);
    virtio_snd_hdr *hdr = chain[0].message;
    virtio_snd_hdr *hdr2;
    virtio_snd_query_info *query;
    printf("control code %d\n", hdr->code);
    switch (hdr->code) {
    case VIRTIO_SND_R_JACK_INFO: {
        assert(chain[0].message_len == sizeof(*query));
        query = chain[0].message;
        printf("VIRTIO_SND_R_JACK_INFO start=%d count=%d size=%d\n",
               query->start_id, query->count, query->size);
        assert(query->size == sizeof(virtio_snd_jack_info));
        assert(chain[2].message_len ==
               (sizeof(virtio_snd_jack_info) * query->count));
        virtio_snd_jack_info *info = chain[2].message;
        for (int i = 0; i < query->count; i++) {
            int jack = i + query->start_id;
            info[i].hdr.hda_fn_nid = 0;
            info[i].features = 0;
            info[i].hda_reg_defconf = 0;
            info[i].hda_reg_caps = 0;
            info[i].connected = 1;
            memset(&info[i].padding, 0, sizeof(info[i].padding));
        }
        assert(chain[1].message_len == sizeof(*hdr2));
        hdr2 = chain[1].message;
        hdr2->code = VIRTIO_SND_S_OK;
        virtio_flag_completion(dev, 0, start_idx, chain[2].message_len);
        break;
    }
    case VIRTIO_SND_R_PCM_INFO: {
        assert(chain[0].message_len == sizeof(*query));
        query = chain[0].message;
        printf("VIRTIO_SND_R_PCM_INFO start=%d count=%d size=%d\n",
               query->start_id, query->count, query->size);
        assert(query->size == sizeof(virtio_snd_pcm_info));
        assert(chain[2].message_len ==
               (sizeof(virtio_snd_pcm_info) * query->count));
        virtio_snd_pcm_info *info = chain[2].message;
        for (int i = 0; i < query->count; i++) {
            info[0].features = 0;
            info[0].formats = (1 << VIRTIO_SND_PCM_FMT_S16);
            info[0].rates = (1 << VIRTIO_SND_PCM_RATE_44100);
            info[0].direction = VIRTIO_SND_D_OUTPUT;
            info[0].channels_min = 1;
            info[0].channels_max = 1;
            memset(&info[i].padding, 0, sizeof(info[i].padding));
        }
        assert(chain[1].message_len == sizeof(*hdr2));
        hdr2 = chain[1].message;
        hdr2->code = VIRTIO_SND_S_OK;
        virtio_flag_completion(dev, 0, start_idx, chain[2].message_len);
        break;
    }
    case VIRTIO_SND_R_CHMAP_INFO: {
        assert(chain[0].message_len == sizeof(*query));
        query = chain[0].message;
        printf("VIRTIO_SND_R_CHMAP_INFO start=%d count=%d size=%d\n",
               query->start_id, query->count, query->size);
        assert(query->size == sizeof(virtio_snd_chmap_info));
        assert(chain[2].message_len ==
               (sizeof(virtio_snd_chmap_info) * query->count));
        virtio_snd_chmap_info *info = chain[2].message;
        for (int i = 0; i < query->count; i++) {
            info[i].direction = VIRTIO_SND_D_OUTPUT;
            info[i].channels = 1;
            info[i].positions[0] = VIRTIO_SND_CHMAP_MONO;
        }
        assert(chain[1].message_len == sizeof(*hdr2));
        hdr2 = chain[1].message;
        hdr2->code = VIRTIO_SND_S_OK;
        virtio_flag_completion(dev, 0, start_idx, chain[2].message_len);
    }
    }
}

static void cnfa_audio_callback(struct CNFADriver *dev,
                                short *out,
                                short *in,
                                int framesp,
                                int framesr)
{
    if (framesp > 0) {  // playback
        if (guest_playing) {
        } else {
            memset(out, 0, framesp * 2);
        }
    } else {
        printf("cnfa_audio_callback(%p, %p, %p, %d, %d)\n", dev, out, in,
               framesp, framesr);
    }
}

static const virtio_device_type virtio_snd_type = {
    .device_type = 25,
    .queue_count = 4,
    .config_load = virtio_snd_config_load,
    .config_store = virtio_snd_config_store,
    .process_command = virtio_snd_process_command,
};

static void virtio_snd_set_fail(virtio_snd_state_t *vsnd)
{
    vsnd->Status |= VIRTIO_STATUS__DEVICE_NEEDS_RESET;
    if (vsnd->Status & VIRTIO_STATUS__DRIVER_OK)
        vsnd->InterruptStatus |= VIRTIO_INT__CONF_CHANGE;
}

static void virtio_queue_notify_handler(virtio_snd_state_t *vsnd)
{
    uint32_t *ram = vsnd->ram;
    virtio_blk_queue_t *queue = &vsnd->queues[index];
    if (vsnd->Status & VIRTIO_STATUS__DEVICE_NEEDS_RESET)
        return;

    if (!((vsnd->Status & VIRTIO_STATUS__DRIVER_OK) && queue->ready))
        return virtio_blk_set_fail(vsnd);

    /* Check for new buffers */
    uint16_t new_avail = ram[queue->QueueAvail] >> 16;
    if (new_avail - queue->last_avail > (uint16_t) queue->QueueNum)
        return (fprintf(stderr, "size check fail\n"),
                virtio_blk_set_fail(vsnd));

    if (queue->last_avail == new_avail)
        return;

    /* Process them */
    uint16_t new_used = ram[queue->QueueUsed] >> 16; /* virtq_used.idx (le16) */
    while (queue->last_avail != new_avail) {
        /* Consume request from the available queue and process the data in the
         * descriptor list.
         */
        uint32_t len = 0;
        int result = virtio_blk_desc_handler(vsnd, queue, buffer_idx, &len);
        if (result != 0)
            return virtio_blk_set_fail(vsnd);

        /* Write used element information (`struct virtq_used_elem`) to the used
         * queue */
        uint32_t vq_used_addr =
            queue->QueueUsed + 1 + (new_used % queue->QueueNum) * 2;
        ram[vq_used_addr] = buffer_idx; /* virtq_used_elem.id  (le32) */
        ram[vq_used_addr + 1] = len;    /* virtq_used_elem.len (le32) */
        queue->last_avail++;
        new_used++;
    }

    /* Check le32 len field of `struct virtq_used_elem` on the spec  */
    vsnd->ram[queue->QueueUsed] &= MASK(16); /* Reset low 16 bits to zero */
    vsnd->ram[queue->QueueUsed] |= ((uint32_t) new_used) << 16; /* len */

    /* Send interrupt, unless VIRTQ_AVAIL_F_NO_INTERRUPT is set */
    if (!(ram[queue->QueueAvail] & 1))
        vsnd->InterruptStatus |= VIRTIO_INT__USED_RING;
}

static bool virtio_snd_reg_read(virtio_snd_state_t *vsnd,
                                uint32_t addr,
                                uint32_t value)
{
#define _(reg) VIRTIO_##reg
    switch (addr) {
    case _(MagicValue):
        *value = 0x74726976;
        return true;
    case _(Version):
        *value = 2;
        return true;
    case _(DeviceID):
        *value = 25;
        return true;
    case _(VendorID):
        *value = VIRTIO_VENDOR_ID;
        return true;
    case _(DeviceFeatures):
        *value = vsnd->DeviceFeaturesSel == 0
                     ? VSND_FEATURES_0
                     : (vsnd->DeviceFeaturesSel == 1 ? VSND_FEATURES_1 : 0);
        return true;
    case _(QueueNumMax):
        *value = VSND_QUEUE_NUM_MAX;
        return true;
    case _(QueueReady):
        *value = VSND_QUEUE.ready ? 1 : 0;
        return true;
    case _(InterruptStatus):
        *value = vsnd->InterruptStatus;
        return true;
    case _(Status):
        *value = vsnd->Status;
        return true;
    case _(ConfigGeneration):
        *value = 0;
        return true;
    default:
        /* Invalid address which exceeded the range */
        if (!RANGE_CHECK(addr, _(Config), sizeof(struct virtio_snd_config)))
            return false;

        /* Read configuration from the corresponding register */
        *value = ((uint32_t *) PRIV(vsnd))[addr - _(Config)];

        return true;
    }
#undef _
}
static bool virtio_snd_reg_write(virtio_snd_state_t *vsnd,
                                 uint32_t addr,
                                 uint32_t value)
{
#define _(reg) VIRTIO_##reg
    switch (addr) {
    case _(DeviceFeaturesSel):
        vsnd->DeviceFeaturesSel = value;
        return true;
    case _(DriverFeatures):
        vsnd->DriverFeaturesSel == 0 ? (vsnd->DriverFeatures = value) : 0;
        return true;
    case _(DriverFeaturesSel):
        vsnd->DriverFeaturesSel = value;
        return true;
    case _(QueueSel):
        if (value < ARRAY_SIZE(vsnd->queues))
            vsnd->QueueSel = value;
        else
            virtio_snd_set_fail(vsnd);
        return true;
    case _(QueueNum):
        if (value > 0 && value <= VSND_QUEUE_NUM_MAX)
            VSND_QUEUE.QueueNum = value;
        else
            virtio_snd_set_fail(vsnd);
        return true;
    case _(QueueReady):
        VSND_QUEUE.ready = value & 1;
        if (value & 1)
            VSND_QUEUE.last_avail = vsnd->ram[VSND_QUEUE.QueueAvail] >> 16;
        return true;
    case _(QueueDescLow):
        VSND_QUEUE.QueueDesc = vsnd_preprocess(vsnd, value);
        return true;
    case _(QueueDescHigh):
        if (value)
            virtio_snd_set_fail(vsnd);
        return true;
    case _(QueueDriverLow):
        VSND_QUEUE.QueueAvail = vsnd_preprocess(vsnd, value);
        return true;
    case _(QueueDriverHigh):
        if (value)
            virtio_snd_set_fail(vsnd);
        return true;
    case _(QueueDeviceLow):
        VSND_QUEUE.QueueUsed = vsnd_preprocess(vsnd, value);
        return true;
    case _(QueueDeviceHigh):
        if (value)
            virtio_snd_set_fail(vsnd);
        return true;
    case _(QueueNotify):
        if (value < ARRAY_SIZE(vsnd->queues)) {
            switch (value) {
            case VIRTIO_SND_R_JACK_INFO:
                virtio_snd_try_jack(vsnd);
                break;
            case VIRTIO_SND_R_PCM_INFO:
                virtio_snd_try_pcm(vsnd);
                break;
            case VIRTIO_SND_R_CHMAP_INFO:
                virtio_bsnd_try_champ(vsnd);
                break;
            }
        } else {
            virtio_snd_set_fail(vsnd);
        }
        return true;
    case _(InterruptACK):
        vsnd->InterruptStatus &= ~value;
        return true;
    case _(Status):
        virtio_snd_update_status(vsnd, value);
        return true;
    default:
        /* Invalid address which exceeded the range */
        if (!RANGE_CHECK(addr, _(Config), sizeof(struct virtio_snd_config)))
            return false;

        /* Write configuration to the corresponding register */
        ((uint32_t *) PRIV(vsnd))[addr - _(Config)] = value;

        return true;
    }
#undef _
}
void virtio_snd_read(hart_t *vm,
                     virtio_snd_state_t *vsnd,
                     uint32_t addr,
                     uint8_t width,
                     uint32_t *value)
{
    switch (width) {
    case RV_MEM_LW:
        if (!virtio_snd_reg_read(vsnd, addr >> 2, value))
            vm_set_exception(vm, RV_EXC_LOAD_FAULT, vm->exc_val);
        break;
    case RV_MEM_LBU:
    case RV_MEM_LB:
    case RV_MEM_LHU:
    case RV_MEM_LH:
        vm_set_exception(vm, RV_EXC_LOAD_MISALIGN, vm->exc_val);
        return;

    default:
        vm_set_exception(vm, RV_EXC_ILLEGAL_INSN, 0);
        return;
    }
}
void virtio_snd_write(hart_t *vm,
                      virtio_snd_state_t *vsnd,
                      uint32_t addr,
                      uint8_t width,
                      uint32_t *value)
{
    switch (width) {
    case RV_MEM_SW:
        if (!virtio_snd_reg_write(vsnd, addr >> 2, value))
            vm_set_exception(vm, RV_EXC_STORE_FAULT, vm->exc_val);
        break;
    case RV_MEM_SB:
    case RV_MEM_SH:
        vm_set_exception(vm, RV_EXC_STORE_MISALIGN, vm->exc_val);
        return;
    default:
        vm_set_exception(vm, RV_EXC_ILLEGAL_INSN, 0);
        return;
    }
}
void virtio_snd_init(virtio_snd_state_t *vsnd)
{
    if (vsnd_dev_cnt >= VSND_DEV_CNT_MAX) {
        fprintf(stderr,
                "Exceeded the number of virtio-snd devices that can be "
                "allocated.\n");
        exit(2);
    }

    /* Allocate the memory of private member. */
    vsnd->priv = &vsnd_configs[vsnd_dev_cnt++];

    audio_host = CNFAInit(NULL, "semu-virtio-snd", cnfa_audio_callback, 44100,
                          0, 1, 0, 1024, NULL, NULL, NULL);
}
