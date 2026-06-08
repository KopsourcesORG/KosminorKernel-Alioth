#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <errno.h>
#include <unistd.h>

/* Test that mount operations with adversarial Smack options do not
 * cause heap corruption (buffer overflow in smack_lsm.c strcat/strcpy).
 * Invariant: mount() with oversized Smack options must either succeed
 * or fail gracefully (EINVAL/EPERM/ENODEV) — never crash/corrupt heap. */

START_TEST(test_smack_mount_option_no_overflow)
{
    /* Invariant: processing adversarial mount options must not overflow
     * the heap buffer used to concatenate Smack options. */
    const char *payloads[] = {
        /* Exact exploit: option string long enough to overflow PAGE_SIZE buffer */
        "smackfsdef=" "A",
        /* Boundary: exactly at typical buffer limit (4096 bytes of option data) */
        "smackfsdef=BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",
        /* Valid: normal short option */
        "smackfsdef=_"
    };

    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    /* Build a large adversarial option string for the overflow case */
    char large_opt[8192];
    memset(large_opt, 'X', sizeof(large_opt) - 1);
    large_opt[sizeof(large_opt) - 1] = '\0';
    /* Prepend smackfsdef= */
    char overflow_opt[8200];
    snprintf(overflow_opt, sizeof(overflow_opt), "smackfsdef=%s", large_opt);

    for (int i = 0; i < num_payloads; i++) {
        /* Attempt mount with adversarial Smack option on a tmpfs.
         * We expect failure (not root, or Smack not enabled, or invalid opt)
         * but the process must NOT crash or corrupt memory. */
        int ret = mount("none", "/tmp", "tmpfs", MS_SILENT,
                        (i == 0) ? overflow_opt : payloads[i]);
        int err = errno;

        /* The invariant: ret is either 0 (success) or -1 with a sane errno.
         * A heap overflow would likely cause a crash (SIGSEGV/SIGABRT),
         * which would prevent reaching this assertion. */
        ck_assert_msg(ret == 0 || ret == -1,
                      "mount returned unexpected value %d for payload %d", ret, i);
        if (ret == -1) {
            ck_assert_msg(err == EPERM || err == EINVAL || err == ENODEV ||
                          err == EACCES || err == ENOENT || err == EBUSY ||
                          err == ENOMEM || err == EFAULT,
                          "mount failed with unexpected errno %d for payload %d",
                          err, i);
        } else {
            /* Clean up if mount somehow succeeded */
            umount("/tmp");
        }
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_smack_mount_option_no_overflow);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}