/* C11 host exercise: 2-byte BE payload length followed by 0..64 bytes.
 * A callback borrows the payload only for the duration of that callback.
 * Callbacks must not re-enter or reset the same parser.
 * The caller owns connection handling and partial-frame timeouts.
 */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MAX_PAYLOAD = 64, MAX_CAPTURED = 8 };
typedef void (*frame_callback)(const uint8_t *, size_t, void *);

struct Parser {
    uint8_t header[2];
    size_t header_used;
    size_t wanted;
    size_t payload_used;
    uint8_t payload[MAX_PAYLOAD];
    bool failed;
};

static void parser_reset(struct Parser *p)
{
    memset(p, 0, sizeof *p);
}

static void frame_complete(struct Parser *p, frame_callback cb, void *ctx)
{
    cb(p->payload, p->wanted, ctx);
    p->header_used = 0;
    p->payload_used = 0;
    p->wanted = 0;
}

static bool parser_feed(struct Parser *p, const uint8_t *input, size_t size,
                        frame_callback cb, void *ctx)
{
    assert(p != NULL && cb != NULL);
    assert(input != NULL || size == 0);
    if (p->failed) {
        return false;
    }
    while (size != 0) {
        if (p->header_used < sizeof p->header) {
            p->header[p->header_used++] = *input++;
            --size;
            if (p->header_used == sizeof p->header) {
                p->wanted = ((size_t)p->header[0] << 8) | p->header[1];
                if (p->wanted > MAX_PAYLOAD) {
                    p->failed = true; /* Reject connection; do not guess a boundary. */
                    return false;
                }
                if (p->wanted == 0) {
                    frame_complete(p, cb, ctx);
                }
            }
            continue;
        }

        size_t take = p->wanted - p->payload_used;
        if (take > size) {
            take = size;
        }
        memcpy(p->payload + p->payload_used, input, take);
        p->payload_used += take;
        input += take;
        size -= take;
        if (p->payload_used == p->wanted) {
            frame_complete(p, cb, ctx);
        }
    }
    return true;
}

struct Capture {
    size_t count;
    size_t lengths[MAX_CAPTURED];
    uint8_t values[MAX_CAPTURED][MAX_PAYLOAD];
};

static void capture_frame(const uint8_t *payload, size_t len, void *ctx)
{
    struct Capture *c = ctx;
    assert(c->count < MAX_CAPTURED && len <= MAX_PAYLOAD);
    c->lengths[c->count] = len;
    memcpy(c->values[c->count], payload, len);
    ++c->count;
}

static void check_three_frames(const struct Capture *c)
{
    assert(c->count == 3);
    assert(c->lengths[0] == 3 && memcmp(c->values[0], "ABC", 3) == 0);
    assert(c->lengths[1] == 0);
    assert(c->lengths[2] == 2 && memcmp(c->values[2], "DE", 2) == 0);
}

int main(void)
{
    const uint8_t stream[] = {0, 3, 'A', 'B', 'C', 0, 0, 0, 2, 'D', 'E'};
    const unsigned int split_patterns = 1u << (sizeof stream - 1u);

    /* All 1024 possible receive-boundary combinations for this 11-byte stream. */
    for (unsigned int mask = 0; mask < split_patterns; ++mask) {
        struct Parser p;
        struct Capture c = {0};
        parser_reset(&p);
        size_t start = 0;
        for (size_t end = 1; end <= sizeof stream; ++end) {
            if (end == sizeof stream || (mask & (1u << (end - 1u))) != 0) {
                assert(parser_feed(&p, stream + start, end - start,
                                   capture_frame, &c));
                start = end;
            }
        }
        check_three_frames(&c);
        assert(p.header_used == 0 && p.payload_used == 0 && !p.failed);
    }

    uint8_t largest[MAX_PAYLOAD + 2] = {0, MAX_PAYLOAD};
    for (size_t i = 0; i < MAX_PAYLOAD; ++i) {
        largest[i + 2] = (uint8_t)i;
    }
    for (size_t chunk = 1; chunk <= sizeof largest; ++chunk) {
        struct Parser p;
        struct Capture c = {0};
        parser_reset(&p);
        for (size_t pos = 0; pos < sizeof largest;) {
            size_t n = sizeof largest - pos;
            if (n > chunk) {
                n = chunk;
            }
            assert(parser_feed(&p, largest + pos, n, capture_frame, &c));
            pos += n;
        }
        assert(c.count == 1 && c.lengths[0] == MAX_PAYLOAD);
        assert(memcmp(c.values[0], largest + 2, MAX_PAYLOAD) == 0);
    }

    struct Parser p;
    struct Capture c = {0};
    parser_reset(&p);
    const uint8_t oversized[] = {0, MAX_PAYLOAD + 1};
    assert(!parser_feed(&p, oversized, sizeof oversized, capture_frame, &c));
    assert(c.count == 0);
    assert(!parser_feed(&p, stream, sizeof stream, capture_frame, &c));
    parser_reset(&p); /* New connection after rejecting an invalid length. */
    assert(parser_feed(&p, NULL, 0, capture_frame, &c));
    assert(parser_feed(&p, stream, 3, capture_frame, &c));
    assert(c.count == 0); /* Header and first body byte only. */
    parser_reset(&p); /* Disconnect: discard incomplete frame. */
    assert(parser_feed(&p, stream, sizeof stream, capture_frame, &c));
    check_three_frames(&c);

    printf("PASS: %u split patterns, max length, zero length, bad length, reset\n",
           split_patterns);
    return EXIT_SUCCESS;
}
