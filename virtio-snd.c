#include <stdio.h>
#include <string.h>

#define CNFA_IMPLEMENTATION
#include "CNFA_sf.h"

#include "device.h"
#include "riscv.h"
#include "riscv_private.h"
#include "virtio.h"

#define VSND_DEV_CNT_MAX 1

#define VSND_QUEUE_NUM_MAX 1024
#define vsndq (vsnd->queues[vsnd->QueueSel])

#define PRIV(x) ((virtio_snd_config_t *) x->priv)

enum {
    VSND_QUEUE_CTRL = 0,
    VSND_QUEUE_EVT = 1,
    VSND_QUEUE_TX = 2,
    VSND_QUEUE_RX = 3,
};

/* supported virtio sound version */
enum {
    VSND_FEATURES_0 = 0,
    VSND_FEATURES_1,
};

enum {
    /* jack control requests types */
    VIRTIO_SND_R_JACK_INFO = 1,

    /* PCM control requests types */
    VIRTIO_SND_R_PCM_INFO = 0x0100,
    VIRTIO_SND_R_PCM_SET_PARAMS,
    VIRTIO_SND_R_PCM_PREPARE,
    VIRTIO_SND_R_PCM_RELEASE,
    VIRTIO_SND_R_PCM_START,
    VIRTIO_SND_R_PCM_STOP,

    /* channel map control requests types */
    VIRTIO_SND_R_CHMAP_INFO = 0x0200,

    /* common status codes */
    VIRTIO_SND_S_OK = 0x8000,
    VIRTIO_SND_S_BAD_MSG,
    VIRTIO_SND_S_NOT_SUPP,
    VIRTIO_SND_S_IO_ERR,
};

/* supported PCM frame rates */
enum {
    VIRTIO_SND_PCM_RATE_5512 = 0, /* 5512 Hz */
    VIRTIO_SND_PCM_RATE_8000,     /* 8000 Hz */
    VIRTIO_SND_PCM_RATE_11025,    /* 11025 Hz */
    VIRTIO_SND_PCM_RATE_16000,    /* 16000 Hz */
    VIRTIO_SND_PCM_RATE_22050,    /* 22050 Hz */
    VIRTIO_SND_PCM_RATE_32000,    /* 32000 Hz */
    VIRTIO_SND_PCM_RATE_44100,    /* 44100 Hz */
    VIRTIO_SND_PCM_RATE_48000,    /* 48000 Hz */
    VIRTIO_SND_PCM_RATE_64000,    /* 64000 Hz */
    VIRTIO_SND_PCM_RATE_88200,    /* 88200 Hz */
    VIRTIO_SND_PCM_RATE_96000,    /* 96000 Hz */
    VIRTIO_SND_PCM_RATE_176400,   /* 176400 Hz */
    VIRTIO_SND_PCM_RATE_192000,   /* 192000 Hz */
    VIRTIO_SND_PCM_RATE_384000,   /* 384000 Hz */
};

/* supported PCM stream features */
enum {
    VIRTIO_SND_PCM_F_SHMEM_HOST = 0,
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
    VIRTIO_SND_PCM_FMT_DSD_U8,          /*  8 /  8 bits */
    VIRTIO_SND_PCM_FMT_DSD_U16,         /* 16 / 16 bits */
    VIRTIO_SND_PCM_FMT_DSD_U32,         /* 32 / 32 bits */
    VIRTIO_SND_PCM_FMT_IEC958_SUBFRAME, /* 32 / 32 bits */
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
    VIRTIO_SND_CHMAP_BRC,      /* bottom right center */
};

/* audio data flow direction */
enum {
    VIRTIO_SND_D_OUTPUT = 0,
    VIRTIO_SND_D_INPUT,
};

typedef struct {
    uint32_t jacks;
    uint32_t streams;
    uint32_t chmaps;
} virtio_snd_config_t;

/* VirtIO sound common header */
typedef struct {
    uint32_t code;
} virtio_snd_hdr_t;

typedef struct {
    uint32_t hda_fn_nid;
} virtio_snd_info_t;

typedef struct {
    virtio_snd_hdr_t hdr;
    uint32_t start_id;
    uint32_t count;
    uint32_t size;
} virtio_snd_query_info_t;

typedef struct {
    virtio_snd_info_t hdr;
    uint32_t features;
    uint32_t hda_reg_defconf;
    uint32_t hda_reg_caps;
    uint8_t connected;
    uint8_t padding[7];
} virtio_snd_jack_info_t;

