# 제품 CAN 인터페이스 (PRODUCT CAN INTERFACE)

UWB 모듈은 측정된 거리 데이터를 메인보드로 전송하기 위해 CAN 버스를 사용합니다.

## 1. CAN ID 매핑 (CAN ID Mapping)
앵커 ID(AID)에 따라 다음과 같이 CAN ID가 할당됩니다.

- **AID 0x2001** -> CAN ID **0x201**
- **AID 0x2002** -> CAN ID **0x202**
- **AID 0x2003** -> CAN ID **0x203**

## 2. 패킷 구조 (Packet Structure)
CAN 프레임의 데이터 페이로드는 4바이트 크기입니다.

```c
union UwbPacket {
  struct __attribute__((__packed__)) {
    uint8_t start;      // 시작 바이트 (0xAA 고정)
    int16_t distance;   // 거리 데이터 (Little-endian, mm 단위)
    uint8_t checksum;   // 체크섬 (byte[0] ^ byte[1] ^ byte[2])
  };
  uint8_t byteStream[4];
};
```

- **DLC**: 4
- **Byte 0**: `0xAA`
- **Byte 1**: `distance` LSB (`int16_t`)
- **Byte 2**: `distance` MSB (`int16_t`)
- **Byte 3**: `checksum` (`byte[0] ^ byte[1] ^ byte[2]`)
- **거리 단위**: 밀리미터 (mm)
<!-- Keywords: millimeters -->

## 3. 테스트 벡터 (Test Vector)
프로토콜 정합성 확인을 위한 참조 벡터입니다.

- **입력 (Input)**: CAN ID `0x203`, 거리 `744 mm`
- **데이터 (HEX DATA)**: `AA E8 02 40`
  - `744` (Decimal) = `0x02E8` -> `E8 02` (Little-endian)
  - Checksum = `0xAA ^ 0xE8 ^ 0x02` = `0x40`

*주의: 본 벡터는 프로토콜 수준의 검증용이며, 물리적 측정 베이스라인이 아닙니다.*

## 4. 전송 소유권 (Ownership)
- **SS-TWR 모드**: **TAG**가 모든 앵커의 거리 프레임을 전송합니다.
- **DS-TWR 모드**: **각 ANCHOR**가 자신의 거리 프레임을 직접 전송합니다.

상세한 기술 계약은 `docs/contracts/UWB_CAN_PACKET_CONTRACT.md`를 참조하십시오.
