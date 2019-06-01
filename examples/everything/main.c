/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019, Erik Moqvist
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * This file is part of the Monolinux project.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mount.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <curl/curl.h>
#include <heatshrink_encoder.h>
#include <heatshrink_decoder.h>
#include <lzma.h>
#include <detools.h>
#include <openssl/ssl.h>
#include <openssl/aes.h>
#include "ml/ml.h"

extern int command_lzma_compress(int argc, const char *argv[]);

static int pmount(const char *source_p,
                  const char *target_p,
                  const char *type_p)
{
    int res;

    res = mount(source_p, target_p, type_p, 0, "");

    if (res != 0) {
        perror("error: mount");
    }

    return (res);
}

static size_t on_write(void *buf_p, size_t size, size_t nmemb, void *arg_p)
{
    (void)arg_p;

    fwrite(buf_p, size, nmemb, stdout);

    return (size * nmemb);
}

static void http_get(const char *url_p)
{
    CURL *curl_p;
    long response_code;
    int res;

    printf(">>> HTTP GET %s. >>>\n", url_p);

    curl_p = curl_easy_init();

    if (curl_p) {
        curl_easy_setopt(curl_p, CURLOPT_URL, url_p);
        curl_easy_setopt(curl_p, CURLOPT_WRITEFUNCTION, on_write);

        /* WARNING: Makes the connection unsecure! */
        curl_easy_setopt(curl_p, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl_p, CURLOPT_SSL_VERIFYHOST, 0);

        res = curl_easy_perform(curl_p);

        if (res == CURLE_OK) {
            curl_easy_getinfo(curl_p, CURLINFO_RESPONSE_CODE, &response_code);
            printf("<<< HTTP GET response code %ld. <<<\n", response_code);
        } else {
            printf("<<< HTTP GET CURL error code %d: %s. <<<\n",
                   res,
                   curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl_p);
    }

    curl_global_cleanup();
}

static int command_http_get(int argc, const char *argv[])
{
    if (argc != 2) {
        printf("http_get <url>\n");

        return (-1);
    }

    http_get(argv[1]);

    return (0);
}

static void init(void)
{
    int res;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    ml_init();
    ml_shell_init();
    ml_network_init();
    ml_shell_register_command("lzmac",
                              "LZMA compress.",
                              command_lzma_compress);
    ml_shell_register_command("http_get",
                              "HTTP GET.",
                              command_http_get);
    ml_shell_start();

    pmount("none", "/proc", "proc");
    pmount("none", "/sys", "sysfs");
    pmount("none", "/sys/kernel/debug", "debugfs");
    pmount("/dev/sda1", "/mnt/disk", "ext4");
}

static void print_banner(void)
{
    int fd;
    char buf[128];
    ssize_t size;

    fd = open("/proc/uptime", O_RDONLY);

    if (fd < 0) {
        perror("error: ");

        return;
    }

    size = read(fd, &buf[0], sizeof(buf) - 1);

    if (size >= 0) {
        buf[size] = '\0';
        strtok(&buf[0], " ");
    } else {
        strcpy(&buf[0], "-");
    }

    close(fd);

    printf("Welcome to Monolinux!\n"
           "\n"
           "Uptime: %s\n"
           "\n",
           &buf[0]);
}

static void disk_test(void)
{
    printf("================ disk test begin ===============\n");
    ml_print_file_systems_space_usage();
    ml_print_file("/mnt/disk/README");
    printf("================= disk test end ================\n\n");
}

static void heatshrink_test(void)
{
    printf("============= heatshrink test begin ============\n");

    printf("Heatshrink encode and decode.\n");

    (void)heatshrink_encoder_alloc(8, 4);
    (void)heatshrink_decoder_alloc(512, 8, 4);

    printf("============= heatshrink test end ==============\n\n");
}

static void lzma_test(void)
{
    lzma_ret ret;
    lzma_stream stream;

    printf("================ lzma test begin ===============\n");

    memset(&stream, 0, sizeof(stream));

    ret = lzma_alone_decoder(&stream, UINT64_MAX);

    if (ret != LZMA_OK) {
        printf("LZMA decoder init failed.\n");
    } else {
        printf("LZMA decoder init successful.\n");
    }

    printf("================= lzma test end ================\n\n");
}

static void detools_test(void)
{
    int res;
    const char from[] = "/mnt/disk/detools/v1.txt";
    const char patch[] = "/mnt/disk/detools/v1-v2.patch";
    const char to[] = "/mnt/disk/detools/v2.txt";

    printf("============== detools test begin ==============\n");
    printf("Applying patch '%s' to '%s' to create '%s'.\n",
           patch,
           from,
           to);

    res = detools_apply_patch_filenames(from, patch, to);

    if (res >= 0) {
        printf("Before patching:\n");
        ml_print_file(from);
        printf("After patching:\n");
        ml_print_file(to);
        printf("detools: OK!\n");
    } else {
        res *= -1;
        printf("error: detools: %s (error code %d)\n",
               detools_error_as_string(res),
               res);
    }

    printf("=============== detools test end ===============\n\n");
}

static void openssl_test(void)
{
    printf("============== openssl test begin ==============\n");
    printf("openssl AES options: '%s'\n", AES_options());
    printf("=============== openssl test end ===============\n\n");
}

static void insert_module_test(void)
{
    int res;
    const char module[] = "/root/i2c-i801.ko";

    printf("=============== insmod test begin ==============\n");

    res = ml_insert_module(&module[0], "");

    if (res == 0) {
        printf("Successfully inserted '%s'.\n", &module[0]);
    } else {
        printf("Failed to insert '%s'.\n", &module[0]);
    }

    printf("Loaded modules:\n");
    ml_print_file("/proc/modules");

    printf("================ insmod test end ===============\n\n");
}

static void http_test(void)
{
    printf("================ http test begin ===============\n");
    http_get("http://10.0.2.2:8001/");
    http_get("https://10.0.2.2:4443/");
    printf("================= http test end ================\n\n");
}

int main()
{
    init();

    print_banner();
    disk_test();
    heatshrink_test();
    lzma_test();
    detools_test();
    openssl_test();
    insert_module_test();
    ml_network_interface_configure("eth0", "10.0.2.15", "255.255.255.0");
    ml_network_interface_up("eth0");
    http_test();

    while (1) {
        sleep(10);
    }

    return (0);
}