typedef struct {
    virtio_snd_info_t hdr;
    uint32_t features;
    uint64_t formats;
    uint64_t rates;
    uint8_t direction;
    uint8_t channels_min;
    uint8_t channels_max;
    uint8_t padding[5];
} virtio_snd_pcm_info_t;

typedef struct {
    virtio_snd_hdr_t hdr;
    uint32_t stream_id;
} virtio_snd_pcm_hdr_t;

typedef struct {
    virtio_snd_pcm_hdr_t hdr; /* .code = VIRTIO_SND_R_PCM_SET_PARAMS */
    uint32_t buffer_bytes;
    uint32_t period_bytes;
    uint32_t features; /* 1 << VIRTIO_SND_PCM_F_XXX */
    uint8_t channels;
    uint8_t format;
    uint8_t rate;
    uint8_t padding;
} virtio_snd_pcm_set_params_t;

/* PCM I/O message header */
typedef struct {
    uint32_t stream_id;
} virtio_snd_pcm_xfer_t;

/* PCM I/O message status */
typedef struct {
    uint32_t status;
    uint32_t latency_bytes;
} virtio_snd_pcm_status_t;

#define VIRTIO_SND_CHMAP_MAX_SIZE 18

typedef struct {
    virtio_snd_info_t hdr;
    uint8_t direction;
    uint8_t channels;
    uint8_t positions[VIRTIO_SND_CHMAP_MAX_SIZE];
} virtio_snd_chmap_info_t;

/* virtio-snd to hold the settings of each stream */
typedef struct {
    virtio_snd_jack_info_t j;
    virtio_snd_pcm_info_t p;
    virtio_snd_chmap_info_t c;
    virtio_snd_pcm_set_params_t pp;
    struct CNFADriver *audio_host;
    bool is_guest_playing;
} virtio_snd_prop_t;

static virtio_snd_config_t vsnd_configs[VSND_DEV_CNT_MAX];
static virtio_snd_prop_t vsnd_props[VSND_DEV_CNT_MAX];
static int vsnd_dev_cnt = 0;

// static struct CNFADriver *audio_host = NULL;

static bool guest_playing = false;

/* Forward declaration */
static void virtio_snd_cb(struct CNFADriver *dev,
                          short *out,
                          short *in,
                          int framesp,
                          int framesr);


static void virtio_snd_set_fail(virtio_snd_state_t *vsnd)
{
    vsnd->Status |= VIRTIO_STATUS__DEVICE_NEEDS_RESET;
    if (vsnd->Status & VIRTIO_STATUS__DRIVER_OK)
        vsnd->InterruptStatus |= VIRTIO_INT__CONF_CHANGE;
}

/* Check whether the address is valid or not */
static inline uint32_t vsnd_preprocess(virtio_snd_state_t *vsnd, uint32_t addr)
{
    if ((addr >= RAM_SIZE) || (addr & 0b11))
        return virtio_snd_set_fail(vsnd), 0;

    /* shift right as we have checked in the above */
    return addr >> 2;
}

static void virtio_snd_update_status(virtio_snd_state_t *vsnd, uint32_t status)
{
    vsnd->Status |= status;
    if (status)
        return;

    /* Reset */
    uint32_t *ram = vsnd->ram;
    void *priv = vsnd->priv;
    uint32_t jacks = PRIV(vsnd)->jacks;
    uint32_t streams = PRIV(vsnd)->streams;
    uint32_t chmaps = PRIV(vsnd)->chmaps;
    memset(vsnd, 0, sizeof(*vsnd));
    vsnd->ram = ram;
    vsnd->priv = priv;
    PRIV(vsnd)->jacks = jacks;
    PRIV(vsnd)->streams = streams;
    PRIV(vsnd)->chmaps = chmaps;
}

static void virtio_snd_read_jack_info_handler(
    virtio_snd_jack_info_t *info,
    const virtio_snd_query_info_t *query)
{
    printf("jack info handler count %d\n", query->count);
    for (uint32_t i = 0; i < query->count; i++) {
        info[i].hdr.hda_fn_nid = 0;
        info[i].features = 0;
        info[i].hda_reg_defconf = 0;
        info[i].hda_reg_caps = 0;
        info[i].connected = 1;
        memset(&info[i].padding, 0, sizeof(info[i].padding));

        vsnd_props[i].j.hdr.hda_fn_nid = 0;
        vsnd_props[i].j.features = 0;
        vsnd_props[i].j.hda_reg_defconf = 0;
        vsnd_props[i].j.hda_reg_caps = 0;
        vsnd_props[i].j.connected = 1;
        memset(&vsnd_props[i].j.padding, 0, sizeof(vsnd_props[i].j.padding));
    }
}

