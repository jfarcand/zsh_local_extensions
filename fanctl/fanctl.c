// fanctl — minimal Apple SMC fan control (Apple Silicon + Intel).
//
//   fanctl status                 fan RPM, limits, mode, hottest CPU sensor
//   fanctl set <rpm>              force every fan to <rpm>            (root)
//   fanctl auto                   hand the fans back to the SMC       (root)
//   fanctl daemon [--rpm N] [--ramp-from C] [--ramp-to C] [--interval S]
//                                 hold N RPM; above ramp-from, ramp linearly
//                                 to the fan maximum at ramp-to. Restores
//                                 auto on SIGTERM/SIGINT/exit.        (root)
//
// Build: clang -O2 -Wall -framework IOKit fanctl.c -o fanctl

#include <IOKit/IOKitLib.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct { uint8_t major, minor, build, reserved; uint16_t release; } SMCVers;
typedef struct { uint16_t version, length; uint32_t cpu, gpu, mem; } SMCPLimit;
typedef struct { uint32_t dataSize, dataType; uint8_t dataAttributes; } SMCKeyInfo;
typedef struct {
    uint32_t key;
    SMCVers vers;
    SMCPLimit pLimit;
    SMCKeyInfo keyInfo;
    uint8_t result, status, data8;
    uint32_t data32;
    uint8_t bytes[32];
} SMCParam;

enum { CMD_READ = 5, CMD_WRITE = 6, CMD_INDEX = 8, CMD_INFO = 9, SELECTOR = 2 };

static io_connect_t conn;

static uint32_t k4(const char *s) {
    return (uint32_t)s[0] << 24 | (uint32_t)s[1] << 16 | (uint32_t)s[2] << 8 | (uint32_t)s[3];
}

static int smc_call(SMCParam *in, SMCParam *out) {
    size_t sz = sizeof(*out);
    if (IOConnectCallStructMethod(conn, SELECTOR, in, sizeof(*in), out, &sz) != kIOReturnSuccess)
        return -1;
    return out->result ? -1 : 0;
}

static int smc_info(uint32_t key, SMCKeyInfo *ki) {
    SMCParam in = {0}, out = {0};
    in.key = key;
    in.data8 = CMD_INFO;
    if (smc_call(&in, &out)) return -1;
    *ki = out.keyInfo;
    return 0;
}

static int smc_read(uint32_t key, SMCKeyInfo *ki, uint8_t bytes[32]) {
    if (smc_info(key, ki)) return -1;
    SMCParam in = {0}, out = {0};
    in.key = key;
    in.keyInfo.dataSize = ki->dataSize;
    in.data8 = CMD_READ;
    if (smc_call(&in, &out)) return -1;
    memcpy(bytes, out.bytes, 32);
    return 0;
}

static int smc_write(uint32_t key, const uint8_t *bytes, uint32_t size) {
    SMCKeyInfo ki;
    if (smc_info(key, &ki) || ki.dataSize != size) return -1;
    SMCParam in = {0}, out = {0};
    in.key = key;
    in.data8 = CMD_WRITE;
    in.keyInfo.dataSize = size;
    memcpy(in.bytes, bytes, size);
    return smc_call(&in, &out);
}

static int key_exists(const char *name) {
    SMCKeyInfo ki;
    return smc_info(k4(name), &ki) == 0;
}

// Fan keys are `flt ` on Apple Silicon and `fpe2` (14.2 fixed point) on Intel.
static int read_num(const char *name, double *v) {
    SMCKeyInfo ki;
    uint8_t b[32];
    if (smc_read(k4(name), &ki, b)) return -1;
    if (ki.dataType == k4("flt ") && ki.dataSize == 4) {
        float f;
        memcpy(&f, b, 4);
        *v = f;
    } else if (ki.dataType == k4("fpe2") && ki.dataSize == 2) {
        *v = ((b[0] << 8) | b[1]) / 4.0;
    } else if (ki.dataSize == 1) {
        *v = b[0];
    } else {
        return -1;
    }
    return 0;
}

