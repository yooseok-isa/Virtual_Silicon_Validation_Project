#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define VNPU_VENDOR_ID 0x1b36
#define VNPU_DEVICE_ID 0x1000

#define VNPU_MMIO_SIZE 0x1000

#define REG_DEVICE_ID      0x000
#define REG_REVISION       0x004
#define REG_CAPABILITIES   0x008
#define REG_CONTROL        0x00c
#define REG_STATUS         0x010
#define REG_ERROR_CODE     0x01c
#define REG_VECTOR_LENGTH  0x024
#define REG_INPUT_A0       0x100
#define REG_INPUT_A1       0x104
#define REG_INPUT_B0       0x120
#define REG_INPUT_B1       0x124
#define REG_RESULT         0x140
#define REG_FAULT_CONTROL  0x180

#define CONTROL_START 0x1
#define CONTROL_RESET 0x2

#define STATUS_IDLE 0
#define STATUS_DONE 2
#define STATUS_ERROR 3

#define VNPU_ERR_INVALID_LENGTH 1
#define FAULT_CORRUPT_RESULT 0x4

struct vnpu_device {
    char bdf[64];
    char path[512];
};

static void fail(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

static unsigned long read_hex_file(const char *path)
{
    FILE *fp;
    unsigned long value;

    fp = fopen(path, "r");
    if (!fp) {
        fail(path);
    }

    if (fscanf(fp, "%lx", &value) != 1) {
        fclose(fp);
        fprintf(stderr, "failed to parse hex value from %s\n", path);
        exit(EXIT_FAILURE);
    }

    fclose(fp);
    return value;
}

static int find_vnpu(struct vnpu_device *out)
{
    DIR *dir;
    struct dirent *entry;

    dir = opendir("/sys/bus/pci/devices");
    if (!dir) {
        fail("opendir /sys/bus/pci/devices");
    }

    while ((entry = readdir(dir)) != NULL) {
        char base[512];
        char vendor_path[600];
        char device_path[600];
        unsigned long vendor;
        unsigned long device;

        if (entry->d_name[0] == '.') {
            continue;
        }

        if (strlen(entry->d_name) >= sizeof(out->bdf)) {
            continue;
        }

        snprintf(base, sizeof(base), "/sys/bus/pci/devices/%s", entry->d_name);
        snprintf(vendor_path, sizeof(vendor_path), "%s/vendor", base);
        snprintf(device_path, sizeof(device_path), "%s/device", base);

        vendor = read_hex_file(vendor_path);
        device = read_hex_file(device_path);

        if (vendor == VNPU_VENDOR_ID && device == VNPU_DEVICE_ID) {
            snprintf(out->bdf, sizeof(out->bdf), "%s", entry->d_name);
            snprintf(out->path, sizeof(out->path), "%s", base);
            closedir(dir);
            return 0;
        }
    }

    closedir(dir);
    return -1;
}

static void read_bar0_range(const struct vnpu_device *dev,
                            unsigned long long *start,
                            unsigned long long *end)
{
    char resource_path[600];
    FILE *fp;

    snprintf(resource_path, sizeof(resource_path), "%s/resource", dev->path);
    fp = fopen(resource_path, "r");
    if (!fp) {
        fail(resource_path);
    }

    if (fscanf(fp, "%llx %llx", start, end) != 2) {
        fclose(fp);
        fprintf(stderr, "failed to parse BAR0 from %s\n", resource_path);
        exit(EXIT_FAILURE);
    }

    fclose(fp);
}

static uint32_t mmio_read32(volatile uint8_t *mmio, uint32_t offset)
{
    volatile uint32_t *reg = (volatile uint32_t *)(void *)(mmio + offset);
    return *reg;
}

static void mmio_write32(volatile uint8_t *mmio, uint32_t offset, uint32_t value)
{
    volatile uint32_t *reg = (volatile uint32_t *)(void *)(mmio + offset);
    *reg = value;
}

static void expect_reg(volatile uint8_t *mmio,
                       uint32_t offset,
                       uint32_t expected,
                       const char *name)
{
    uint32_t actual = mmio_read32(mmio, offset);

    if (actual != expected) {
        fprintf(stderr, "%s: expected 0x%08" PRIx32 ", got 0x%08" PRIx32 "\n",
                name, expected, actual);
        exit(EXIT_FAILURE);
    }

    printf("ok: %s = 0x%08" PRIx32 "\n", name, actual);
}

static void sleep_10ms(void)
{
    struct timespec ts = {
        .tv_sec = 0,
        .tv_nsec = 10 * 1000 * 1000,
    };

    while (nanosleep(&ts, &ts) < 0 && errno == EINTR) {
    }
}

static void wait_done(volatile uint8_t *mmio)
{
    int i;

    for (i = 0; i < 100; i++) {
        uint32_t status = mmio_read32(mmio, REG_STATUS);

        if (status == STATUS_DONE) {
            return;
        }

        if (status == STATUS_ERROR) {
            fprintf(stderr, "device entered STATUS_ERROR, error_code=0x%08" PRIx32 "\n",
                    mmio_read32(mmio, REG_ERROR_CODE));
            exit(EXIT_FAILURE);
        }

        sleep_10ms();
    }

    fprintf(stderr, "timed out waiting for STATUS_DONE\n");
    exit(EXIT_FAILURE);
}

static void run_dot(volatile uint8_t *mmio, uint32_t expected, const char *name)
{
    uint32_t result;

    mmio_write32(mmio, REG_VECTOR_LENGTH, 8);
    mmio_write32(mmio, REG_INPUT_A0, 0x04030201);
    mmio_write32(mmio, REG_INPUT_A1, 0x08070605);
    mmio_write32(mmio, REG_INPUT_B0, 0x01010101);
    mmio_write32(mmio, REG_INPUT_B1, 0x01010101);
    mmio_write32(mmio, REG_CONTROL, CONTROL_START);

    wait_done(mmio);

    result = mmio_read32(mmio, REG_RESULT);
    if (result != expected) {
        fprintf(stderr, "%s: expected result 0x%08" PRIx32 ", got 0x%08" PRIx32 "\n",
                name, expected, result);
        exit(EXIT_FAILURE);
    }

    printf("ok: %s result = 0x%08" PRIx32 "\n", name, result);
}

static void test_invalid_length(volatile uint8_t *mmio)
{
    mmio_write32(mmio, REG_CONTROL, CONTROL_RESET);
    mmio_write32(mmio, REG_VECTOR_LENGTH, 32);

    expect_reg(mmio, REG_STATUS, STATUS_ERROR, "STATUS after invalid length");
    expect_reg(mmio, REG_ERROR_CODE, VNPU_ERR_INVALID_LENGTH,
               "ERROR_CODE after invalid length");
    mmio_write32(mmio, REG_CONTROL, CONTROL_RESET);
}

int main(void)
{
    struct vnpu_device dev;
    unsigned long long bar0_start;
    unsigned long long bar0_end;
    unsigned long long bar0_size;
    char resource0_path[600];
    int fd;
    volatile uint8_t *mmio;

    if (geteuid() != 0) {
        fprintf(stderr, "must run as root to mmap PCI resource0\n");
        return EXIT_FAILURE;
    }

    if (find_vnpu(&dev) < 0) {
        fprintf(stderr, "VNPU PCI device 0x%04x:0x%04x not found\n",
                VNPU_VENDOR_ID, VNPU_DEVICE_ID);
        return EXIT_FAILURE;
    }

    read_bar0_range(&dev, &bar0_start, &bar0_end);
    bar0_size = bar0_end - bar0_start + 1;
    if (bar0_size != VNPU_MMIO_SIZE) {
        fprintf(stderr, "BAR0 size expected 0x%x, got 0x%llx\n",
                VNPU_MMIO_SIZE, bar0_size);
        return EXIT_FAILURE;
    }

    snprintf(resource0_path, sizeof(resource0_path), "%s/resource0", dev.path);
    fd = open(resource0_path, O_RDWR | O_SYNC);
    if (fd < 0) {
        fail(resource0_path);
    }

    mmio = mmap(NULL, VNPU_MMIO_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mmio == MAP_FAILED) {
        fail("mmap resource0");
    }

    printf("VNPU BDF: %s\n", dev.bdf);
    printf("BAR0: 0x%llx-0x%llx (%llu bytes)\n",
           bar0_start, bar0_end, bar0_size);

    mmio_write32(mmio, REG_CONTROL, CONTROL_RESET);

    expect_reg(mmio, REG_DEVICE_ID, 0x564e5055, "DEVICE_ID");
    expect_reg(mmio, REG_REVISION, 0x2, "REVISION");
    expect_reg(mmio, REG_CAPABILITIES, 0x7D, "CAPABILITIES");
    expect_reg(mmio, REG_STATUS, STATUS_IDLE, "STATUS after reset");
    expect_reg(mmio, REG_VECTOR_LENGTH, 0x10, "VECTOR_LENGTH after reset");

    run_dot(mmio, 0x24, "dot8");

    mmio_write32(mmio, REG_CONTROL, CONTROL_RESET);
    mmio_write32(mmio, REG_FAULT_CONTROL, FAULT_CORRUPT_RESULT);
    run_dot(mmio, 0x25, "corrupt-result fault");

    test_invalid_length(mmio);

    if (munmap((void *)mmio, VNPU_MMIO_SIZE) < 0) {
        fail("munmap");
    }
    close(fd);

    printf("PASS: VNPU mmap smoke test\n");
    return EXIT_SUCCESS;
}