static void virtio_snd_read_pcm_info_handler(
    virtio_snd_pcm_info_t *info,
    const virtio_snd_query_info_t *query)
{
    printf("snd info handler count %d\n", query->count);
    for (uint32_t i = 0; i < query->count; i++) {
        info[i].hdr.hda_fn_nid = 0;
        info[i].features = 0;
        info[i].formats = (1 << VIRTIO_SND_PCM_FMT_S16);
        info[i].rates = (1 << VIRTIO_SND_PCM_RATE_44100);
        info[i].direction = VIRTIO_SND_D_OUTPUT;
        info[i].channels_min = 1;
        info[i].channels_max = 1;
        memset(&info[i].padding, 0, sizeof(info[i].padding));

        vsnd_props[i].p.hdr.hda_fn_nid = 0;
        vsnd_props[i].p.features = 0;
        vsnd_props[i].p.formats = (1 << VIRTIO_SND_PCM_FMT_S16);
        vsnd_props[i].p.rates = (1 << VIRTIO_SND_PCM_RATE_44100);
        vsnd_props[i].p.direction = VIRTIO_SND_D_OUTPUT;
        vsnd_props[i].p.channels_min = 1;
        vsnd_props[i].p.channels_max = 1;
        memset(&vsnd_props[i].p.padding, 0, sizeof(vsnd_props[i].p.padding));
    }
}

static void virtio_snd_read_chmap_info_handler(
    virtio_snd_chmap_info_t *info,
    const virtio_snd_query_info_t *query)
{
    printf("chmap info handler count %d\n", query->count);
    for (uint32_t i = 0; i < query->count; i++) {
        info[i].hdr.hda_fn_nid = 0;
        info[i].direction = VIRTIO_SND_D_OUTPUT;
        info[i].channels = 1;
        info[i].positions[0] = VIRTIO_SND_CHMAP_MONO;

        vsnd_props[i].c.hdr.hda_fn_nid = 0;
        vsnd_props[i].c.direction = VIRTIO_SND_D_OUTPUT;
        vsnd_props[i].c.channels = 1;
        vsnd_props[i].c.positions[0] = VIRTIO_SND_CHMAP_MONO;
    }
}

static void virtio_snd_read_pcm_set_params(struct virtq_desc *vq_desc,
                                           const virtio_snd_query_info_t *query)
{
    /* TODO: detect current state of stream */
    /* TODO: check the valiability of buffer_bytes, period_bytes, channel_min,
     * and channel_max */

    virtio_snd_pcm_set_params_t *request = query;
    uint32_t id = request->hdr.stream_id;
    vsnd_props[id].pp.hdr.hdr.code = VIRTIO_SND_R_PCM_SET_PARAMS;
    vsnd_props[id].pp.buffer_bytes = request->buffer_bytes;
    vsnd_props[id].pp.period_bytes = request->period_bytes;
    vsnd_props[id].pp.features = request->features;
    vsnd_props[id].pp.channels = request->channels;
    vsnd_props[id].pp.format = request->format;
    vsnd_props[id].pp.rate = request->rate;
    vsnd_props[id].pp.padding = request->padding;

    fprintf(stderr, "virtio_snd_read_pcm_set_params\n");
}

static void virtio_snd_read_pcm_prepare(struct virtq_desc *vq_desc,
                                        const virtio_snd_query_info_t *query)
{
    virtio_snd_pcm_hdr_t *request = query;
    uint32_t stream_id = request->stream_id;
    vsnd_props[stream_id].pp.hdr.hdr.code = VIRTIO_SND_R_PCM_PREPARE;
    vsnd_props[stream_id].audio_host = CNFAInit(
        NULL, "semu-virtio-snd", virtio_snd_cb, 44100, 0, 1, 0,
        vsnd_props[stream_id].pp.buffer_bytes, NULL, NULL, &guest_playing);

    /* Control the callback to prepare the buffer */
    /* TODO: add lock to avoid race condition */
    guest_playing = false;

    fprintf(stderr, "virtio_snd_read_pcm_prepare\n");
}

static void virtio_snd_read_pcm_start(struct virtq_desc *vq_desc,
                                      const virtio_snd_query_info_t *query)
{
    /* TODO: let application to set stream_id at will */

    /* Control the callback to start playing */
    /* TODO: add lock to avoid race condition */
    guest_playing = true;

    fprintf(stderr, "virtio_snd_read_pcm_start\n");
}