static int write_num(const char *name, double v) {
    SMCKeyInfo ki;
    uint8_t b[32] = {0};
    if (smc_info(k4(name), &ki)) return -1;
    if (ki.dataType == k4("flt ") && ki.dataSize == 4) {
        float f = (float)v;
        memcpy(b, &f, 4);
    } else if (ki.dataType == k4("fpe2") && ki.dataSize == 2) {
        uint16_t x = (uint16_t)(v * 4.0);
        b[0] = x >> 8;
        b[1] = x & 0xff;
    } else if (ki.dataSize == 1) {
        b[0] = (uint8_t)v;
    } else {
        return -1;
    }
    return smc_write(k4(name), b, ki.dataSize);
}

static int fan_count(void) {
    double n;
    return read_num("FNum", &n) ? 0 : (int)n;
}

static void fan_key(char out[5], int fan, const char *suffix) {
    snprintf(out, 5, "F%d%s", fan, suffix);
}

static double fan_get(int fan, const char *suffix) {
    char k[5];
    double v = NAN;
    fan_key(k, fan, suffix);
    read_num(k, &v);
    return v;
}

// Hottest CPU die sensor: Tp* (performance cores) and Te* (efficiency cores)
// on Apple Silicon, TC* on Intel. Discovered once by walking the key table.
static uint32_t cpu_keys[256];
static int n_cpu_keys = -1;

static void discover_cpu_keys(void) {
    n_cpu_keys = 0;
    SMCKeyInfo ki;
    uint8_t b[32];
    if (smc_read(k4("#KEY"), &ki, b)) return;
    uint32_t total = (uint32_t)b[0] << 24 | b[1] << 16 | b[2] << 8 | b[3];
    for (uint32_t i = 0; i < total && n_cpu_keys < 256; i++) {
        SMCParam in = {0}, out = {0};
        in.data8 = CMD_INDEX;
        in.data32 = i;
        if (smc_call(&in, &out)) continue;
        uint32_t k = out.key;
        char c0 = k >> 24, c1 = (k >> 16) & 0xff;
        if (c0 != 'T' || (c1 != 'p' && c1 != 'e' && c1 != 'C')) continue;
        double v;
        char nm[5] = {c0, c1, (k >> 8) & 0xff, k & 0xff, 0};
        if (read_num(nm, &v) == 0 && v > 0 && v < 130) cpu_keys[n_cpu_keys++] = k;
    }
}

static double cpu_temp(void) {
    if (n_cpu_keys < 0) discover_cpu_keys();
    double max = NAN;
    for (int i = 0; i < n_cpu_keys; i++) {
        uint32_t k = cpu_keys[i];
        char nm[5] = {k >> 24, (k >> 16) & 0xff, (k >> 8) & 0xff, k & 0xff, 0};
        double v;
        if (read_num(nm, &v) == 0 && v > 0 && v < 130 && (isnan(max) || v > max)) max = v;
    }
    return max;
}

// Apple Silicon M3+ refuse manual mode until the `Ftst` unlock key is set.
static int set_manual(int fan, double rpm) {
    char k[5];
    if (key_exists("Ftst") && write_num("Ftst", 1)) return -1;
    fan_key(k, fan, "Md");
    if (key_exists(k)) {
        if (write_num(k, 1)) return -1;
    } else {
        // Intel: FS! is a bitmask of fans under manual control.
        double mask = 0;
        read_num("FS! ", &mask);
        uint8_t b[2] = {0, (uint8_t)((int)mask | (1 << fan))};
        if (smc_write(k4("FS! "), b, 2)) return -1;
    }
    fan_key(k, fan, "Tg");
    return write_num(k, rpm);
}

static int set_auto(void) {
    int rc = 0;
    for (int f = 0; f < fan_count(); f++) {
        char k[5];
        fan_key(k, f, "Md");
        if (key_exists(k)) rc |= write_num(k, 0);
    }
    if (key_exists("FS! ")) {
        uint8_t b[2] = {0, 0};
        rc |= smc_write(k4("FS! "), b, 2);
    }
    if (key_exists("Ftst")) rc |= write_num("Ftst", 0);
    return rc;
}

static double clamp_rpm(int fan, double rpm) {
    double mn = fan_get(fan, "Mn"), mx = fan_get(fan, "Mx");
    if (!isnan(mn) && rpm < mn) rpm = mn;
    if (!isnan(mx) && rpm > mx) rpm = mx;
    return rpm;
}

