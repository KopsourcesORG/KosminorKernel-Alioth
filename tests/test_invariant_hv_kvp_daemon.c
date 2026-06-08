#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* KVP key/value field sizes as defined in the kernel uapi headers
 * and used by hv_kvp_daemon.c */
#define HV_KVP_EXCHANGE_MAX_KEY_SIZE   512
#define HV_KVP_EXCHANGE_MAX_VALUE_SIZE 2048

/* Simulate the record buffer structure used in hv_kvp_daemon.c */
struct kvp_record {
    char key[HV_KVP_EXCHANGE_MAX_KEY_SIZE];
    char value[HV_KVP_EXCHANGE_MAX_VALUE_SIZE];
};

START_TEST(test_kvp_copy_bounds)
{
    /* Invariant: key and value strings copied into fixed-size record buffers
     * must never exceed the buffer boundaries, regardless of input length. */
    const char *key_payloads[] = {
        /* Exact exploit: oversized key exceeding HV_KVP_EXCHANGE_MAX_KEY_SIZE */
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "OVERFLOW_PAST_512",
        /* Boundary: exactly HV_KVP_EXCHANGE_MAX_KEY_SIZE - 1 chars */
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",
        /* Valid: short normal key */
        "valid_key"
    };

    int num_payloads = sizeof(key_payloads) / sizeof(key_payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        struct kvp_record dst;
        memset(&dst, 0, sizeof(dst));

        size_t key_len = strlen(key_payloads[i]);

        /* Security invariant: length of source must fit within destination buffer */
        ck_assert_msg(key_len < HV_KVP_EXCHANGE_MAX_KEY_SIZE,
            "Payload %d: key length %zu exceeds max buffer size %d — "
            "strcpy would overflow the destination buffer",
            i, key_len, HV_KVP_EXCHANGE_MAX_KEY_SIZE);

        /* If safe, perform the copy and verify no overrun occurred */
        if (key_len < HV_KVP_EXCHANGE_MAX_KEY_SIZE) {
            strncpy(dst.key, key_payloads[i], HV_KVP_EXCHANGE_MAX_KEY_SIZE - 1);
            dst.key[HV_KVP_EXCHANGE_MAX_KEY_SIZE - 1] = '\0';
            ck_assert(strlen(dst.key) < HV_KVP_EXCHANGE_MAX_KEY_SIZE);
        }
    }
}
END_TEST

Suite *security_suite(void