static void virtio_snd_read_pcm_stop(struct virtq_desc *vq_desc,
                                     const virtio_snd_query_info_t *query)
{
    /* TODO: let application to set stream_id at will */

    /* Control the callback to stop playing */
    /* TODO: add lock to avoid race condition */
    guest_playing = false;

    fprintf(stderr, "virtio_snd_read_pcm_stop\n");
}

static void virtio_snd_read_pcm_release(struct virtq_desc *vq_desc,
                                        const virtio_snd_query_info_t *query)
{
    /* Control the callback to stop playing */
    /* TODO: add lock to avoid race condition */
    guest_playing = false;

    virtio_snd_pcm_hdr_t *request = query;
    uint32_t stream_id = request->stream_id;
    vsnd_props[stream_id].pp.hdr.hdr.code = VIRTIO_SND_R_PCM_RELEASE;
    CNFAClose(vsnd_props[stream_id].audio_host);

    fprintf(stderr, "virtio_snd_read_pcm_release\n");
}

static void virtio_snd_cb(struct CNFADriver *dev,
                          short *out,
                          short *in,
                          int framesp,
                          int framesr)
{
    /* TODO: apply lock on guest_playing and guest_playing_ptr */
    int *guest_playing_ptr = (int *) dev->opaque;
    int output_channels = dev->channelsPlay;
    int output_buf_sz = framesp * output_channels;

    if (framesp > 0) {  // playback
        if (!(*guest_playing_ptr)) {
            memset(out, 0, sizeof(short) * output_buf_sz);
            return;
        }
    } else {
        fprintf(stderr, "virtio_snd_cb(%p, %p, %p, %d, %d)\n", dev, out, in,
                framesp, framesr);
    }
}

#define VSND_DESC_CNT 3
static int virtio_snd_desc_handler(virtio_snd_state_t *vsnd,
                                   const virtio_snd_queue_t *queue,
                                   uint32_t desc_idx,
                                   uint32_t *plen)
{
    /* TODO: clarify the use of the third descriptor */
    /* virtio-snd uses at most 3 virtqueue descriptors, where
     * the first descriptor contains:
     *   struct virtio_snd_hdr hdr (for request)
     * the second descriptors contains:
     *   struct virtio_snd_hdr hdr (for response)
     * if needed, the third descriptors contains:
     *   (response payload structure)
     */
    struct virtq_desc vq_desc[VSND_DESC_CNT];

    /* Collect the descriptors */
    for (int i = 0; i < VSND_DESC_CNT; i++) {
        /* The size of the `struct virtq_desc` is 4 words */
        const uint32_t *desc = &vsnd->ram[queue->QueueDesc + desc_idx * 4];

        /* Retrieve the fields of current descriptor */
        vq_desc[i].addr = desc[0];
        vq_desc[i].len = desc[2];
        vq_desc[i].flags = desc[3];
        desc_idx = desc[3] >> 16; /* vq_desc[desc_cnt].next */

        /* Leave the loop if next-flag is not set */
        if (!(vq_desc[i].flags & VIRTIO_DESC_F_NEXT))
            break;
    }

    /* Process the header */
    const virtio_snd_hdr_t *request =
        (virtio_snd_hdr_t *) ((uintptr_t) vsnd->ram + vq_desc[0].addr);
    uint32_t type = request->code;
    virtio_snd_hdr_t *response =
        (virtio_snd_hdr_t *) ((uintptr_t) vsnd->ram + vq_desc[1].addr);
    const virtio_snd_query_info_t *query =
        (virtio_snd_query_info_t *) ((uintptr_t) vsnd->ram + vq_desc[0].addr);

    /* TODO: let the program use this variable selectively according to
     * the type.
     */
    /* As there are plenty of structures for response payload,
     * use a void pointer for generic type support.
     */
    void *info = (void *) (uintptr_t) vsnd->ram + vq_desc[2].addr;

    /* Process the data */
    switch (type) {
    case VIRTIO_SND_R_JACK_INFO:
        virtio_snd_read_jack_info_handler(info, query);
        break;
    case VIRTIO_SND_R_PCM_INFO:
        virtio_snd_read_pcm_info_handler(info, query);
        break;
    case VIRTIO_SND_R_CHMAP_INFO:
        virtio_snd_read_chmap_info_handler(info, query);
        break;
    case VIRTIO_SND_R_PCM_SET_PARAMS:
        virtio_snd_read_pcm_set_params(vq_desc, query);
        break;
    case VIRTIO_SND_R_PCM_PREPARE:
        virtio_snd_read_pcm_prepare(vq_desc, query);
        break;
    case VIRTIO_SND_R_PCM_RELEASE:
        virtio_snd_read_pcm_release(vq_desc, query);
        break;
    case VIRTIO_SND_R_PCM_START:
        virtio_snd_read_pcm_start(vq_desc, query);
        break;
    case VIRTIO_SND_R_PCM_STOP:
        virtio_snd_read_pcm_stop(vq_desc, query);
        break;
    default:
        fprintf(stderr, "%d: unsupported virtio-snd operation!\n", type);
        response->code = VIRTIO_SND_S_NOT_SUPP;
        *plen = 0;
        return -1;
    }

    /* Return the device status */
    response->code = VIRTIO_SND_S_OK;
    *plen = vq_desc[2].len;

    return 0;
}