static int set_all(double rpm) {
    int rc = 0;
    for (int f = 0; f < fan_count(); f++) rc |= set_manual(f, clamp_rpm(f, rpm));
    return rc;
}

static int cmd_status(void) {
    int n = fan_count();
    for (int f = 0; f < n; f++) {
        printf("fan %d: %.0f rpm  target %.0f  min %.0f  max %.0f  mode %s\n", f, fan_get(f, "Ac"),
               fan_get(f, "Tg"), fan_get(f, "Mn"), fan_get(f, "Mx"),
               fan_get(f, "Md") == 1 ? "manual" : "auto");
    }
    printf("cpu:   %.1f °C (hottest of %d sensors)\n", cpu_temp(), n_cpu_keys);
    return n ? 0 : 1;
}

static volatile sig_atomic_t stop;
static void on_signal(int sig) { (void)sig; stop = 1; }
static void restore_auto(void) { set_auto(); }

static int cmd_daemon(double rpm, double ramp_from, double ramp_to, unsigned interval) {
    if (ramp_to <= ramp_from) {
        fprintf(stderr, "--ramp-to must be above --ramp-from\n");
        return 2;
    }
    atexit(restore_auto);
    signal(SIGTERM, on_signal);
    signal(SIGINT, on_signal);
    signal(SIGHUP, on_signal);
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("fanctl daemon: hold %.0f rpm, ramp %.0f→%.0f °C, every %us\n", rpm, ramp_from,
           ramp_to, interval);

    double last = -1;
    while (!stop) {
        double t = cpu_temp();
        for (int f = 0; f < fan_count(); f++) {
            double mx = fan_get(f, "Mx"), want = rpm;
            if (isnan(t)) {
                want = mx;  // no reading: fail loud, not hot
            } else if (t > ramp_from) {
                double x = fmin(1.0, (t - ramp_from) / (ramp_to - ramp_from));
                want = rpm + x * (mx - rpm);
            }
            want = clamp_rpm(f, want);
            // Re-assert every tick: sleep/wake hands the fan back to the SMC.
            if (set_manual(f, want)) fprintf(stderr, "fan %d: SMC write failed\n", f);
            if (f == 0 && fabs(want - last) >= 50) {
                printf("cpu %.1f °C → fan %.0f rpm\n", t, want);
                last = want;
            }
        }
        for (unsigned i = 0; i < interval && !stop; i++) sleep(1);
    }
    printf("fanctl daemon: stopping, fans back to auto\n");
    return 0;
}

static void usage(void) {
    fprintf(stderr,
            "usage: fanctl status | set <rpm> | auto |\n"
            "       daemon [--rpm N] [--ramp-from C] [--ramp-to C] [--interval S]\n");
}

int main(int argc, char **argv) {
    io_service_t svc = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("AppleSMC"));
    if (!svc || IOServiceOpen(svc, mach_task_self(), 0, &conn) != kIOReturnSuccess) {
        fprintf(stderr, "cannot open AppleSMC\n");
        return 1;
    }
    IOObjectRelease(svc);
    if (argc < 2) return cmd_status();

    const char *cmd = argv[1];
    if (!strcmp(cmd, "status")) return cmd_status();

    if (geteuid() != 0) {
        fprintf(stderr, "fanctl %s: writing the SMC needs root (sudo)\n", cmd);
        return 1;
    }
    if (!strcmp(cmd, "auto")) return set_auto() ? 1 : 0;
    if (!strcmp(cmd, "set") && argc == 3) return set_all(atof(argv[2])) ? 1 : cmd_status();
    if (!strcmp(cmd, "daemon")) {
        double rpm = 0, from = 75, to = 95;
        unsigned interval = 3;
        for (int i = 2; i + 1 < argc; i += 2) {
            if (!strcmp(argv[i], "--rpm")) rpm = atof(argv[i + 1]);
            else if (!strcmp(argv[i], "--ramp-from")) from = atof(argv[i + 1]);
            else if (!strcmp(argv[i], "--ramp-to")) to = atof(argv[i + 1]);
            else if (!strcmp(argv[i], "--interval")) interval = (unsigned)atoi(argv[i + 1]);
            else { usage(); return 2; }
        }
        return cmd_daemon(rpm, from, to, interval ? interval : 3);
    }
    usage();
    return 2;
}
