#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct Sample {
    uint8_t flag;
    uint32_t timestamp;
    uint16_t value;
    uint8_t status;
};

struct SampleCompact {
    uint32_t timestamp;
    uint16_t value;
    uint8_t flag;
    uint8_t status;
};

union Storage {
    uint8_t bytes[5];
    uint32_t word;
};

_Static_assert(sizeof(uint8_t) == 1 && _Alignof(uint8_t) == 1,
               "This exercise assumes uint8_t size/alignment 1/1");
_Static_assert(sizeof(uint16_t) == 2 && _Alignof(uint16_t) == 2,
               "This exercise assumes uint16_t size/alignment 2/2");
_Static_assert(sizeof(uint32_t) == 4 && _Alignof(uint32_t) == 4,
               "This exercise assumes uint32_t size/alignment 4/4");

static uint32_t get_u32_be(const uint8_t in[4])
{
    return ((uint32_t)in[0] << 24) | ((uint32_t)in[1] << 16)
           | ((uint32_t)in[2] << 8) | (uint32_t)in[3];
}

static void put_u32_be(uint8_t out[4], uint32_t value)
{
    out[0] = (uint8_t)(value >> 24);
    out[1] = (uint8_t)(value >> 16);
    out[2] = (uint8_t)(value >> 8);
    out[3] = (uint8_t)value;
}

int main(void)
{
    printf("Sample: size=%zu align=%zu offsets=%zu,%zu,%zu,%zu\n",
           sizeof(struct Sample), _Alignof(struct Sample),
           offsetof(struct Sample, flag), offsetof(struct Sample, timestamp),
           offsetof(struct Sample, value), offsetof(struct Sample, status));
    printf("Compact: size=%zu align=%zu; union size=%zu\n",
           sizeof(struct SampleCompact), _Alignof(struct SampleCompact),
           sizeof(union Storage));
    assert(sizeof(struct Sample) == 12);
    assert(sizeof(struct SampleCompact) == 8);
    assert(sizeof(union Storage) == 8);

    const uint32_t probe = 0x01020304u;
    const unsigned char *bytes = (const unsigned char *)&probe;
    if (bytes[0] == 4u) {
        puts("Host byte order: little endian");
    } else if (bytes[0] == 1u) {
        puts("Host byte order: big endian");
    } else {
        puts("Host byte order: other representation");
    }

    uint8_t encoded[4];
    put_u32_be(encoded, 0x12345678u);
    assert(encoded[0] == 0x12 && encoded[1] == 0x34);
    assert(encoded[2] == 0x56 && encoded[3] == 0x78);
    assert(get_u32_be(encoded) == 0x12345678u);

    const uint8_t packed_value[2] = {0xA1, 0x23};
    const uint16_t word = (uint16_t)(((uint16_t)packed_value[0] << 8)
                                   | packed_value[1]);
    const unsigned int type = (word >> 13) & 7u;
    const unsigned int value = word & 0x1FFFu;
    assert(type == 5u && value == 291u);
    printf("Protocol field: type=%u value=%u\n", type, value);
    puts("PASS: layout, byte order, explicit protocol decoding");
    return EXIT_SUCCESS;
}