static int virtio_snd_tx_desc_handler(virtio_snd_state_t *vsnd,
                                      const virtio_snd_queue_t *queue,
                                      uint32_t desc_idx,
                                      uint32_t *plen)
{
    /* TODO: clarify the use of the third descriptor */
    /* virtio-snd uses at most 3 virtqueue descriptors, where
     * the first descriptor contains:
     *   struct virtio_snd_hdr hdr (for request)
     * the second descriptors contains:
     *   struct virtio_snd_hdr hdr (for response)
     * if needed, the third descriptors contains:
     *   (response payload structure)
     */
    struct virtq_desc vq_desc[VSND_DESC_CNT];

    /* Collect the descriptors */
    for (int i = 0; i < VSND_DESC_CNT; i++) {
        /* The size of the `struct virtq_desc` is 4 words */
        const uint32_t *desc = &vsnd->ram[queue->QueueDesc + desc_idx * 4];

        /* Retrieve the fields of current descriptor */
        vq_desc[i].addr = desc[0];
        vq_desc[i].len = desc[2];
        vq_desc[i].flags = desc[3];
        desc_idx = desc[3] >> 16; /* vq_desc[desc_cnt].next */

        /* Leave the loop if next-flag is not set */
        if (!(vq_desc[i].flags & VIRTIO_DESC_F_NEXT))
            break;
    }

    /* Process the header */
    const virtio_snd_pcm_xfer_t *request =
        (virtio_snd_pcm_xfer_t *) ((uintptr_t) vsnd->ram + vq_desc[0].addr);
    uint32_t stream_id = request->stream_id;
    void *payload = (void *) ((uintptr_t) vsnd->ram + vq_desc[1].addr);
    virtio_snd_pcm_status_t *response =
        (virtio_snd_pcm_status_t *) (uintptr_t) vsnd->ram + vq_desc[2].addr;

    /* Process the data */
    fprintf(stderr, "stream %d got %ld bytes.\n", stream_id, sizeof(payload));

    /* Return the device status */
    response->status = VIRTIO_SND_S_OK;
    response->latency_bytes = 0; /* TODO: show the actual latency bytes */
    *plen = vq_desc[2].len;

    return 0;
}


