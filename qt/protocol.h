#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

namespace XCMGProtocol {
    enum {
        CANID_SAVE_CONFIG      = 0x529,
        CANID_COMPUTE_POSITION = 0x530,
        CANID_SET_POSITION_P1  = 0x531,
        CANID_SET_POSITION_P2  = 0x532,
        CANID_SET_POSITION_P3  = 0x533,
        CANID_SET_POSITION_P4  = 0x534,
        CANID_SET_POSITION_P5  = 0x535,
    };

    struct SaveConfig {
        uint8_t save;
    } __attribute__((packed));

    struct ComputePosition {
        int16_t x;
        int16_t y;
        int16_t z;
        int16_t d;
    } __attribute__((packed));

    struct SetPosition {
        int16_t x;
        int16_t y;
        int16_t z;
    } __attribute__((packed));
}

#endif // PROTOCOL_H