static void virtio_queue_notify_handler(
    virtio_snd_state_t *vsnd,
    int index,
    int (*handler)(virtio_snd_state_t *,
                   const virtio_snd_queue_t *,
                   uint32_t,
                   uint32_t *))
{
    uint32_t *ram = vsnd->ram;
    virtio_snd_queue_t *queue = &vsnd->queues[index];
    if (vsnd->Status & VIRTIO_STATUS__DEVICE_NEEDS_RESET)
        return;

    if (!((vsnd->Status & VIRTIO_STATUS__DRIVER_OK) && queue->ready))
        return virtio_snd_set_fail(vsnd);

    /* Check for new buffers */
    uint16_t new_avail = ram[queue->QueueAvail] >> 16;
    if (new_avail - queue->last_avail > (uint16_t) queue->QueueNum)
        return (fprintf(stderr, "size check fail\n"),
                virtio_snd_set_fail(vsnd));

    if (queue->last_avail == new_avail)
        return;

    /* Process them */
    uint16_t new_used = ram[queue->QueueUsed] >> 16; /* virtq_used.idx (le16) */
    while (queue->last_avail != new_avail) {
        /* Obtain the index in the ring buffer */
        uint16_t queue_idx = queue->last_avail % queue->QueueNum;

        /* Since each buffer index occupies 2 bytes but the memory is aligned
         * with 4 bytes, and the first element of the available queue is stored
         * at ram[queue->QueueAvail + 1], to acquire the buffer index, it
         * requires the following array index calculation and bit shifting.
         * Check also the `struct virtq_avail` on the spec.
         */
        uint16_t buffer_idx = ram[queue->QueueAvail + 1 + queue_idx / 2] >>
                              (16 * (queue_idx % 2));

        /* Consume request from the available queue and process the data in the
         * descriptor list.
         */
        uint32_t len = 0;
        int result = handler(vsnd, queue, buffer_idx, &len);
        if (result != 0)
            return virtio_snd_set_fail(vsnd);

        /* Write used element information (`struct virtq_used_elem`) to the used
         * queue */
        uint32_t vq_used_addr =
            queue->QueueUsed + 1 + (new_used % queue->QueueNum) * 2;
        ram[vq_used_addr] = buffer_idx; /* virtq_used_elem.id  (le32) */
        ram[vq_used_addr + 1] = len;    /* virtq_used_elem.len (le32) */
        queue->last_avail++;
        new_used++;
    }

    /* Check le32 len field of struct virtq_used_elem on the spec  */
    vsnd->ram[queue->QueueUsed] &= MASK(16); /* Reset low 16 bits to zero */
    vsnd->ram[queue->QueueUsed] |= ((uint32_t) new_used) << 16; /* len */

    /* Send interrupt, unless VIRTQ_AVAIL_F_NO_INTERRUPT is set */
    if (!(ram[queue->QueueAvail] & 1))
        vsnd->InterruptStatus |= VIRTIO_INT__USED_RING;
}

static bool virtio_snd_reg_read(virtio_snd_state_t *vsnd,
                                uint32_t addr,
                                uint32_t *value)
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
        *value = vsndq.ready ? 1 : 0;
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
        if (!RANGE_CHECK(addr, _(Config), sizeof(virtio_snd_config_t)))
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
            vsndq.QueueNum = value;
        else
            virtio_snd_set_fail(vsnd);
        return true;
    case _(QueueReady):
        vsndq.ready = value & 1;
        if (value & 1)
            vsndq.last_avail = vsnd->ram[vsndq.QueueAvail] >> 16;
        return true;
    case _(QueueDescLow):
        vsndq.QueueDesc = vsnd_preprocess(vsnd, value);
        return true;
    case _(QueueDescHigh):
        if (value)
            virtio_snd_set_fail(vsnd);
        return true;
    case _(QueueDriverLow):
        vsndq.QueueAvail = vsnd_preprocess(vsnd, value);
        return true;
    case _(QueueDriverHigh):
        if (value)
            virtio_snd_set_fail(vsnd);
        return true;
    case _(QueueDeviceLow):
        vsndq.QueueUsed = vsnd_preprocess(vsnd, value);
        return true;
    case _(QueueDeviceHigh):
        if (value)
            virtio_snd_set_fail(vsnd);
        return true;
    case _(QueueNotify):
        if (value < ARRAY_SIZE(vsnd->queues)) {
            switch (value) {
            case VSND_QUEUE_CTRL:
                virtio_queue_notify_handler(vsnd, value,
                                            virtio_snd_desc_handler);
                break;
            case VSND_QUEUE_TX:
                virtio_queue_notify_handler(vsnd, value,
                                            virtio_snd_tx_desc_handler);
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
        if (!RANGE_CHECK(addr, _(Config), sizeof(virtio_snd_config_t)))
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
                      uint32_t value)
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

bool virtio_snd_init(virtio_snd_state_t *vsnd)
{
    if (vsnd_dev_cnt >= VSND_DEV_CNT_MAX) {
        fprintf(stderr,
                "Exceeded the number of virtio-snd devices that can be "
                "allocated.\n");
        return false;
    }

    /* Allocate the memory of private member. */
    vsnd->priv = &vsnd_configs[vsnd_dev_cnt++];

    /*audio_host = CNFAInit(NULL, "semu-virtio-snd", virtio_snd_cb, 44100, 0, 1,
                          0, 1024, NULL, NULL, &guest_playing);
    if (!audio_host) {
        fprintf(stderr, "virtio-snd driver initialization failed.\n");
        return false;
    }*/

    PRIV(vsnd)->jacks = 1;
    PRIV(vsnd)->streams = 1;
    PRIV(vsnd)->chmaps = 1;

    return true;
}
